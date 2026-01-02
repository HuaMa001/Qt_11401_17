#ifndef VISUALMAP_H
#define VISUALMAP_H

#include <QWidget>
#include <QPainter>

class VisualMap : public QWidget {
    Q_OBJECT
public:
    explicit VisualMap(QWidget *parent = nullptr) : QWidget(parent) {
        m_currX = 0; m_currY = 0;
        setMinimumHeight(250);
        // 設定背景色
        QPalette pal = palette();
        pal.setColor(QPalette::Window, Qt::white);
        setPalette(pal);
        setAutoFillBackground(true);
    }

    void updatePosition(double x, double y) {
        m_currX = x;
        m_currY = y;
        update(); // 觸發重繪
    }

protected:
    void paintEvent(QPaintEvent *) override {
        QPainter painter(this);
        painter.setRenderHint(QPainter::Antialiasing);

        // 1. 計算保持 16:9 比例的繪製區域
        int padding = 30;
        int availableW = width() - 2 * padding;
        int availableH = height() - 2 * padding;

        int mapW = availableW;
        int mapH = mapW * 1080 / 1920;
        if (mapH > availableH) {
            mapH = availableH;
            mapW = mapH * 1920 / 1080;
        }

        QRect mapRect((width() - mapW) / 2, (height() - mapH) / 2, mapW, mapH);

        // 2. 繪製地圖外框與背景
        painter.setBrush(QColor(245, 245, 245));
        painter.setPen(QPen(Qt::darkGray, 2));
        painter.drawRect(mapRect);

        // 繪製中心十字輔助線 (灰色虛線)
        painter.setPen(QPen(QColor(210, 210, 210), 1, Qt::DashLine));
        painter.drawLine(mapRect.center().x(), mapRect.top(), mapRect.center().x(), mapRect.bottom());
        painter.drawLine(mapRect.left(), mapRect.center().y(), mapRect.right(), mapRect.center().y());

        // 3. 座標映射 (1920x1080 -> 畫布大小)
        // 防止座標超出邊界
        double normX = qBound(0.0, m_currX, 1920.0);
        double normY = qBound(0.0, m_currY, 1080.0);

        int drawX = mapRect.left() + (normX / 1920.0) * mapW;
        int drawY = mapRect.top() + (normY / 1080.0) * mapH;

        // 4. 繪製物體標記 (紅色十字準星)
        painter.setPen(QPen(Qt::red, 2));
        painter.drawLine(drawX - 12, drawY, drawX + 12, drawY);
        painter.drawLine(drawX, drawY - 12, drawX, drawY + 12);
        painter.setBrush(Qt::red);
        painter.drawEllipse(drawX - 4, drawY - 4, 8, 8);

        // 5. 繪製文字資訊
        painter.setPen(Qt::black);
        QFont font = painter.font();
        font.setBold(true);
        painter.setFont(font);
        painter.drawText(mapRect.left(), mapRect.top() - 10, "影片相對位址監控 (1920x1080)");

        painter.setPen(Qt::blue);
        painter.drawText(mapRect.left(), mapRect.bottom() + 20,
                         QString("當前座標: X=%1, Y=%2").arg((int)m_currX).arg((int)m_currY));
    }

private:
    double m_currX, m_currY;
};

#endif
