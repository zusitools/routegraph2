QT       += core gui

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = routegraph2
TEMPLATE = app

CONFIG += c++14 rtti_off

SOURCES += main.cpp\
        mainwindow.cpp \
    view/streckeview.cpp \
    view/streckescene.cpp \
    view/graphicsitems/streckensegmentitem.cpp \
    routegraphapplication.cpp \
    view/graphicsitems/dreieckitem.cpp \
    view/graphicsitems/label.cpp \
    view/segmentierer.cpp \
    view/visualisierung.cpp

HEADERS  += mainwindow.h \
    view/streckeview.h \
    view/streckescene.h \
    view/graphicsitems/minbreitegraphicsitem.h \
    view/graphicsitems/streckensegmentitem.h \
    routegraphapplication.h \
    view/graphicsitems/dreieckitem.h \
    view/graphicsitems/label.h \
    view/zwerte.h \
    view/segmentierer.h \
    view/visualisierung.h

FORMS    += mainwindow.ui

INCLUDEPATH += $$_PRO_FILE_PWD_/zusi_file_lib
INCLUDEPATH += $$_PRO_FILE_PWD_/zusi_file_lib/src

QMAKE_CXXFLAGS_RELEASE -= -O
QMAKE_CXXFLAGS_RELEASE -= -O1
QMAKE_CXXFLAGS_RELEASE -= -O2
QMAKE_CXXFLAGS_RELEASE *= -O3 -flto

win32 {
    LIBS += -ladvapi32 -lpthread
}
