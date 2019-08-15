#-------------------------------------------------
#
# Project created by QtCreator 2017-08-17T13:11:44
#
#-------------------------------------------------

QT += core gui widgets sql

TARGET = procyon
TEMPLATE = app
DESTDIR = $$_PRO_FILE_PWD_/bin

ORION = $$_PRO_FILE_PWD_/orion/
include($$ORION"orion.pri")

# hunspell
INCLUDEPATH += $$_PRO_FILE_PWD_/hunspell-1.7.0/src
win32: LIBS += -L$$_PRO_FILE_PWD_/hunspell-1.7.0/src/hunspell/.libs -lhunspell-1.7-0
else: LIBS += $$_PRO_FILE_PWD_/hunspell-1.7.0/src/hunspell/.libs/libhunspell-1.7.a

# Version information
include(release/version.pri)

# The following define makes your compiler emit warnings if you use
# any feature of Qt which as been marked as deprecated (the exact warnings
# depend on your compiler). Please consult the documentation of the
# deprecated API in order to know how to port your code away from it.
DEFINES += QT_DEPRECATED_WARNINGS

RESOURCES += src/resources.qrc

win32: RC_FILE = src/app.rc

SOURCES += src/main.cpp\
    src/AppSettings.cpp \
    src/MainWindow.cpp \
    src/CatalogWidget.cpp \
    src/Spellchecker.cpp \
    src/TextEditorHelpers.cpp \
    src/catalog/Catalog.cpp \
    src/catalog/CatalogStore.cpp \
    src/catalog/SqlHelper.cpp \
    src/catalog/Memo.cpp \
    src/highlighter/HighlightingRule.cpp \
    src/highlighter/PythonSyntaxHighlighter.cpp \
    src/highlighter/ShellMemoSyntaxHighlighter.cpp \
    src/CatalogModel.cpp \
    src/OpenedPagesWidget.cpp \
    src/pages/MemoEditor.cpp \
    src/pages/MemoPage.cpp \
    src/pages/PageWidgets.cpp \
    src/pages/StyleEditorPage.cpp

HEADERS  += src/MainWindow.h \
    src/AppSettings.h \
    src/CatalogWidget.h \
    src/CatalogModel.h \
    src/Spellchecker.h \
    src/TextEditorHelpers.h \
    src/catalog/Catalog.h \
    src/catalog/CatalogStore.h \
    src/catalog/SqlHelper.h \
    src/catalog/Memo.h \
    src/highlighter/HighlightingRule.h \
    src/highlighter/PythonSyntaxHighlighter.h \
    src/highlighter/ShellMemoSyntaxHighlighter.h \
    src/OpenedPagesWidget.h \
    src/pages/MemoEditor.h \
    src/pages/MemoPage.h \
    src/pages/PageWidgets.h \
    src/pages/StyleEditorPage.h

DISTFILES += \
    src/app.qss
