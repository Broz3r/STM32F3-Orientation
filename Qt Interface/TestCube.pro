#-------------------------------------------------
#
# Project created by QtCreator 2015-05-17T15:04:39
#
#-------------------------------------------------

QT += core gui opengl serialport

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = TestCube
TEMPLATE = app


SOURCES += main.cpp \
    myglwidget.cpp \
    mainwindow.cpp \
    settingsdialog.cpp

HEADERS  += \
    myglwidget.h \
    mainwindow.h \
    settingsdialog.h

FORMS    += \
    mainwindow.ui \
    settingsdialog.ui
