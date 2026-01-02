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

timeLine::timeLine(QWidget *parent)
    : QMainWindow(parent),
    m_isCalibrating(false)
{
    setWindowTitle("Pro Video Tracker - 校正系統");
    resize(1100, 900);

    // =========================
    // Status Bar
    // =========================
    setStatusBar(new QStatusBar(this));

    // =========================
    // Style
    // =========================
    setStyleSheet(R"(
        QMainWindow { background:#121212; }
        QWidget { color:#e6e6e6; font-family:"Microsoft JhengHei"; }
        QFrame#Card {
            background:#1e1e1e;
            border:1px solid #2a2a2a;
            border-radius:12px;
        }
        QLabel#Title {
            font-size:16px;
            font-weight:bold;
            color:#00bcd4;
        }
        QPushButton {
            background:#2a2a2a;
            border:1px solid #3a3a3a;
            border-radius:6px;
            padding:8px;
        }
        QPushButton:hover { border:1px solid #00bcd4; }
        QSlider::handle:horizontal {
            background:#00bcd4;
            width:14px;
            margin:-5px 0;
            border-radius:7px;
        }
    )");

    // =========================
    // Media
    // =========================
    m_player = new QMediaPlayer(this);
    m_audioOutput = new QAudioOutput(this);
    m_videoWidget = new ClickableVideoWidget(this);

    m_player->setAudioOutput(m_audioOutput);
    m_player->setVideoOutput(m_videoWidget);

    // =========================
    // Map
    // =========================
    m_visualMap = new VisualMap(this);

    // =========================
    // Layout
    // =========================
    QWidget *central = new QWidget(this);
    QVBoxLayout *mainLayout = new QVBoxLayout(central);
    mainLayout->setContentsMargins(16, 16, 16, 16);
    mainLayout->setSpacing(12);

    // =====================================================
    // 🎥 Video
    // =====================================================
    QFrame *videoCard = new QFrame;
    videoCard->setObjectName("Card");
    QVBoxLayout *videoLayout = new QVBoxLayout(videoCard);

    QLabel *videoTitle = new QLabel("🎥 影像播放");
    videoTitle->setObjectName("Title");

    videoLayout->addWidget(videoTitle);
    videoLayout->addWidget(m_videoWidget, 1);

    mainLayout->addWidget(videoCard, 5);

    // =====================================================
    // ⏱️ Timeline
    // =====================================================
    QFrame *timeCard = new QFrame;
    timeCard->setObjectName("Card");
    QVBoxLayout *timeLayout = new QVBoxLayout(timeCard);

    QLabel *timeTitle = new QLabel("⏱️ 時間軸");
    timeTitle->setObjectName("Title");

    m_timeSlider = new QSlider(Qt::Horizontal);
    m_timeSlider->setRange(0, 0);

    timeLayout->addWidget(timeTitle);
    timeLayout->addWidget(m_timeSlider);

    mainLayout->addWidget(timeCard);

    // =====================================================
    // 🔽 Bottom Area
    // =====================================================
    QWidget *bottomWidget = new QWidget;
    QHBoxLayout *bottomLayout = new QHBoxLayout(bottomWidget);
    bottomLayout->setSpacing(12);

    // ---------------------
    // 🗺️ Map
    // ---------------------
    QFrame *mapCard = new QFrame;
    mapCard->setObjectName("Card");
    QVBoxLayout *mapLayout = new QVBoxLayout(mapCard);

    QLabel *mapTitle = new QLabel("🗺️ 位址軌跡");
    mapTitle->setObjectName("Title");

    mapLayout->addWidget(mapTitle);
    mapLayout->addWidget(m_visualMap, 1);

    // ---------------------
    // ⚙️ Controls
    // ---------------------
    QFrame *controlCard = new QFrame;
    controlCard->setObjectName("Card");
    QVBoxLayout *controlLayout = new QVBoxLayout(controlCard);

    QLabel *controlTitle = new QLabel("⚙️ 功能控制");
    controlTitle->setObjectName("Title");

    QPushButton *btnLoadProject = new QPushButton("📂 載入專案");
    QPushButton *btnRecalibrate = new QPushButton("📍 重新校正座標");
    QPushButton *btnExport = new QPushButton("📤 輸出影片");

    controlLayout->addWidget(controlTitle);
    controlLayout->addWidget(btnLoadProject);
    controlLayout->addWidget(btnRecalibrate);
    controlLayout->addWidget(btnExport);
    controlLayout->addStretch();

    bottomLayout->addWidget(mapCard, 3);
    bottomLayout->addWidget(controlCard, 2);

    mainLayout->addWidget(bottomWidget, 3);
    setCentralWidget(central);

    // =========================
    // Signals
    // =========================
    connect(btnLoadProject, &QPushButton::clicked,
            this, &timeLine::loadFile);

    connect(btnRecalibrate, &QPushButton::clicked, this, [&](){
        m_isCalibrating = true;
        m_calibrationPoints.clear();
        statusBar()->showMessage("校正模式：請在影片上點擊");
    });

    connect(m_player, &QMediaPlayer::durationChanged,
            this, &timeLine::onDurationChanged);

    connect(m_player, &QMediaPlayer::positionChanged,
            this, &timeLine::onPositionChanged);

    connect(m_timeSlider, &QSlider::sliderMoved,
            m_player, &QMediaPlayer::setPosition);

    connect(m_videoWidget, &ClickableVideoWidget::clicked,
            this, &timeLine::onVideoClicked);
}

// =========================
// Slots
// =========================
void timeLine::onDurationChanged(qint64 duration)
{
    m_timeSlider->setRange(0, duration);
}

void timeLine::onPositionChanged(qint64 position)
{
    m_timeSlider->setValue(position);

    double sec = position / 1000.0;
    for (auto &pt : m_dataPoints) {
        if (qAbs(pt.time - sec) < 0.033) {
            m_visualMap->updatePosition(pt.x, pt.y);
            break;
        }
    }
}

void timeLine::onVideoClicked(const QPoint &pos)
{
    if (!m_isCalibrating)
        return;

    QPointF worldPos(100 + m_calibrationPoints.size() * 10, 200);

    CalibrationPoint cp;
    cp.videoPos = pos;
    cp.worldPos = worldPos;
    m_calibrationPoints.append(cp);

    statusBar()->showMessage(
        QString("已選取校正點 %1").arg(m_calibrationPoints.size()));

    if (m_calibrationPoints.size() >= 4) {
        m_isCalibrating = false;
        statusBar()->showMessage("校正完成（4 點）");
    }
}

void timeLine::loadFile()
{
    QString csv = QFileDialog::getOpenFileName(this, "選擇 CSV", "", "*.csv");
    if (!csv.isEmpty()) {
        QFile f(csv);
        if (f.open(QIODevice::ReadOnly)) {
            QTextStream in(&f);
            m_dataPoints.clear();
            while (!in.atEnd()) {
                auto s = in.readLine().split(",");
                if (s.size() >= 3) {
                    DataPoint d;
                    d.time = s[0].toDouble();
                    d.x = s[1].toDouble();
                    d.y = s[2].toDouble();
                    m_dataPoints.append(d);
                }
            }
        }
    }

    QString video = QFileDialog::getOpenFileName(this, "選擇影片", "", "*.mp4 *.avi");
    if (!video.isEmpty()) {
        m_player->setSource(QUrl::fromLocalFile(video));
        m_player->play();
    }
}
