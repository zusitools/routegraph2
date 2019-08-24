QT       += core gui

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = routegraph2
TEMPLATE = app

CONFIG += c++1z rtti_off

SOURCES += main.cpp\
        mainwindow.cpp \
    streckennetz.cpp \
    view/streckeview.cpp \
    view/streckescene.cpp \
    view/graphicsitems/streckensegmentitem.cpp \
    routegraphapplication.cpp \
    view/graphicsitems/dreieckitem.cpp \
    view/graphicsitems/label.cpp \
    view/segmentierer.cpp \
    view/visualisierung/gleisfunktionvisualisierung.cpp \
    view/visualisierung/geschwindigkeitvisualisierung.cpp \
    view/visualisierung/oberbauvisualisierung.cpp \
    view/visualisierung/visualisierung.cpp \
    view/visualisierung/fahrleitungvisualisierung.cpp \
    view/visualisierung/kruemmungvisualisierung.cpp \
    view/visualisierung/ueberhoehungvisualisierung.cpp \
    view/visualisierung/neigungvisualisierung.cpp

HEADERS  += mainwindow.h \
    streckennetz.h \
    view/streckeview.h \
    view/streckescene.h \
    view/graphicsitems/minbreitegraphicsitem.h \
    view/graphicsitems/streckensegmentitem.h \
    routegraphapplication.h \
    view/graphicsitems/dreieckitem.h \
    view/graphicsitems/label.h \
    view/zwerte.h \
    view/segmentierer.h \
    view/visualisierung/visualisierung.h \
    view/visualisierung/gleisfunktionvisualisierung.h \
    view/visualisierung/geschwindigkeitvisualisierung.h \
    view/visualisierung/oberbauvisualisierung.h \
    view/visualisierung/fahrleitungvisualisierung.h \
    view/visualisierung/kruemmungvisualisierung.h \
    view/visualisierung/ueberhoehungvisualisierung.h \
    view/visualisierung/neigungvisualisierung.h \
    model/streckenelement.h

FORMS    += mainwindow.ui

INCLUDEPATH += $$_PRO_FILE_PWD_/build-parser/include
INCLUDEPATH += $$_PRO_FILE_PWD_/parser/include
INCLUDEPATH += $$_PRO_FILE_PWD_/parser/rapidxml-mod

QMAKE_CXXFLAGS_RELEASE -= -O
QMAKE_CXXFLAGS_RELEASE -= -O1
QMAKE_CXXFLAGS_RELEASE -= -O2
QMAKE_CXXFLAGS_RELEASE *= -O3 -flto

win32 {
    LIBS += -ladvapi32 -lpthread -lboost_system-mt -lboost_filesystem-mt
    QMAKE_CXXFLAGS += -DZUSI_PARSER_USE_BOOST_FILESYSTEM
}
