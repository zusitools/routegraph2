QT       += core gui

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = routegraph2
TEMPLATE = app


SOURCES += main.cpp\
        mainwindow.cpp \
    view/streckeview.cpp \
    view/streckescene.cpp \
    view/graphicsitems/streckensegmentitem.cpp \
    routegraphapplication.cpp \
    view/graphicsitems/dreieckitem.cpp \
    view/graphicsitems/label.cpp \
    view/segmentierer.cpp

HEADERS  += mainwindow.h \
    view/streckeview.h \
    view/streckescene.h \
    view/graphicsitems/minbreitegraphicsitem.h \
    view/graphicsitems/streckensegmentitem.h \
    routegraphapplication.h \
    view/graphicsitems/dreieckitem.h \
    view/graphicsitems/label.h \
    view/zwerte.h \
    view/segmentierer.h

FORMS    += mainwindow.ui

QMAKE_CXXFLAGS += -std=c++11

INCLUDEPATH += $$PWD/zusi_file_lib
INCLUDEPATH += $$PWD/zusi_file_lib/src
