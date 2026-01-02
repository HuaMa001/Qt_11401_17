#ifndef CLICKABLEVIDEOWIDGET_H
#define CLICKABLEVIDEOWIDGET_H

#include <QVideoWidget>
#include <QMouseEvent>

class ClickableVideoWidget : public QVideoWidget {
    Q_OBJECT
public:
    explicit ClickableVideoWidget(QWidget *parent = nullptr) : QVideoWidget(parent) {
        // 確保 Widget 能夠追蹤滑鼠
        setMouseTracking(true);
    }

signals:
    void clicked(const QPoint &pos);

protected:
    void mousePressEvent(QMouseEvent *event) override {
        // 發送相對於 Widget 自身的座標
        emit clicked(event->pos());
    }
};
#endif
