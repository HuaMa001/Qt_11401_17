#ifndef CLICKABLEVIDEOWIDGET_H
#define CLICKABLEVIDEOWIDGET_H

#include <QVideoWidget>
#include <QMouseEvent>

class ClickableVideoWidget : public QVideoWidget
{
    Q_OBJECT
public:
    explicit ClickableVideoWidget(QWidget *parent = nullptr)
        : QVideoWidget(parent) {}

signals:
    void clicked(const QPoint &pos);

protected:
    void mousePressEvent(QMouseEvent *event) override
    {
        emit clicked(event->pos());   // 影片內像素座標
    }
};

#endif
