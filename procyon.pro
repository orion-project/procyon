#-------------------------------------------------
#
# Project created by QtCreator 2017-08-17T13:11:44
#
#-------------------------------------------------

QT += core gui widgets sql printsupport

TARGET = procyon
TEMPLATE = app
DESTDIR = $$_PRO_FILE_PWD_/bin

#-------------------------------------------------
#                      Deps

# orion
include($$_PRO_FILE_PWD_/orion/orion.pri)

# hunspell
INCLUDEPATH += $$_PRO_FILE_PWD_/deps/hunspell-1.7.0/src
win32: LIBS += -L$$_PRO_FILE_PWD_/deps/hunspell-1.7.0/src/hunspell/.libs -lhunspell-1.7-0
else: LIBS += $$_PRO_FILE_PWD_/deps/hunspell-1.7.0/src/hunspell/.libs/libhunspell-1.7.a

# hoedown
include($$_PRO_FILE_PWD_/deps/hoedown.pri)

#-------------------------------------------------

# Version information
include(release/version.pri)

# The following define makes your compiler emit warnings if you use
# any feature of Qt which as been marked as deprecated (the exact warnings
# depend on your compiler). Please consult the documentation of the
# deprecated API in order to know how to port your code away from it.
DEFINES += QT_DEPRECATED_WARNINGS

RESOURCES += src/resources.qrc

win32: RC_FILE = src/app.rc
macx: ICON = img/icon/main.icns

SOURCES += src/main.cpp\
    src/AppSettings.cpp \
    src/pages/AppSettingsPage.cpp \
    src/spellcheck/LangCodeAndNames.cpp \
    src/MainWindow.cpp \
    src/CatalogWidget.cpp \
    src/spellcheck/Spellchecker.cpp \
    src/TextEditHelpers.cpp \
    src/Utils.cpp \
    src/catalog/Catalog.cpp \
    src/catalog/CatalogStore.cpp \
    src/catalog/FolderManager.cpp \
    src/catalog/MemoManager.cpp \
    src/catalog/SettingsManager.cpp \
    src/catalog/SqlHelper.cpp \
    src/markdown/MarkdownHelper.cpp \
    src/editors/MarkdownMemoEditor.cpp \
    src/editors/MemoEditor.cpp \
    src/widgets/MemoTextBrowser.cpp \
    src/widgets/MemoTextEdit.cpp \
    src/editors/PlainTextMemoEditor.cpp \
    src/markdown/ori_html.c \
    src/highlighter/HighlighterControl.cpp \
    src/highlighter/HighlighterManager.cpp \
    src/highlighter/HighlightingRule.cpp \
    src/highlighter/ProcyonSyntaxHighlighter.cpp \
    src/highlighter/PythonSyntaxHighlighter.cpp \
    src/CatalogModel.cpp \
    src/OpenedPagesWidget.cpp \
    src/pages/HelpPage.cpp \
    src/pages/MarkdownCssEditorPage.cpp \
    src/pages/MemoPage.cpp \
    src/pages/PageWidgets.cpp \
    src/pages/SqlConsolePage.cpp \
    src/pages/StyleEditorPage.cpp \
    src/spellcheck/TextEditSpellcheck.cpp

HEADERS  += src/MainWindow.h \
    src/AppSettings.h \
    src/CatalogWidget.h \
    src/CatalogModel.h \
    src/pages/AppSettingsPage.h \
    src/spellcheck/Spellchecker.h \
    src/TextEditHelpers.h \
    src/Utils.h \
    src/catalog/Catalog.h \
    src/catalog/CatalogStore.h \
    src/catalog/FolderManager.h \
    src/catalog/MemoManager.h \
    src/catalog/SettingsManager.h \
    src/catalog/SqlHelper.h \
    src/markdown/MarkdownHelper.h \
    src/editors/MarkdownMemoEditor.h \
    src/editors/MemoEditor.h \
    src/widgets/MemoTextBrowser.h \
    src/widgets/MemoTextEdit.h \
    src/editors/PlainTextMemoEditor.h \
    src/markdown/ori_html.h \
    src/highlighter/HighlighterControl.h \
    src/highlighter/HighlighterManager.h \
    src/highlighter/HighlightingRule.h \
    src/highlighter/ProcyonSyntaxHighlighter.h \
    src/highlighter/PythonSyntaxHighlighter.h \
    src/OpenedPagesWidget.h \
    src/pages/HelpPage.h \
    src/pages/MarkdownCssEditorPage.h \
    src/pages/MemoPage.h \
    src/pages/PageWidgets.h \
    src/pages/SqlConsolePage.h \
    src/pages/StyleEditorPage.h \
    src/spellcheck/TextEditSpellcheck.h

DISTFILES += \
    src/app.qss \
    src/markdown/markdown.css
