#include "timeLine.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPushButton>
#include <QFileDialog>
#include <QFile>
#include <QTextStream>
#include <QFrame>
#include <QLabel>
#include <QStatusBar>
#include <QSlider>
#include <QDebug>
#include <QTimer>
#include <QScrollArea>
#include <QScrollBar>
#include <QApplication>

timeLine::timeLine(QWidget *parent)
    : QMainWindow(parent),
    m_isCalibrating(false),
    m_currentScale(0.6)
{
    setWindowTitle("Pro Video Tracker - 自動裁切預覽系統");
    resize(1100, 900);

    setStatusBar(new QStatusBar(this));
    setStyleSheet(R"(
        QMainWindow { background:#121212; }
        QWidget { color:#e6e6e6; font-family:"Microsoft JhengHei"; }
        QFrame#Card { background:#1e1e1e; border:1px solid #2a2a2a; border-radius:12px; }
        QLabel#Title { font-size:16px; font-weight:bold; color:#00bcd4; }
        QPushButton { background:#2a2a2a; border:1px solid #3a3a3a; border-radius:6px; padding:8px; min-width: 80px; }
        QPushButton:hover { border:1px solid #00bcd4; }
        QSlider::handle:horizontal { background:#00bcd4; width:14px; margin:-5px 0; border-radius:7px; }
    )");

    m_player = new QMediaPlayer(this);
    m_audioOutput = new QAudioOutput(this);
    m_player->setAudioOutput(m_audioOutput);

    QWidget *central = new QWidget(this);
    QVBoxLayout *mainLayout = new QVBoxLayout(central);
    mainLayout->setContentsMargins(16, 16, 16, 16);
    mainLayout->setSpacing(12);

    // --- 🎥 Video Section ---
    QFrame *videoCard = new QFrame;
    videoCard->setObjectName("Card");
    QVBoxLayout *videoLayout = new QVBoxLayout(videoCard);

    QLabel *videoTitle = new QLabel("🎥 影像預覽 (已自動裁切)");
    videoTitle->setObjectName("Title");
    videoLayout->addWidget(videoTitle);

    QScrollArea *scrollArea = new QScrollArea(videoCard);
    scrollArea->setWidgetResizable(false);
    scrollArea->setFrameShape(QFrame::NoFrame);
    // 為了 Debug 建議先開啟捲軸觀測，確定沒問題後再設為 AlwaysOff
    scrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    scrollArea->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    scrollArea->setStyleSheet("background: #000000; border-radius: 8px;");

    // ⭐ 重要修正：移除 AlignCenter，改回預設左上對齊
    // 這樣捲軸 0 的位置就是影片內容的最左邊
    scrollArea->setAlignment(Qt::AlignLeft | Qt::AlignTop);

    m_videoWidget = new ClickableVideoWidget();
    m_player->setVideoOutput(m_videoWidget);
    m_videoWidget->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);

    // ⭐ 建立一個中間層容器，給予超大邊距，確保人物在邊緣也能置中
    QWidget *container = new QWidget();
    container->setStyleSheet("background: transparent;");
    QVBoxLayout *containerLayout = new QVBoxLayout(container);
    containerLayout->setContentsMargins(1000, 500, 1000, 500); // 預留極大黑邊空間
    containerLayout->addWidget(m_videoWidget);

    scrollArea->setWidget(container);
    m_videoContainer = scrollArea;

    videoLayout->addWidget(scrollArea, 1);
    mainLayout->addWidget(videoCard, 5);

    // --- ⏱️ Timeline ---
    QFrame *timeCard = new QFrame;
    timeCard->setObjectName("Card");
    QVBoxLayout *timeLayout = new QVBoxLayout(timeCard);
    m_timeSlider = new QSlider(Qt::Horizontal);
    timeLayout->addWidget(new QLabel("⏱️ 數據涵蓋時間軸", timeCard));
    timeLayout->addWidget(m_timeSlider);
    mainLayout->addWidget(timeCard);

    // --- 🔽 Bottom Area ---
    QWidget *bottomWidget = new QWidget;
    QHBoxLayout *bottomLayout = new QHBoxLayout(bottomWidget);
    m_visualMap = new VisualMap(this);

    QFrame *controlCard = new QFrame;
    controlCard->setObjectName("Card");
    QVBoxLayout *controlLayout = new QVBoxLayout(controlCard);

    m_btnPlayPause = new QPushButton("⏸️ 暫停");
    QPushButton *btnLoad = new QPushButton("📂 載入數據與影片");

    controlLayout->addWidget(m_btnPlayPause);
    controlLayout->addWidget(btnLoad);
    controlLayout->addStretch();

    bottomLayout->addWidget(m_visualMap, 3);
    bottomLayout->addWidget(controlCard, 2);
    mainLayout->addWidget(bottomWidget, 3);

    setCentralWidget(central);

    connect(btnLoad, &QPushButton::clicked, this, &timeLine::loadFile);
    connect(m_btnPlayPause, &QPushButton::clicked, this, &timeLine::togglePlayPause);
    connect(m_player, &QMediaPlayer::positionChanged, this, &timeLine::onPositionChanged);
    connect(m_player, &QMediaPlayer::durationChanged, this, &timeLine::onDurationChanged);
    connect(m_timeSlider, &QSlider::sliderMoved, m_player, &QMediaPlayer::setPosition);
    connect(m_videoWidget, &ClickableVideoWidget::clicked, this, &timeLine::onVideoClicked);
}

void timeLine::togglePlayPause() {
    if (m_player->playbackState() == QMediaPlayer::PlayingState) {
        m_player->pause();
        m_btnPlayPause->setText("▶️ 播放");
    } else {
        m_player->play();
        m_btnPlayPause->setText("⏸️ 暫停");
    }
}

void timeLine::loadFile() {
    QString csv = QFileDialog::getOpenFileName(this, "選擇 CSV", "", "*.csv");
    if (csv.isEmpty()) return;

    QFile f(csv);
    if (f.open(QIODevice::ReadOnly)) {
        QTextStream in(&f);
        m_dataPoints.clear();
        while (!in.atEnd()) {
            auto s = in.readLine().split(",");
            if (s.size() >= 3) {
                DataPoint d = {s[0].toDouble(), s[1].toDouble(), s[2].toDouble()};
                m_dataPoints.append(d);
            }
        }
        if (!m_dataPoints.isEmpty()) {
            m_startTime = m_dataPoints.first().time;
            m_endTime = m_dataPoints.last().time;
            m_timeSlider->setRange(m_startTime * 1000, m_endTime * 1000);
        }
    }

    QString video = QFileDialog::getOpenFileName(this, "選擇影片", "", "*.mp4 *.avi");
    if (!video.isEmpty()) {
        m_player->setSource(QUrl::fromLocalFile(video));
        connect(m_player, &QMediaPlayer::mediaStatusChanged, this, [this](QMediaPlayer::MediaStatus status){
            if (status == QMediaPlayer::LoadedMedia) {
                m_player->setPosition(m_startTime * 1000);
                QTimer::singleShot(300, this, &timeLine::applyAutoZoom);
            }
        });
        m_player->play();
        m_btnPlayPause->setText("⏸️ 暫停");
    }
}

void timeLine::applyAutoZoom() {
    if (m_dataPoints.isEmpty() || !m_videoContainer || !m_videoWidget) return;

    m_videoWidget->setFixedSize(1920 * m_currentScale, 1080 * m_currentScale);
    QApplication::processEvents();

    QTimer::singleShot(200, this, [=]() {
        onPositionChanged(m_player->position());
    });
}

void timeLine::onPositionChanged(qint64 position) {
    double sec = position / 1000.0;
    m_timeSlider->setValue(position);

    for (const auto &pt : m_dataPoints) {
        if (qAbs(pt.time - sec) < 0.033) {
            m_visualMap->updatePosition(pt.x, pt.y);

            QScrollArea *sa = qobject_cast<QScrollArea*>(m_videoContainer);
            if (sa) {
                // ⭐ 新的平移公式：
                // 基礎偏移(容器邊距) + 人物縮放位置 - 視窗一半寬度
                int targetX = 1000 + (pt.x * m_currentScale) - (sa->viewport()->width() / 2);
                int targetY = 500 + (pt.y * m_currentScale) - (sa->viewport()->height() / 2);

                sa->horizontalScrollBar()->setValue(targetX);
                sa->verticalScrollBar()->setValue(targetY);

                qDebug() << QString("Time: %1s | Person: (%2, %3) | Scroll: (%4, %5)")
                                .arg(sec, 0, 'f', 2).arg(pt.x).arg(pt.y).arg(targetX).arg(targetY);
            }
            break;
        }
    }
}

void timeLine::onVideoClicked(const QPoint &pos) {
    if (!m_isCalibrating) return;
    int originalX = pos.x() / m_currentScale;
    int originalY = pos.y() / m_currentScale;
    qDebug() << "Calibration Click:" << originalX << originalY;
}

void timeLine::onDurationChanged(qint64 duration) {
    Q_UNUSED(duration);
}
