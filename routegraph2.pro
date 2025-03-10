QT       += core gui

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

VERSION = 0.1.16
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
    view/visualisierung/etcstrustedareavisualisierung.cpp \
    view/visualisierung/gleisfunktionvisualisierung.cpp \
    view/visualisierung/geschwindigkeitvisualisierung.cpp \
    view/visualisierung/oberbauvisualisierung.cpp \
    view/visualisierung/visualisierung.cpp \
    view/visualisierung/fahrleitungvisualisierung.cpp \
    view/visualisierung/kruemmungvisualisierung.cpp \
    view/visualisierung/ueberhoehungvisualisierung.cpp \
    view/visualisierung/neigungvisualisierung.cpp \
    utm/utm.c

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
    view/visualisierung/etcstrustedareavisualisierung.h \
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
INCLUDEPATH += $$_PRO_FILE_PWD_/utm/include

QMAKE_CXXFLAGS_RELEASE -= -O
QMAKE_CXXFLAGS_RELEASE -= -O1
QMAKE_CXXFLAGS_RELEASE -= -O2
QMAKE_CXXFLAGS_RELEASE *= -O3 -flto

QMAKE_CXXFLAGS += -DROUTEGRAPH2_VERSION=$$VERSION

win32 {
    LIBS += -ladvapi32 -lpthread -lboost_system-mt-x64 -lboost_filesystem-mt-x64 -lboost_nowide-mt-x64 -ldbgeng
    QMAKE_CXXFLAGS += -DZUSI_PARSER_USE_BOOST_FILESYSTEM
}
linux {
    QMAKE_CXXFLAGS_DEBUG *= -fsanitize=address,undefined
    QMAKE_LFLAGS_DEBUG *= -fsanitize=address,undefined
}
