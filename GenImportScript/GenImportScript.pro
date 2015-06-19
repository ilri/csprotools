#-------------------------------------------------
#
# Project created by QtCreator 2012-11-16T09:00:12
#
#-------------------------------------------------

QT       += core xml

QT       -= gui

TARGET = genimportscript
CONFIG   += console
CONFIG   -= app_bundle

TEMPLATE = app

unix:INCLUDEPATH += ../3rdparty

SOURCES += main.cpp

RESOURCES += \
    files.qrc
