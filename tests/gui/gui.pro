QT += core gui widgets printsupport testlib

TARGET   = guitest
TEMPLATE = app
CONFIG  += console testcase c++14
CONFIG  -= app_bundle

INCLUDEPATH += ../../src

SOURCES += \
    tst_mainwindow.cpp \
    ../../src/actionicons.cpp \
    ../../src/editor.cpp \
    ../../src/finddialog.cpp \
    ../../src/imagedialog.cpp \
    ../../src/linkdialog.cpp \
    ../../src/markdownsnippets.cpp \
    ../../src/tabledialog.cpp \
    ../../src/mainwindow.cpp \
    ../../src/markdownhighlighter.cpp \
    ../../src/markdownparser.cpp \
    ../../src/previewview.cpp \
    ../../src/translationmanager.cpp

HEADERS += \
    ../../src/actionicons.h \
    ../../src/editor.h \
    ../../src/finddialog.h \
    ../../src/imagedialog.h \
    ../../src/linkdialog.h \
    ../../src/markdownsnippets.h \
    ../../src/tabledialog.h \
    ../../src/mainwindow.h \
    ../../src/markdownhighlighter.h \
    ../../src/markdownparser.h \
    ../../src/previewview.h \
    ../../src/translationmanager.h

FORMS += \
    ../../ui/mainwindow.ui \
    ../../ui/finddialog.ui \
    ../../ui/linkdialog.ui \
    ../../ui/imagedialog.ui \
    ../../ui/tabledialog.ui

RESOURCES += ../../resources/markly.qrc
