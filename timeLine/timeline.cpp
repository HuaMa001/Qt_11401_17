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
#include <QImageReader>
#include <QGraphicsBlurEffect>
#include <opencv2/opencv.hpp>
#include <QMessageBox>
#include <QDir>
#include <QDateTime>

/**
 * @brief timeLine Constructor
 * @param parent 父級 QWidget
 * 初始化 UI、播放器、滑桿、按鈕以及信號槽
 */
timeLine::timeLine(QWidget *parent)
    : QMainWindow(parent),
    m_currentScale(0.6)
{
    setWindowTitle("自動裁切系統");
    resize(1100, 900);

    // --- 狀態列 ---
    setStatusBar(new QStatusBar(this));

    // --- 全域樣式 ---
    setStyleSheet(R"(
        QMainWindow { background:#121212; }
        QWidget { color:#e6e6e6; font-family:"Microsoft JhengHei"; }
        QFrame#Card { background:#1e1e1e; border:1px solid #2a2a2a; border-radius:12px; }
        QLabel#Title { font-size:16px; font-weight:bold; color:#00bcd4; }
        QPushButton { background:#2a2a2a; border:1px solid #3a3a3a; border-radius:6px; padding:8px; min-width: 80px; }
        QPushButton:hover { border:1px solid #00bcd4; }
        QSlider::handle:horizontal { background:#00bcd4; width:14px; margin:-5px 0; border-radius:7px; }
    )");

    // --- 媒體播放器初始化 ---
    m_player = new QMediaPlayer(this);
    m_audioOutput = new QAudioOutput(this);
    m_player->setAudioOutput(m_audioOutput);

    // --- 主 Layout ---
    QWidget *central = new QWidget(this);
    QVBoxLayout *mainLayout = new QVBoxLayout(central);
    mainLayout->setContentsMargins(16, 16, 16, 16);
    mainLayout->setSpacing(12);

    // -------------------------
    // 🎥 影片預覽區
    // -------------------------
    QFrame *videoCard = new QFrame;
    videoCard->setObjectName("Card");
    QVBoxLayout *videoLayout = new QVBoxLayout(videoCard);

    QLabel *videoTitle = new QLabel("影像預覽");
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

    // -------------------------
    // ⏱️ 時間軸區
    // -------------------------
    QFrame *timeCard = new QFrame;
    timeCard->setObjectName("Card");
    QVBoxLayout *timeLayout = new QVBoxLayout(timeCard);

    m_timeSlider = new QSlider(Qt::Horizontal);
    timeLayout->addWidget(new QLabel("時間軸", timeCard));
    timeLayout->addWidget(m_timeSlider);
    mainLayout->addWidget(timeCard);

    // -------------------------
    // 🔽 底部控制區
    // -------------------------
    QWidget *bottomWidget = new QWidget;
    QHBoxLayout *bottomLayout = new QHBoxLayout(bottomWidget);

    m_visualMap = new VisualMap(this);

    QFrame *controlCard = new QFrame;
    controlCard->setObjectName("Card");
    QVBoxLayout *controlLayout = new QVBoxLayout(controlCard);

    QPushButton *btnLoadCSV = new QPushButton("📂 讀取存檔");
    QPushButton *btnExport  = new QPushButton("💾 輸出校正影片");
    m_btnPlayPause          = new QPushButton("⏸️ 暫停");
    QPushButton *btnLoad    = new QPushButton("🔍️ 追蹤");

    QLabel *lblScale   = new QLabel("縮放比例:");
    m_sliderScale      = new QSlider(Qt::Horizontal);
    m_sliderScale->setRange(50, 150); // 對應 0.5x ~ 1.5x
    m_sliderScale->setValue(100);     // 預設 1.0x

    // 控制按鈕加入布局
    controlLayout->addStretch();
    controlLayout->addWidget(btnLoadCSV);
    controlLayout->addWidget(m_btnPlayPause);
    controlLayout->addWidget(btnLoad);
    controlLayout->addWidget(lblScale);
    controlLayout->addWidget(m_sliderScale);
    controlLayout->addWidget(btnExport);

    // 加入底部 layout
    bottomLayout->addWidget(m_visualMap, 3);
    bottomLayout->addWidget(controlCard, 2);
    mainLayout->addWidget(bottomWidget, 3);

    setCentralWidget(central);

    // -------------------------
    // 連接信號槽
    // -------------------------
    connect(m_sliderScale, &QSlider::valueChanged, this, &timeLine::applyManualAdjust);
    connect(btnLoad, &QPushButton::clicked, this, &timeLine::loadFile);
    connect(btnLoadCSV, &QPushButton::clicked, this, &timeLine::loadFileAndCSV);
    connect(m_btnPlayPause, &QPushButton::clicked, this, &timeLine::togglePlayPause);
    connect(m_player, &QMediaPlayer::positionChanged, this, &timeLine::onPositionChanged);
    connect(m_player, &QMediaPlayer::durationChanged, this, &timeLine::onDurationChanged);
    connect(m_timeSlider, &QSlider::sliderMoved, m_player, &QMediaPlayer::setPosition);
    connect(btnExport, &QPushButton::clicked, this, &timeLine::exportCorrectedVideo);
}

// -------------------------
// 播放 / 暫停
// -------------------------
void timeLine::togglePlayPause() {
    if (m_player->playbackState() == QMediaPlayer::PlayingState) {
        m_player->pause();
        m_btnPlayPause->setText("▶️ 播放");
    } else {
        m_player->play();
        m_btnPlayPause->setText("⏸️ 暫停");
    }
}

// -------------------------
// 選影片並自動追蹤 (Python 追蹤腳本)
// -------------------------
void timeLine::loadFile() {
    // 1️⃣ 選影片
    QString video = QFileDialog::getOpenFileName(this, "選擇影片", "", "*.mp4 *.avi");
    if (video.isEmpty()) return;

    // 2️⃣ 設定影片來源
    m_player->setSource(QUrl::fromLocalFile(video));

    // 3️⃣ Python 路徑與 CSV
    QString pythonExe  = "C:/Users/User/anaconda3/envs/Qt_11401_17/python.exe";
    QString scriptPath = "../track/track.py";
    QString csvPath    = "./output.csv";

    if (!QFile::exists(scriptPath)) {
        qDebug() << "❌ track.py 不存在，終止執行";
        return;
    }

    // 4️⃣ 進度條顯示
    QProgressDialog *progress = new QProgressDialog(
        "⏳ 影片追蹤中，請稍候...", nullptr, 0, 0, this);
    progress->setWindowTitle("影片追蹤中");
    progress->setCancelButton(nullptr);
    progress->setWindowModality(Qt::ApplicationModal);
    progress->setMinimumDuration(300);
    progress->setStyleSheet(
        "QProgressDialog { color: black; }"
        "QLabel { color: black; }"
        );
    progress->show();

    // 5️⃣ 啟動 Python Process
    QProcess *proc = new QProcess(this);
    QStringList args;
    args << scriptPath << "--input" << video << "--output" << csvPath;
    proc->setProgram(pythonExe);
    proc->setArguments(args);
    proc->setProcessChannelMode(QProcess::MergedChannels);

    // 6️⃣ 讀 Python 輸出
    connect(proc, &QProcess::readyRead, this, [=](){
        qDebug() << proc->readAll();
    });

    // 7️⃣ 錯誤處理
    connect(proc, &QProcess::errorOccurred, this, [=](QProcess::ProcessError e){
        qDebug() << "QProcess error:" << e;
        progress->close();
        progress->deleteLater();
    });

    // 8️⃣ Python 完成時
    connect(proc, &QProcess::finished, this, [=](int, QProcess::ExitStatus status) {
        progress->close();
        progress->deleteLater();

        if (status == QProcess::NormalExit && QFile::exists(csvPath)) {
            QString timestamp = QDateTime::currentDateTime().toString("yyyyMMdd-HHmmss");
            QString saveRoot = QDir::currentPath() + "/save/" + timestamp;
            QDir().mkpath(saveRoot);
            m_saveFolder = saveRoot;

            QFile::copy(m_player->source().toLocalFile(),
                        m_saveFolder + "/" + QFileInfo(m_player->source().toLocalFile()).fileName());
            QFile::copy(csvPath, m_saveFolder + "/tracking.csv");

            loadCSV(csvPath);
            m_player->setPosition(m_startTime * 1000);
            QTimer::singleShot(300, this, &timeLine::applyAutoZoom);
            m_player->play();
            m_btnPlayPause->setText("⏸️ 暫停");

        } else {
            qDebug() << "❌ Python crash 或 CSV 不存在";
        }

        proc->deleteLater();
    });

    // 啟動 Python
    proc->start();
    if (!proc->waitForStarted()) {
        qDebug() << "❌ Process 沒有成功啟動";
        progress->close();
        progress->deleteLater();
    }
}
// -------------------------
// 選影片 + CSV（已有追蹤結果）
// -------------------------
void timeLine::loadFileAndCSV()
{
    // 1️⃣ 選影片
    QString video = QFileDialog::getOpenFileName(this, "選擇影片", "./save", "*.mp4 *.avi");
    if (video.isEmpty()) return;

    // 2️⃣ 選 CSV
    QString csvFile = QFileDialog::getOpenFileName(this, "選擇 CSV", "", "*.csv");
    if (csvFile.isEmpty()) return;

    // 3️⃣ 設定影片來源
    m_player->setSource(QUrl::fromLocalFile(video));

    // 4️⃣ 讀 CSV
    loadCSV(csvFile);

    // 5️⃣ 自動播放影片
    m_player->setPosition(m_startTime * 1000);
    QTimer::singleShot(300, this, &timeLine::applyAutoZoom);
    m_player->play();
    m_btnPlayPause->setText("⏸️ 暫停");
}

// -------------------------
// 讀 CSV，更新 m_dataPoints
// -------------------------
void timeLine::loadCSV(const QString &csvFile)
{
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
        m_endTime   = m_dataPoints.last().time;
        m_timeSlider->setRange(m_startTime * 1000, m_endTime * 1000);
    }

    // 自動播放影片
    if (!m_player->source().isEmpty()) {
        m_player->setPosition(m_startTime * 1000);
        QTimer::singleShot(300, this, &timeLine::applyAutoZoom);
        m_player->play();
        m_btnPlayPause->setText("⏸️ 暫停");
    }
}

// -------------------------
// 自動縮放人物，讓視窗顯示完整影片
// -------------------------
void timeLine::applyAutoZoom()
{
    if (!m_videoContainer || !m_videoWidget) return;

    QScrollArea *sa = qobject_cast<QScrollArea*>(m_videoContainer);
    if (!sa) return;

    // 實際可見尺寸
    m_camW = sa->width();
    m_camH = sa->height();

    QWidget *container = sa->widget();
    container->setFixedSize(m_camW, m_camH);

    // 設定 VideoWidget 大小
    m_videoWidget->setParent(container);
    m_videoWidget->setFixedSize(static_cast<int>(1920 * m_currentScale),
                                static_cast<int>(1080 * m_currentScale));

    QApplication::processEvents();

    // 立即更新位置
    QTimer::singleShot(0, this, [=]() {
        onPositionChanged(m_player->position());
    });
}

// -------------------------
// 播放位置改變時呼叫
// -------------------------
void timeLine::onPositionChanged(qint64 position)
{
    double sec = position / 1000.0;
    m_timeSlider->setValue(position);

    if (m_dataPoints.isEmpty()) return;

    // 找到當前時間對應座標
    const auto &pt = *std::lower_bound(
        m_dataPoints.begin(), m_dataPoints.end(), sec,
        [](const DataPoint &d, double t) { return d.time < t; });

    // 更新可視化地圖
    m_visualMap->updatePosition(pt.x, pt.y);

    QScrollArea *sa = qobject_cast<QScrollArea*>(m_videoContainer);
    if (!sa) return;

    // 總縮放率
    double totalScale = m_currentScale * m_manualScale;

    // 人物在縮放後影片中的像素
    double personX = pt.x * totalScale;
    double personY = pt.y * totalScale;

    // 計算影片左上角位置，使人物居中
    double camX = personX - sa->width() / 2.0;
    double camY = personY - sa->height() / 2.0;

    // 移動 VideoWidget
    m_videoWidget->move(-camX, -camY);
}

// -------------------------
// 輸出校正影片
// -------------------------
void timeLine::exportCorrectedVideo()
{
    if (m_player->source().isEmpty() || m_dataPoints.isEmpty()) {
        QMessageBox::warning(this, "錯誤", "請先載入影片和 CSV！");
        return;
    }

    QString inputFile = m_player->source().toLocalFile();
    QString saveFile  = QFileDialog::getSaveFileName(this, "儲存校正影片", "", "*.avi");
    if (saveFile.isEmpty()) return;

    cv::VideoCapture cap(inputFile.toStdString());
    if (!cap.isOpened()) {
        QMessageBox::critical(this, "錯誤", "無法開啟影片！");
        return;
    }

    int width       = static_cast<int>(cap.get(cv::CAP_PROP_FRAME_WIDTH));
    int height      = static_cast<int>(cap.get(cv::CAP_PROP_FRAME_HEIGHT));
    double fps      = cap.get(cv::CAP_PROP_FPS);
    int totalFrames = static_cast<int>(cap.get(cv::CAP_PROP_FRAME_COUNT));

    cv::VideoWriter writer(saveFile.toStdString(),
                           cv::VideoWriter::fourcc('M','J','P','G'),
                           fps, cv::Size(width, height));

    if (!writer.isOpened()) {
        QMessageBox::critical(this, "錯誤", "無法初始化輸出！");
        return;
    }

    // 進度對話框
    QProgressDialog progress("影片輸出中...", "取消", 0, totalFrames, this);
    progress.setWindowTitle("正在處理");

    progress.setWindowModality(Qt::ApplicationModal);
    progress.setMinimumDuration(0);
    progress.setStyleSheet(
        "QProgressDialog { color: black; }"
        "QLabel { color: black; }"
        );
    progress.show();

    double totalScale = m_currentScale * m_manualScale;
    double roiW = m_camW / totalScale;
    double roiH = m_camH / totalScale;

    cv::Mat frame;
    int frameIdx = 0;

    while (cap.read(frame)) {
        if (progress.wasCanceled()) break;
        progress.setValue(frameIdx);
        QApplication::processEvents();

        double sec = frameIdx / fps;
        auto it = std::lower_bound(m_dataPoints.begin(), m_dataPoints.end(), sec,
                                   [](const DataPoint &d, double t){ return d.time < t; });
        DataPoint pt = (it == m_dataPoints.end()) ? m_dataPoints.back() : *it;

        int x1 = static_cast<int>(pt.x - roiW / 2.0);
        int y1 = static_cast<int>(pt.y - roiH / 2.0);

        cv::Mat cropped(static_cast<int>(roiH), static_cast<int>(roiW), frame.type(), cv::Scalar(0,0,0));

        int srcX1 = std::max(0, x1);
        int srcY1 = std::max(0, y1);
        int srcX2 = std::min(width, static_cast<int>(x1 + roiW));
        int srcY2 = std::min(height, static_cast<int>(y1 + roiH));
        int dstX  = (x1 < 0) ? -x1 : 0;
        int dstY  = (y1 < 0) ? -y1 : 0;

        if (srcX2 > srcX1 && srcY2 > srcY1) {
            frame(cv::Rect(srcX1, srcY1, srcX2 - srcX1, srcY2 - srcY1))
            .copyTo(cropped(cv::Rect(dstX, dstY, srcX2 - srcX1, srcY2 - srcY1)));
        }

        cv::Mat outFrame;
        cv::resize(cropped, outFrame, cv::Size(width, height));
        writer.write(outFrame);

        frameIdx++;
    }

    writer.release();
    cap.release();
    progress.setValue(totalFrames);

    if (frameIdx >= totalFrames && !progress.wasCanceled()) {
        QMessageBox::information(this, "完成", "影片校正輸出完成！");
    } else if (progress.wasCanceled()) {
        QMessageBox::warning(this, "已取消", "輸出任務已手動停止。");
    }
}

// -------------------------
// 手動縮放調整
// -------------------------
void timeLine::applyManualAdjust()
{
    m_manualScale = m_sliderScale->value() / 100.0;

    if (!m_videoContainer || !m_videoWidget) return;

    double totalScale = m_currentScale * m_manualScale;
    int videoW = static_cast<int>(1920 * totalScale);
    int videoH = static_cast<int>(1080 * totalScale);

    m_videoWidget->setFixedSize(videoW, videoH);

    // 立即重新計算人物位置
    onPositionChanged(m_player->position());
}

// -------------------------
// 時間軸總長度變動 (未使用)
// -------------------------
void timeLine::onDurationChanged(qint64 duration) {
    Q_UNUSED(duration);
}

