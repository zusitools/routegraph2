QT       += core gui

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = routegraph2
TEMPLATE = app


SOURCES += main.cpp\
        mainwindow.cpp \
    view/streckeview.cpp \
    view/streckescene.cpp \
    zusi_file_lib/src/model/streckenelement.cpp \
    zusi_file_lib/src/model/punkt3d.cpp \
    zusi_file_lib/src/io/z2_leser.cpp \
    zusi_file_lib/src/io/str_leser.cpp \
    view/graphicsitems/streckensegmentitem.cpp \
    routegraphapplication.cpp \
    zusi_file_lib/src/io/z3_leser.cpp \
    zusi_file_lib/src/io/st3_leser.cpp

HEADERS  += mainwindow.h \
    view/streckeview.h \
    view/streckescene.h \
    view/graphicsitems/minbreitegraphicsitem.h \
    view/graphicsitems/streckensegmentitem.h \
    routegraphapplication.h

FORMS    += mainwindow.ui

QMAKE_CXXFLAGS += -std=c++11

INCLUDEPATH += $$PWD/zusi_file_lib
INCLUDEPATH += $$PWD/zusi_file_lib/src
