QT += core gui widgets svg testlib

TARGET   = imagestest
TEMPLATE = app
CONFIG  += console testcase c++14
CONFIG  -= app_bundle

INCLUDEPATH += ../../src

SOURCES += \
    tst_previewimages.cpp \
    ../../src/markdownparser.cpp \
    ../../src/previewview.cpp

HEADERS += \
    ../../src/markdownparser.h \
    ../../src/previewview.h

RESOURCES += ../../resources/markly.qrc
