cmake_minimum_required(VERSION 3.20)

project(routegraph2 VERSION 0.1.18 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_INCLUDE_CURRENT_DIR ON)

# Helps Qt Creator pick up generated headers (e.g. ui_*.h, zusi_parser/*.hpp).
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
  set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type" FORCE)
  set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS Debug Release RelWithDebInfo MinSizeRel)
endif()

# ---------------------------------------------------------------------------
# zusi_parser / ls3render
#
# The ls3render subdirectory provides:
#   * generate_zusi_parser(): defines a zusi_parser INTERFACE target whose
#     headers are generated from the XSDs in ls3render/parser/xsd.
#   * the ls3render library used for the LS3 signal preview.
#
# routegraph2 needs additional types from zusi_parser beyond the
# Landschaft/LS3 whitelist already configured by ls3render/CMakeLists.txt,
# so we extend the WHITELIST property of the zusi_parser target below.
# ---------------------------------------------------------------------------
add_subdirectory(ls3render ls3render)

set_property(TARGET zusi_parser APPEND PROPERTY WHITELIST
    Zusi::Strecke
    Zusi::Fahrplan

    Strecke::StrElement
    Strecke::UTM
    Strecke::ReferenzElemente
    Strecke::Fahrstrasse

    Fahrstrasse::FahrstrName
    Fahrstrasse::FahrstrTyp
    Fahrstrasse::FahrstrStart
    Fahrstrasse::FahrstrZiel
    Fahrstrasse::FahrstrWeiche
    Fahrstrasse::FahrstrRegister
    Fahrstrasse::FahrstrAufloesung
    Fahrstrasse::FahrstrTeilaufloesung
    Fahrstrasse::FahrstrSigHaltfall
    Fahrstrasse::FahrstrSignal
    Fahrstrasse::FahrstrVSignal

    FahrstrStart::Ref
    FahrstrStart::Datei
    FahrstrZiel::Ref
    FahrstrZiel::Datei
    FahrstrWeiche::Ref
    FahrstrWeiche::Datei
    FahrstrWeiche::FahrstrWeichenlage
    FahrstrRegister::Ref
    FahrstrRegister::Datei
    FahrstrAufloesung::Ref
    FahrstrAufloesung::Datei
    FahrstrTeilaufloesung::Ref
    FahrstrTeilaufloesung::Datei
    FahrstrSigHaltfall::Ref
    FahrstrSigHaltfall::Datei
    FahrstrSignal::Ref
    FahrstrSignal::Datei
    FahrstrSignal::FahrstrSignalZeile
    FahrstrSignal::FahrstrSignalErsatzsignal
    FahrstrVSignal::Ref
    FahrstrVSignal::Datei
    FahrstrVSignal::FahrstrSignalSpalte

    StrElement::Fkt
    StrElement::Anschluss
    StrElement::Nr
    StrElement::kr
    StrElement::Ueberh
    StrElement::Volt
    StrElement::Oberbau
    StrElement::Drahthoehe
    StrElement::InfoNormRichtung
    StrElement::InfoGegenRichtung
    StrElement::NachNorm
    StrElement::NachGegen
    StrElement::NachNormModul
    StrElement::NachGegenModul
    StrElement::g
    StrElement::b

    StreckenelementRichtungsInfo::vMax
    StreckenelementRichtungsInfo::Signal
    StreckenelementRichtungsInfo::Ereignis

    Ereignis::Er

    NachfolgerSelbesModul::Nr
    NachfolgerAnderesModul::Nr
    NachfolgerAnderesModul::Datei

    Fahrplan::StrModul
    StrModul::Datei

    Signal::SignalTyp
    Signal::Signalname
    Signal::NameBetriebsstelle
    Signal::Stellwerk
    Signal::SignalFrame
    Signal::HsigBegriff
    Signal::VsigBegriff
    Signal::MatrixEintrag
    Signal::Ersatzsignal
    Signal::KoppelSignal

    VsigBegriff::VsigGeschw

    SignalFrame::Datei
    SignalFrame::p
    SignalFrame::phi
    SignalFrame::sk

    MatrixEintrag::Signalbild

    Ersatzsignal::MatrixEintrag

    KoppelSignal::ReferenzNr
    KoppelSignal::Datei

    ReferenzElement::RefTyp
    ReferenzElement::StrElement
    ReferenzElement::StrNorm
    ReferenzElement::Info
    ReferenzElement::ReferenzNr
    UTM::UTM_WE
    UTM::UTM_NS
    UTM::UTM_Zone
    ETCSFunkmast::ETCSSenderadius
    ETCSFunkmast::ETCSRBCNummer
)

# ---------------------------------------------------------------------------
# Qt
# ---------------------------------------------------------------------------
find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS Core Gui Widgets)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Core Gui Widgets)

# ---------------------------------------------------------------------------
# Boost (required for boost::stacktrace in main.cpp)
# ---------------------------------------------------------------------------
if(WIN32)
  find_package(Boost REQUIRED COMPONENTS filesystem nowide)
else()
  find_package(Boost REQUIRED)
endif()

# ---------------------------------------------------------------------------
# routegraph2 sources
# ---------------------------------------------------------------------------
set(routegraph2_SOURCES
    main.cpp
    mainwindow.cpp
    routegraphapplication.cpp
    streckennetz.cpp
    model/fahrstrasse.cpp
    model/fahrstrassendetails.cpp
    view/anleitungwindow.cpp
    view/bahnsteige.cpp
    view/cursorwraphelper.cpp
    view/fahrstrassendetailswindow.cpp
    view/fahrstrassenpanel.cpp
    view/segmentierer.cpp
    view/streckescene.cpp
    view/streckeview.cpp
    view/graphicsitems/dreieckitem.cpp
    view/graphicsitems/label.cpp
    view/graphicsitems/streckensegmentitem.cpp
    view/visualisierung/etcstrustedareavisualisierung.cpp
    view/visualisierung/fahrleitungvisualisierung.cpp
    view/visualisierung/geschwindigkeitvisualisierung.cpp
    view/visualisierung/gleisfunktionvisualisierung.cpp
    view/visualisierung/kruemmungvisualisierung.cpp
    view/visualisierung/neigungvisualisierung.cpp
    view/visualisierung/oberbauvisualisierung.cpp
    view/visualisierung/ueberhoehungvisualisierung.cpp
    view/visualisierung/visualisierung.cpp
    utm/utm.c
)

set(routegraph2_HEADERS
    mainwindow.h
    routegraphapplication.h
    streckennetz.h
    model/fahrstrasse.h
    model/fahrstrassendetails.h
    model/streckenelement.h
    view/anleitungwindow.h
    view/bahnsteige.h
    view/cursorwraphelper.h
    view/fahrstrassendetailswindow.h
    view/fahrstrassenpanel.h
    view/segmentierer.h
    view/streckescene.h
    view/streckeview.h
    view/zwerte.h
    view/graphicsitems/dreieckitem.h
    view/graphicsitems/label.h
    view/graphicsitems/minbreitegraphicsitem.h
    view/graphicsitems/streckensegmentitem.h
    view/visualisierung/etcstrustedareavisualisierung.h
    view/visualisierung/fahrleitungvisualisierung.h
    view/visualisierung/geschwindigkeitvisualisierung.h
    view/visualisierung/gleisfunktionvisualisierung.h
    view/visualisierung/kruemmungvisualisierung.h
    view/visualisierung/neigungvisualisierung.h
    view/visualisierung/oberbauvisualisierung.h
    view/visualisierung/ueberhoehungvisualisierung.h
    view/visualisierung/visualisierung.h
)

set(routegraph2_FORMS
    mainwindow.ui
)

add_executable(routegraph2 WIN32
    ${routegraph2_SOURCES}
    ${routegraph2_HEADERS}
    ${routegraph2_FORMS}
)

target_include_directories(routegraph2 PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/utm/include
)

target_link_libraries(routegraph2 PRIVATE
    Qt${QT_VERSION_MAJOR}::Core
    Qt${QT_VERSION_MAJOR}::Gui
    Qt${QT_VERSION_MAJOR}::Widgets
    ls3render
    zusi_parser
    Boost::headers
)

target_compile_definitions(routegraph2 PRIVATE
    ROUTEGRAPH2_VERSION=${PROJECT_VERSION}
    GLM_ENABLE_EXPERIMENTAL
)

# Build flags equivalent to the previous QMAKE_CXXFLAGS_RELEASE override
# (-O3 -flto for Release builds).
target_compile_options(routegraph2 PRIVATE
    $<$<CONFIG:Release>:-O3 -flto>
)
target_link_options(routegraph2 PRIVATE
    $<$<CONFIG:Release>:-flto>
)

if(WIN32)
  target_compile_definitions(routegraph2 PRIVATE ZUSI_PARSER_USE_BOOST_FILESYSTEM)
  target_link_libraries(routegraph2 PRIVATE
    Boost::filesystem
    Boost::nowide
    advapi32
    dbgeng
    pthread
  )
endif()

if(UNIX AND NOT APPLE)
  # AddressSanitizer + UBSan in Debug builds
  target_compile_options(routegraph2 PRIVATE
      $<$<CONFIG:Debug>:-fsanitize=address,undefined>
  )
  target_link_options(routegraph2 PRIVATE
      $<$<CONFIG:Debug>:-fsanitize=address,undefined>
  )
endif()

install(TARGETS routegraph2 RUNTIME DESTINATION bin)
