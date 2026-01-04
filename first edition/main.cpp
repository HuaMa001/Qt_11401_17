#include "image_processor.h"
#include <QApplication>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    Image_Processor w;
    w.show();
    return a.exec();
}
