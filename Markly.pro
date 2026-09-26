QT += core gui widgets printsupport

TARGET   = Markly
TEMPLATE = app
CONFIG  += c++14

# Compiles TRANSLATIONS to .qm at build time and embeds them under :/i18n.
CONFIG  += lrelease embed_translations

TRANSLATIONS += translations/markly_ru.ts

DEFINES += QT_DEPRECATED_WARNINGS

INCLUDEPATH += src

SOURCES += \
    src/actionicons.cpp \
    src/finddialog.cpp \
    src/imagedialog.cpp \
    src/linkdialog.cpp \
    src/main.cpp \
    src/markdownsnippets.cpp \
    src/tabledialog.cpp \
    src/mainwindow.cpp \
    src/editor.cpp \
    src/markdownhighlighter.cpp \
    src/markdownparser.cpp \
    src/previewview.cpp \
    src/translationmanager.cpp

HEADERS += \
    src/actionicons.h \
    src/finddialog.h \
    src/imagedialog.h \
    src/linkdialog.h \
    src/markdownsnippets.h \
    src/tabledialog.h \
    src/mainwindow.h \
    src/editor.h \
    src/markdownhighlighter.h \
    src/markdownparser.h \
    src/previewview.h \
    src/translationmanager.h

FORMS += \
    ui/mainwindow.ui \
    ui/finddialog.ui \
    ui/linkdialog.ui \
    ui/imagedialog.ui \
    ui/tabledialog.ui

RESOURCES += resources/markly.qrc

# Windows takes the window and taskbar icon from the executable's resource, so
# this is all that is needed; regenerate it with tools/appicon.
win32:RC_ICONS = resources/markly.ico
