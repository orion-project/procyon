#-------------------------------------------------
#
# Project created by QtCreator 2017-08-17T13:11:44
#
#-------------------------------------------------

QT += core gui widgets sql printsupport core5compat

# core5compat is only needed for QTextCodec
# which is needed for opening LibreOffice dictionaries for hunspell
# which are in strange encodings sometimes (e.g. KOI8-R)
greaterThan(QT_MAJOR_VERSION,5): QT += core5compat

TARGET = procyon
TEMPLATE = app
DESTDIR = $$_PRO_FILE_PWD_/bin

#-------------------------------------------------
#                      Deps

# orion
include($$_PRO_FILE_PWD_/orion/orion.pri)

# hunspell
exists($$_PRO_FILE_PWD_/deps/hunspell) {
    include($$_PRO_FILE_PWD_/deps/hunspell.pri)
    DEFINES += ENABLE_SPELLCHECK
}

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

RESOURCES += resources.qrc

win32: RC_FILE = src/app.rc
macx: ICON = img/icon/main.icns

SOURCES += src/main.cpp\
    src/AppSettings.cpp \
    src/AppTheme.cpp \
    src/highlighter/EnotStorage.cpp \
    src/highlighter/OriHighlighter.cpp \
    src/pages/AppSettingsPage.cpp \
    src/pages/CssEditorPage.cpp \
    src/pages/PhlEditorPage.cpp \
    src/pages/QssEditorPage.cpp \
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
    src/widgets/CodeTextEdit.cpp \
    src/widgets/MemoTextBrowser.cpp \
    src/widgets/MemoTextEdit.cpp \
    src/markdown/ori_html.c \
    src/CatalogModel.cpp \
    src/OpenedPagesWidget.cpp \
    src/pages/HelpPage.cpp \
    src/pages/MemoPage.cpp \
    src/pages/PageWidgets.cpp \
    src/pages/SqlConsolePage.cpp \
    src/spellcheck/TextEditSpellcheck.cpp \
    src/widgets/PopupMessage.cpp

HEADERS  += src/MainWindow.h \
    src/AppSettings.h \
    src/AppTheme.h \
    src/CatalogWidget.h \
    src/CatalogModel.h \
    src/highlighter/EnotStorage.h \
    src/highlighter/OriHighlighter.h \
    src/pages/AppSettingsPage.h \
    src/pages/CssEditorPage.h \
    src/pages/PhlEditorPage.h \
    src/pages/QssEditorPage.h \
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
    src/widgets/CodeTextEdit.h \
    src/widgets/MemoTextBrowser.h \
    src/widgets/MemoTextEdit.h \
    src/markdown/ori_html.h \
    src/OpenedPagesWidget.h \
    src/pages/HelpPage.h \
    src/pages/MemoPage.h \
    src/pages/PageWidgets.h \
    src/pages/SqlConsolePage.h \
    src/spellcheck/TextEditSpellcheck.h \
    src/widgets/PopupMessage.h

DISTFILES += \
    src/app.qss \
    src/markdown/markdown.css
