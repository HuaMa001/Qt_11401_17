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
#include <QProcess>
#include <QProgressDialog>

timeLine::timeLine(QWidget *parent)
    : QMainWindow(parent),
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
    scrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    scrollArea->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    scrollArea->setStyleSheet("background: #000000; border-radius: 8px;");
    scrollArea->setAlignment(Qt::AlignLeft | Qt::AlignTop);

    m_videoWidget = new ClickableVideoWidget();
    m_player->setVideoOutput(m_videoWidget);
    m_videoWidget->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);

    QWidget *container = new QWidget();
    container->setStyleSheet("background: transparent;");
    QVBoxLayout *containerLayout = new QVBoxLayout(container);

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
    QPushButton *btnLoad = new QPushButton("📂 選取影片並追蹤");
    QPushButton *btnLoadCSV = new QPushButton("📂 讀取影片和路徑檔"); // 新增按鈕

    controlLayout->addWidget(m_btnPlayPause);
    controlLayout->addWidget(btnLoad);
    controlLayout->addWidget(btnLoadCSV); // 加入布局
    controlLayout->addStretch();

    bottomLayout->addWidget(m_visualMap, 3);
    bottomLayout->addWidget(controlCard, 2);
    mainLayout->addWidget(bottomWidget, 3);

    setCentralWidget(central);

    // --- 連接信號槽 ---
    connect(btnLoad, &QPushButton::clicked, this, &timeLine::loadFile);
    connect(btnLoadCSV, &QPushButton::clicked, this, &timeLine::loadFileAndCSV); // 新增槽
    connect(m_btnPlayPause, &QPushButton::clicked, this, &timeLine::togglePlayPause);
    connect(m_player, &QMediaPlayer::positionChanged, this, &timeLine::onPositionChanged);
    connect(m_player, &QMediaPlayer::durationChanged, this, &timeLine::onDurationChanged);
    connect(m_timeSlider, &QSlider::sliderMoved, m_player, &QMediaPlayer::setPosition);

}

// --- 播放 / 暫停 ---
void timeLine::togglePlayPause() {
    if (m_player->playbackState() == QMediaPlayer::PlayingState) {
        m_player->pause();
        m_btnPlayPause->setText("▶️ 播放");
    } else {
        m_player->play();
        m_btnPlayPause->setText("⏸️ 暫停");
    }
}

// --- 選影片並自動追蹤 ---
// --- 選影片並自動追蹤 ---
void timeLine::loadFile()
{
    // --- 1. 選影片 ---
    QString video = QFileDialog::getOpenFileName(
        this, "選擇影片", "", "*.mp4 *.avi");
    if (video.isEmpty()) return;

    // --- 2. 設定影片來源，方便播放 ---
    m_player->setSource(QUrl::fromLocalFile(video));

    // --- 3. Python 路徑 ---
    QString pythonExe = "C:/Users/User/anaconda3/envs/Qt_11401_17/python.exe";
    QString scriptPath = "../track/track.py";
    QString csvPath    = "./output.csv";


    qDebug() << "scriptPath:" << scriptPath;
    qDebug() << "script exists?" << QFile::exists(scriptPath);

    if (!QFile::exists(scriptPath)) {
        qDebug() << "❌ track.py 不存在，終止執行";
        return;
    }

    // --- 4. 等待小視窗 ---
    QProgressDialog *progress = new QProgressDialog(
        "⏳ 影片追蹤中，請稍候...", nullptr, 0, 0, this);
    progress->setWindowTitle("影片追蹤中");
    progress->setCancelButton(nullptr);
    progress->setWindowModality(Qt::ApplicationModal);
    progress->setMinimumDuration(300);   // ❗ 避免閃一下
    progress->setStyleSheet(
        "QProgressDialog { color: black; }"
        "QLabel { color: black; }"
        );
    progress->show();

    // --- 5. 啟動 QProcess ---
    QProcess *proc = new QProcess(this);
    QStringList args;
    args << scriptPath << "--input" << video << "--output" << csvPath;

    proc->setProgram(pythonExe);
    proc->setArguments(args);
    proc->setProcessChannelMode(QProcess::MergedChannels);

    // --- 6. 讀 Python 輸出 ---
    connect(proc, &QProcess::readyRead, this, [=](){
        qDebug() << proc->readAll();
    });

    // --- 7. 錯誤處理 ---
    connect(proc, &QProcess::errorOccurred, this, [=](QProcess::ProcessError e){
        qDebug() << "QProcess error:" << e;
        progress->close();
        progress->deleteLater();
    });

    // --- 8. Python 完成時 ---
    connect(proc, &QProcess::finished, this,
            [=](int exitCode, QProcess::ExitStatus status)
            {
                progress->close();
                progress->deleteLater();
                qDebug() << "Process finished:" << exitCode << status;

                if (status == QProcess::NormalExit && QFile::exists(csvPath)) {
                    // --- 9. 讀 CSV ---
                    loadCSV(csvPath);

                    // --- 10. 自動播放影片 ---
                    m_player->setPosition(m_startTime * 1000);
                    QTimer::singleShot(300, this, &timeLine::applyAutoZoom);
                    m_player->play();
                    m_btnPlayPause->setText("⏸️ 暫停");
                } else {
                    qDebug() << "❌ Python crash 或 CSV 不存在";
                }

                proc->deleteLater();
            });

    // --- 11. 啟動 Python ---
    proc->start();
    if (!proc->waitForStarted()) {
        qDebug() << "❌ Process 沒有成功啟動";
        progress->close();
        progress->deleteLater();
    }
}
void timeLine::loadFileAndCSV()
{
    // 1. 選影片
    QString video = QFileDialog::getOpenFileName(
        this, "選擇影片", "", "*.mp4 *.avi");
    if (video.isEmpty()) return;

    // 2. 選 CSV
    QString csvFile = QFileDialog::getOpenFileName(
        this, "選擇 CSV", "", "*.csv");
    if (csvFile.isEmpty()) return;

    // 3. 設定影片來源
    m_player->setSource(QUrl::fromLocalFile(video));

    // 4. 讀 CSV
    loadCSV(csvFile);

    // 5. 自動播放影片
    m_player->setPosition(m_startTime * 1000);
    QTimer::singleShot(300, this, &timeLine::applyAutoZoom);
    m_player->play();
    m_btnPlayPause->setText("⏸️ 暫停");
}


// --- 讀 CSV ---
void timeLine::loadCSV(const QString &csvFile) {
    QFile f(csvFile);
    if (!f.open(QIODevice::ReadOnly)) return;

    QTextStream in(&f);
    m_dataPoints.clear();
    while (!in.atEnd()) {
        auto s = in.readLine().split(",");
        if (s.size() >= 3) {
            DataPoint d = { s[0].toDouble(), s[1].toDouble(), s[2].toDouble() };
            m_dataPoints.append(d);
        }
    }
    if (!m_dataPoints.isEmpty()) {
        m_startTime = m_dataPoints.first().time;
        m_endTime = m_dataPoints.last().time;
        m_timeSlider->setRange(m_startTime * 1000, m_endTime * 1000);
    }

    // 開始播放影片
    if (!m_player->source().isEmpty()) {
        m_player->setPosition(m_startTime * 1000);
        QTimer::singleShot(300, this, &timeLine::applyAutoZoom);
        m_player->play();
        m_btnPlayPause->setText("⏸️ 暫停");
    }
}


// --- 自動縮放人物 ---
void timeLine::applyAutoZoom()
{
    if (!m_videoContainer || !m_videoWidget) return;

    QScrollArea *sa = qobject_cast<QScrollArea*>(m_videoContainer);
    if (!sa) return;

    // ✅ 用實際可見尺寸（不是 viewport）
    m_camW = sa->width();
    m_camH = sa->height();

    QWidget *container = sa->widget();
    container->setFixedSize(m_camW, m_camH);

    m_videoWidget->setParent(container);
    m_videoWidget->setFixedSize(
        1920 * m_currentScale,
        1080 * m_currentScale
        );

    QApplication::processEvents();

    QTimer::singleShot(0, this, [=]() {
        onPositionChanged(m_player->position());
    });
}



// --- 更新影片位置 & 滾動 ---
void timeLine::onPositionChanged(qint64 position)
{
    double sec = position / 1000.0;
    m_timeSlider->setValue(position);

    if (m_dataPoints.isEmpty()) return;

    const auto &pt = *std::lower_bound(
        m_dataPoints.begin(), m_dataPoints.end(), sec,
        [](const DataPoint &d, double t) { return d.time < t; });

    m_visualMap->updatePosition(pt.x, pt.y);

    QScrollArea *sa = qobject_cast<QScrollArea*>(m_videoContainer);
    if (!sa) return;

    int camW = m_camW;
    int camH = m_camH;

    // 人物在影片中的位置（世界座標）
    double personX = pt.x * m_currentScale;
    double personY = pt.y * m_currentScale;

    // 🎯 鏡頭永遠置中人物（不 clamp）
    double camX = personX - camW / 2.0;
    double camY = personY - camH / 2.0;

    // 🎥 移動影片，超出部分自然顯示黑邊
    m_videoWidget->move(-camX, -camY);
}


void timeLine::onDurationChanged(qint64 duration) {
    Q_UNUSED(duration);
}
