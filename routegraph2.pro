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
    view/graphicsitems/streckenelementitem.cpp

HEADERS  += mainwindow.h \
    view/streckeview.h \
    view/streckescene.h \
    view/graphicsitems/streckenelementitem.h

FORMS    += mainwindow.ui

QMAKE_CXXFLAGS += -std=c++11

INCLUDEPATH += $$PWD/zusi_file_lib/src
