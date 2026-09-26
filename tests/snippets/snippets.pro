QT += core testlib
QT -= gui

TARGET   = snippetstest
TEMPLATE = app
CONFIG  += console testcase c++14
CONFIG  -= app_bundle

INCLUDEPATH += ../../src

SOURCES += \
    tst_markdownsnippets.cpp \
    ../../src/markdownsnippets.cpp \
    ../../src/markdownparser.cpp

HEADERS += \
    ../../src/markdownsnippets.h \
    ../../src/markdownparser.h
