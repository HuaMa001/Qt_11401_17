#include "image_processor.h"

#include <QWidget>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGroupBox>
#include <QFrame>
#include <QPushButton>
#include <QSlider>
#include <QLabel>
#include <QFileDialog>
#include <QStyle>

#include <QMediaPlayer>
#include <QAudioOutput>
#include <QVideoWidget>
#include <QUrl>

static QString msToTime(qint64 ms) {
    qint64 sec = ms / 1000;
    qint64 m = sec / 60;
    qint64 s = sec % 60;
    return QString("%1:%2").arg(m, 2, 10, QChar('0'))
        .arg(s, 2, 10, QChar('0'));
}

Image_Processor::Image_Processor(QWidget *parent)
    : QMainWindow(parent)
{
    buildUpperUI();

    // Qt6 multimedia
    player = new QMediaPlayer(this);
    audio  = new QAudioOutput(this);
    player->setAudioOutput(audio);
    player->setVideoOutput(videoWidget);

    connect(player, &QMediaPlayer::durationChanged, this, &Image_Processor::onDurationChanged);
    connect(player, &QMediaPlayer::positionChanged, this, &Image_Processor::onPositionChanged);
}

Image_Processor::~Image_Processor() {}

void Image_Processor::buildUpperUI() {
    auto *central = new QWidget(this);
    setCentralWidget(central);

    auto *root = new QVBoxLayout(central);
    root->setContentsMargins(16, 16, 16, 16);
    root->setSpacing(14);

    // ===== 顯示影片區塊 =====
    groupVideo = new QGroupBox(QStringLiteral("顯示影片區塊"), central);
    auto *vbox = new QVBoxLayout(groupVideo);
    vbox->setSpacing(10);

    videoFrame = new QFrame(groupVideo);
    videoFrame->setFrameShape(QFrame::Box);
    videoFrame->setLineWidth(2);
    videoFrame->setMinimumHeight(260);
    videoFrame->setStyleSheet("QFrame { background: #111; }");

    auto *videoFrameLayout = new QVBoxLayout(videoFrame);
    videoFrameLayout->setContentsMargins(6, 6, 6, 6);

    videoWidget = new QVideoWidget(videoFrame);
    videoWidget->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    videoFrameLayout->addWidget(videoWidget);

    vbox->addWidget(videoFrame);

    // controls row
    auto *controlRow = new QHBoxLayout();
    controlRow->setSpacing(10);

    btnImport = new QPushButton(QStringLiteral("匯入影片"), groupVideo);
    btnPlayPause = new QPushButton(QStringLiteral("暫停播放"), groupVideo);
    btnRecordPlayback = new QPushButton(QStringLiteral("錄製播放"), groupVideo);

    btnImport->setIcon(style()->standardIcon(QStyle::SP_DialogOpenButton));
    btnPlayPause->setIcon(style()->standardIcon(QStyle::SP_MediaPlay));
    btnRecordPlayback->setIcon(style()->standardIcon(QStyle::SP_ComputerIcon));

    slider = new QSlider(Qt::Horizontal, groupVideo);
    slider->setRange(0, 0);

    timeLabel = new QLabel("00:00 / 00:00", groupVideo);
    timeLabel->setMinimumWidth(110);
    timeLabel->setAlignment(Qt::AlignRight | Qt::AlignVCenter);

    controlRow->addWidget(btnImport);
    controlRow->addWidget(btnPlayPause);
    controlRow->addWidget(btnRecordPlayback);
    controlRow->addWidget(slider, 1);
    controlRow->addWidget(timeLabel);

    vbox->addLayout(controlRow);
    root->addWidget(groupVideo, 3);

    // ===== 時間軸顯示 =====
    groupTimeline = new QGroupBox(QStringLiteral("時間軸顯示"), central);
    auto *tbox = new QVBoxLayout(groupTimeline);

    timelineFrame = new QFrame(groupTimeline);
    timelineFrame->setFrameShape(QFrame::Box);
    timelineFrame->setLineWidth(2);
    timelineFrame->setMinimumHeight(150);
    timelineFrame->setStyleSheet("QFrame { background: #fafafa; }");

    tbox->addWidget(timelineFrame);
    root->addWidget(groupTimeline, 2);

    // signals
    connect(btnImport, &QPushButton::clicked, this, &Image_Processor::onImportVideo);
    connect(btnPlayPause, &QPushButton::clicked, this, &Image_Processor::onPlayPause);
    connect(btnRecordPlayback, &QPushButton::clicked, this, &Image_Processor::onRecordPlayback);
    connect(slider, &QSlider::sliderMoved, this, &Image_Processor::onSliderMoved);
}

void Image_Processor::onImportVideo() {
    const QString file = QFileDialog::getOpenFileName(
        this,
        QStringLiteral("選擇影片"),
        QString(),
        QStringLiteral("Video Files (*.mp4 *.mov *.mkv *.avi);;All Files (*.*)")
        );
    if (file.isEmpty()) return;

    player->setSource(QUrl::fromLocalFile(file));
    player->play();

    isPlaying = true;
    btnPlayPause->setText(QStringLiteral("暫停播放"));
    btnPlayPause->setIcon(style()->standardIcon(QStyle::SP_MediaPause));
}

void Image_Processor::onPlayPause() {
    if (!player) return;

    if (isPlaying) {
        player->pause();
        isPlaying = false;
        btnPlayPause->setText(QStringLiteral("繼續播放"));
        btnPlayPause->setIcon(style()->standardIcon(QStyle::SP_MediaPlay));
    } else {
        player->play();
        isPlaying = true;
        btnPlayPause->setText(QStringLiteral("暫停播放"));
        btnPlayPause->setIcon(style()->standardIcon(QStyle::SP_MediaPause));
    }
}

void Image_Processor::onRecordPlayback() {
    btnRecordPlayback->setText(QStringLiteral("錄製播放(示意)"));
}

void Image_Processor::onSliderMoved(int value) {
    if (!player) return;
    player->setPosition(value);
}

void Image_Processor::onDurationChanged(qint64 dur) {
    durationMs = dur;
    slider->setRange(0, (int)dur);
    timeLabel->setText(msToTime(0) + " / " + msToTime(durationMs));
}

void Image_Processor::onPositionChanged(qint64 pos) {
    if (!slider->isSliderDown())
        slider->setValue((int)pos);

    timeLabel->setText(msToTime(pos) + " / " + msToTime(durationMs));
}
