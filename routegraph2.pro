QT       += core gui

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = routegraph2
TEMPLATE = app


SOURCES += main.cpp\
        mainwindow.cpp \
    view/streckeview.cpp

HEADERS  += mainwindow.h \
    view/streckeview.h

FORMS    += mainwindow.ui
