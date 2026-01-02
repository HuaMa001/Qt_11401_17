#ifndef TIMELINE_H
#define TIMELINE_H

#include <QMainWindow>
#include <QMediaPlayer>
#include <QAudioOutput>
#include <QSlider>
#include <QVector>
#include <QPointF>

#include "ClickableVideoWidget.h"
#include "VisualMap.h"

// =========================
// 資料結構
// =========================
struct DataPoint {
    double time;
    double x;
    double y;
};

struct CalibrationPoint {
    QPoint  videoPos;   // 影片像素
    QPointF worldPos;   // 世界座標
};

class timeLine : public QMainWindow
{
    Q_OBJECT

public:
    explicit timeLine(QWidget *parent = nullptr);

private slots:
    void loadFile();
    void onPositionChanged(qint64 position);
    void onDurationChanged(qint64 duration);
    void onVideoClicked(const QPoint &pos);

private:
    // 🎥 Media
    QMediaPlayer *m_player;
    QAudioOutput *m_audioOutput;
    ClickableVideoWidget *m_videoWidget;

    // 🗺️ Map
    VisualMap *m_visualMap;

    // ⏱️ Timeline
    QSlider *m_timeSlider;
    bool m_isUserSeeking = false;

    // 📍 Calibration
    bool m_isCalibrating = false;
    QVector<CalibrationPoint> m_calibrationPoints;

    // 📊 Data
    QVector<DataPoint> m_dataPoints;
};

#endif
