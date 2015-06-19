#-------------------------------------------------
#
# Project created by QtCreator 2012-11-14T14:37:11
#
#-------------------------------------------------

QT       += core xml sql

QT       -= gui

TARGET = csprodattosqlite
TEMPLATE = app

CONFIG   += console
CONFIG   -= app_bundle

unix:INCLUDEPATH += ../3rdparty

SOURCES += main.cpp
