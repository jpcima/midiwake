TEMPLATE = app

QT += core
QT -= gui

CONFIG += c++11 console
CONFIG -= app_bundle

TARGET = SigTest

include(../qctrlsignals.pri)

SOURCES += main.cpp
