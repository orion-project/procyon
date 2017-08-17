#-------------------------------------------------
#
# Project created by QtCreator 2017-08-17T13:11:44
#
#-------------------------------------------------

QT       += core gui sql

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = procyon
TEMPLATE = app
DESTDIR = $$_PRO_FILE_PWD_/bin

ORION = $$_PRO_FILE_PWD_/orion/
include($$ORION"orion.pri")

# The following define makes your compiler emit warnings if you use
# any feature of Qt which as been marked as deprecated (the exact warnings
# depend on your compiler). Please consult the documentation of the
# deprecated API in order to know how to port your code away from it.
DEFINES += QT_DEPRECATED_WARNINGS

# You can also make your code fail to compile if you use deprecated APIs.
# In order to do so, uncomment the following line.
# You can also select to disable deprecated APIs only up to a certain version of Qt.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0


SOURCES += src/main.cpp\
        src/MainWindow.cpp \
    src/CatalogWidget.cpp \
    src/InfoWidget.cpp \
    src/Glass.cpp \
    src/Catalog.cpp \
    src/CatalogStore.cpp \
    src/SqlHelper.cpp \
    src/DispersionPlot.cpp

HEADERS  += src/MainWindow.h \
    src/CatalogWidget.h \
    src/InfoWidget.h \
    src/Glass.h \
    src/Catalog.h \
    src/CatalogModel.h \
    src/CatalogStore.h \
    src/SqlHelper.h \
    src/DispersionPlot.h \
    src/Appearance.h
