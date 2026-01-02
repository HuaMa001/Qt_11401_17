#ifndef TIMELINE_H
#define TIMELINE_H

#include <QMainWindow>
#include <QMediaPlayer>
#include <QAudioOutput>
#include <QSlider>
#include <QVector>
#include <QPointF>
#include <QTimer>
#include <QPushButton>

#include "ClickableVideoWidget.h"
#include "VisualMap.h"

// 數據結構定義
struct DataPoint { double time; double x; double y; };
struct CalibrationPoint { QPoint videoPos; QPointF worldPos; };

class timeLine : public QMainWindow {
    Q_OBJECT
public:
    explicit timeLine(QWidget *parent = nullptr);

private slots:
    // 按鈕與播放邏輯
    void togglePlayPause();      // 切換播放與暫停
    void loadFile();            // 載入 CSV 與影片
    void applyAutoZoom();       // 執行畫面縮放與中心對齊

    // 播放器狀態同步
    void onPositionChanged(qint64 position);
    void onDurationChanged(qint64 duration); // 解決編譯錯誤的關鍵宣告

    // 滑鼠與顯示邏輯
    void onVideoClicked(const QPoint &pos);

private:
    // 多媒體核心
    QMediaPlayer *m_player;
    QAudioOutput *m_audioOutput;
    ClickableVideoWidget *m_videoWidget;
    QWidget *m_videoContainer;   // QScrollArea 指標，用於裁切與置中控制

    // UI 元件
    VisualMap *m_visualMap;
    QSlider *m_timeSlider;
    QPushButton *m_btnPlayPause; // 暫停/播放按鈕指標

    // 數據儲存
    QVector<DataPoint> m_dataPoints;
    double m_startTime = 0;
    double m_endTime = 0;

    // 座標轉換與縮放參數
    double m_currentScale = 0.6; // 縮放倍率 (與 cpp 同步)
    QPoint m_videoOffset = QPoint(0, 0);

    // 校正狀態
    bool m_isCalibrating = false;
    QVector<CalibrationPoint> m_calibrationPoints;
};

#endif // TIMELINE_H
