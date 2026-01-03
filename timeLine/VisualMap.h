#ifndef VISUALMAP_H
#define VISUALMAP_H

#include <QWidget>
#include <QPainter>

class VisualMap : public QWidget {
    Q_OBJECT
public:
    explicit VisualMap(QWidget *parent = nullptr) : QWidget(parent) {
        m_currX = 0;
        m_currY = 0;
        setMinimumHeight(250);
        QPalette pal = palette();
        pal.setColor(QPalette::Window, Qt::white);
        setPalette(pal);
        setAutoFillBackground(true);
    }

    void updatePosition(double x, double y) {
        m_currX = x;
        m_currY = y;
        update();
    }

protected:
    // 點擊校正功能已移除
    void mousePressEvent(QMouseEvent *event) override {
        Q_UNUSED(event);
        // 不做任何事
    }

    void paintEvent(QPaintEvent *) override {
        QPainter painter(this);
        painter.setRenderHint(QPainter::Antialiasing);

        int padding = 30;
        int mapW = width() - 2 * padding;
        int mapH = mapW * 1080 / 1920;
        if (mapH > height() - 2 * padding) {
            mapH = height() - 2 * padding;
            mapW = mapH * 1920 / 1080;
        }
        QRect mapRect((width() - mapW) / 2, (height() - mapH) / 2, mapW, mapH);

        // 背景與邊框
        painter.setBrush(QColor(245, 245, 245));
        painter.setPen(QPen(Qt::darkGray, 2));
        painter.drawRect(mapRect);

        // 中心虛線
        painter.setPen(QPen(QColor(210, 210, 210), 1, Qt::DashLine));
        painter.drawLine(mapRect.center().x(), mapRect.top(), mapRect.center().x(), mapRect.bottom());
        painter.drawLine(mapRect.left(), mapRect.center().y(), mapRect.right(), mapRect.center().y());

        // 畫紅色十字與圓點
        double normX = qBound(0.0, m_currX, 1920.0);
        double normY = qBound(0.0, m_currY, 1080.0);
        int drawX = mapRect.left() + (normX / 1920.0) * mapW;
        int drawY = mapRect.top() + (normY / 1080.0) * mapH;

        painter.setPen(QPen(Qt::red, 2));
        painter.drawLine(drawX - 12, drawY, drawX + 12, drawY);
        painter.drawLine(drawX, drawY - 12, drawX, drawY + 12);
        painter.setBrush(Qt::red);
        painter.drawEllipse(drawX - 4, drawY - 4, 8, 8);

        // 提示文字
        painter.setPen(Qt::black);
        painter.drawText(mapRect.left(), mapRect.top() - 10, "座標監控");
    }

private:
    double m_currX, m_currY;
};

#endif // VISUALMAP_H
