#include "timeLine.h"

#include <QApplication>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    timeLine w;
    w.show();
    return a.exec();
}
