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
    src/db/Db.cpp \
    src/db/FolderManager.cpp \
    src/db/MemoManager.cpp \
    src/db/SettingsManager.cpp \
    src/db/SqlHelper.cpp \
    src/CatalogModel.cpp \
    src/CatalogWidget.cpp \
    src/editors/MarkdownMemoEditor.cpp \
    src/editors/MemoEditor.cpp \
    src/highlighter/EnotStorage.cpp \
    src/highlighter/PhlManager.cpp \
    src/MainWindow.cpp \
    src/markdown/MarkdownHelper.cpp \
    src/markdown/ori_html.c \
    src/OpenTabsWidget.cpp \
    src/spellcheck/LangCodeAndNames.cpp \
    src/spellcheck/Spellchecker.cpp \
    src/spellcheck/TextEditSpellcheck.cpp \
    src/tabs/AppSettingsTab.cpp \
    src/tabs/CmdConsoleTab.cpp \
    src/tabs/CssEditorTab.cpp \
    src/tabs/HelpTab.cpp \
    src/tabs/MemoTab.cpp \
    src/tabs/PhlEditorTab.cpp \
    src/tabs/QssEditorTab.cpp \
    src/tabs/SqlConsoleTab.cpp \
    src/tabs/TabHelpers.cpp \
    src/TextEditHelpers.cpp \
    src/Utils.cpp \
    src/widgets/MemoTextBrowser.cpp \
    src/widgets/MemoTextEdit.cpp

HEADERS  += src/MainWindow.h \
    src/AppSettings.h \
    src/AppTheme.h \
    src/db/Db.h \
    src/db/FolderManager.h \
    src/db/MemoManager.h \
    src/db/SettingsManager.h \
    src/db/SqlHelper.h \
    src/CatalogModel.h \
    src/CatalogWidget.h \
    src/editors/MarkdownMemoEditor.h \
    src/editors/MemoEditor.h \
    src/highlighter/EnotStorage.h \
    src/highlighter/PhlManager.h \
    src/markdown/MarkdownHelper.h \
    src/markdown/ori_html.h \
    src/OpenTabsWidget.h \
    src/spellcheck/Spellchecker.h \
    src/spellcheck/TextEditSpellcheck.h \
    src/tabs/AppSettingsTab.h \
    src/tabs/CmdConsoleTab.h \
    src/tabs/CssEditorTab.h \
    src/tabs/HelpTab.h \
    src/tabs/MemoTab.h \
    src/tabs/PhlEditorTab.h \
    src/tabs/QssEditorTab.h \
    src/tabs/SqlConsoleTab.h \
    src/tabs/TabHelpers.h \
    src/TextEditHelpers.h \
    src/Utils.h \
    src/widgets/MemoTextBrowser.h \
    src/widgets/MemoTextEdit.h

DISTFILES += \
    src/app.qss \
    src/markdown/markdown.css
