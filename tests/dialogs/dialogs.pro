QT += core gui widgets testlib

TARGET   = dialogstest
TEMPLATE = app
CONFIG  += console testcase c++14
CONFIG  -= app_bundle

INCLUDEPATH += ../../src

SOURCES += \
    tst_dialogs.cpp \
    ../../src/imagedialog.cpp \
    ../../src/linkdialog.cpp \
    ../../src/markdownsnippets.cpp \
    ../../src/tabledialog.cpp

HEADERS += \
    ../../src/imagedialog.h \
    ../../src/linkdialog.h \
    ../../src/markdownsnippets.h \
    ../../src/tabledialog.h

FORMS += \
    ../../ui/linkdialog.ui \
    ../../ui/imagedialog.ui \
    ../../ui/tabledialog.ui
