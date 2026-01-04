QT += core gui widgets multimedia multimediawidgets charts
greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

CONFIG += c++17

SOURCES += main.cpp \
           timeLine.cpp

HEADERS += ClickableVideoWidget.h \
           VisualMap.h \
           timeLine.h

# OpenCV Include
INCLUDEPATH += D:/package_for_C++/OpenCV-MinGW-Build-OpenCV-4.5.5-x64/include

# OpenCV Libraries (針對 MinGW 分散檔案版本)
win32 {
    LIBS += -LD:/package_for_C++/OpenCV-MinGW-Build-OpenCV-4.5.5-x64/x64/mingw/lib \
            -lopencv_core455 \
            -lopencv_highgui455 \
            -lopencv_imgcodecs455 \
            -lopencv_imgproc455 \
            -lopencv_video455 \
            -lopencv_videoio455 \
            -lopencv_objdetect455
}
