#pragma once
#include <QMainWindow>

QT_BEGIN_NAMESPACE
class QMediaPlayer;
class QAudioOutput;
class QVideoWidget;
class QPushButton;
class QSlider;
class QLabel;
class QFrame;
class QGroupBox;
QT_END_NAMESPACE

class Image_Processor : public QMainWindow
{
    Q_OBJECT
public:
    explicit Image_Processor(QWidget *parent = nullptr);
    ~Image_Processor();

private slots:
    void onImportVideo();
    void onPlayPause();
    void onRecordPlayback();
    void onSliderMoved(int value);

    void onDurationChanged(qint64 dur);
    void onPositionChanged(qint64 pos);

private:
    void buildUpperUI();

private:
    QMediaPlayer *player = nullptr;
    QAudioOutput *audio  = nullptr;
    QVideoWidget *videoWidget = nullptr;

    QGroupBox *groupVideo = nullptr;
    QGroupBox *groupTimeline = nullptr;

    QFrame *videoFrame = nullptr;
    QFrame *timelineFrame = nullptr;

    QPushButton *btnImport = nullptr;
    QPushButton *btnPlayPause = nullptr;
    QPushButton *btnRecordPlayback = nullptr;

    QSlider *slider = nullptr;
    QLabel  *timeLabel = nullptr;

    qint64 durationMs = 0;
    bool isPlaying = false;
};
