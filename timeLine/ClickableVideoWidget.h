#ifndef CLICKABLEVIDEOWIDGET_H
#define CLICKABLEVIDEOWIDGET_H

#include <QVideoWidget>
#include <QMouseEvent>

/**
 * @brief ClickableVideoWidget
 * 可點擊的 QVideoWidget 子類別
 * 提供滑鼠點擊事件，並發送相對於 Widget 的座標
 */
class ClickableVideoWidget : public QVideoWidget {
    Q_OBJECT
public:
    /**
     * @brief Constructor
     * @param parent 父 QWidget，預設為 nullptr
     *
     * 初始化 Widget 並啟用滑鼠追蹤
     */
    explicit ClickableVideoWidget(QWidget *parent = nullptr)
        : QVideoWidget(parent)
    {
        // 啟用滑鼠追蹤，即使沒有按鍵也能追蹤滑鼠位置
        setMouseTracking(true);
    }

signals:
    /**
     * @brief clicked 信號
     * @param pos 點擊位置，相對於 Widget 的座標
     */
    void clicked(const QPoint &pos);

protected:
    /**
     * @brief mousePressEvent
     * @param event QMouseEvent 指標事件
     *
     * 當滑鼠在 Widget 上按下時發送 clicked 信號
     */
    void mousePressEvent(QMouseEvent *event) override {
        // 發送相對於 Widget 自身的座標
        emit clicked(event->pos());
    }
};

#endif // CLICKABLEVIDEOWIDGET_H
