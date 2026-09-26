QT += core gui widgets testlib

TARGET   = previewtest
TEMPLATE = app
CONFIG  += console testcase c++14
CONFIG  -= app_bundle

INCLUDEPATH += ../../src

SOURCES += \
    tst_previewtheme.cpp \
    ../../src/previewview.cpp

HEADERS += \
    ../../src/previewview.h

RESOURCES += ../../resources/markly.qrc
