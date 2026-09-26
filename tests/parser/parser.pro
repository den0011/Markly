QT += core testlib
QT -= gui

TARGET   = parsertest
TEMPLATE = app
CONFIG  += console testcase c++14
CONFIG  -= app_bundle

INCLUDEPATH += ../../src

SOURCES += \
    tst_markdownparser.cpp \
    ../../src/markdownparser.cpp

HEADERS += \
    ../../src/markdownparser.h
