#ifndef TIMELINE_H
#define TIMELINE_H

#include <QMainWindow>
#include <QMediaPlayer>
#include <QAudioOutput>
#include <QSlider>
#include <QVector>
#include <QPushButton>
#include <QLabel>
#include <QScrollArea>
#include <opencv2/opencv.hpp>
#include "ClickableVideoWidget.h"
#include "VisualMap.h"

// -----------------------------
// 基礎數據結構
// -----------------------------
/**
 * @brief DataPoint
 * 單個數據點，包含時間與位置
 */
struct DataPoint {
    double time;  ///< 時間 (秒)
    double x;     ///< x 座標
    double y;     ///< y 座標
};

// -----------------------------
// timeLine 主視窗類別
// -----------------------------
class timeLine : public QMainWindow {
    Q_OBJECT

public:
    /**
     * @brief Constructor
     * @param parent 父級 QWidget
     */
    explicit timeLine(QWidget *parent = nullptr);

private slots:
    void togglePlayPause();                  ///< 播放或暫停影片
    void loadFile();                         ///< 執行 Python 追蹤腳本並載入影片
    void loadCSV(const QString &csvFile);    ///< 讀取 CSV 數據
    void loadFileAndCSV();                   ///< 直接讀取現有影片與 CSV
    void applyAutoZoom();                    ///< 自動初始化縮放參數
    void applyManualAdjust();                ///< 手動縮放滑桿更新
    void onPositionChanged(qint64 position);///< 播放位置變動，同步 UI
    void onDurationChanged(qint64 duration);///< 播放總長度變化
    void exportCorrectedVideo();             ///< 關鍵功能：輸出校正影片

private:
    // -----------------------------
    // 核心數學邏輯
    // -----------------------------
    /**
     * @brief RoiResult
     * 記錄裁切區域資訊
     */
    struct RoiResult {
        double x1, y1; ///< 左上角座標
        double w, h;   ///< 寬度與高度
    };

    /**
     * @brief 計算 ROI (Region of Interest)
     * @param centerX 中心 X 座標
     * @param centerY 中心 Y 座標
     * @return RoiResult 裁切區域資訊
     */
    RoiResult calculateROI(double centerX, double centerY);

    // -----------------------------
    // 多媒體與 UI 元件
    // -----------------------------
    QMediaPlayer *m_player;                 ///< 媒體播放器
    QAudioOutput *m_audioOutput;            ///< 音訊輸出
    ClickableVideoWidget *m_videoWidget;    ///< 可點擊的影片顯示區
    QWidget *m_videoContainer;              ///< 影片容器 Widget
    VisualMap *m_visualMap;                 ///< 可視化地圖 (追蹤顯示)
    QSlider *m_timeSlider;                  ///< 時間軸滑桿
    QSlider *m_sliderScale;                 ///< 縮放比例滑桿
    QPushButton *m_btnPlayPause;            ///< 播放/暫停按鈕

    // -----------------------------
    // 數據與參數
    // -----------------------------
    QVector<DataPoint> m_dataPoints;        ///< 影片追蹤數據點
    double m_startTime = 0;                 ///< 影片起始時間
    double m_endTime = 0;                   ///< 影片結束時間
    double m_currentScale = 0.6;            ///< 預設基礎縮放
    double m_manualScale = 1.0;             ///< 手動調整倍率
    int m_camW = 0, m_camH = 0;             ///< 預覽窗口尺寸
    QString m_saveFolder;                    ///< 校正影片輸出資料夾
};

#endif // TIMELINE_H
