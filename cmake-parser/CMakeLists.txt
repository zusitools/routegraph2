# Ein gemeinsamer zusi_parser für ls3render und routegraph2:
# ls3render/CMakeLists.txt erzeugt zusi_parser (Landschaft/LS3-Whitelist).
# Routegraph-Einträge werden per WHITELIST APPEND ergänzt (kein zweites generate_zusi_parser).
#
# cd path/to/routegraph2/
# mkdir build-parser
# cd build-parser
# cmake ../cmake-parser
# cmake --build . --target zusi_parser_includes
#
# Generierte Header: <build-parser>/ls3render/zusi_parser/zusi_parser/zusi_types.hpp
# (INCLUDEPATH = .../ls3render/zusi_parser, Include: "zusi_parser/zusi_types.hpp")
#
# Zusätzliche Quellen: ls3render/parser/include, ls3render/parser/rapidxml-mod
# (in routegraph2.pro eintragen; nicht die alten parser/-Pfade aus dem Wurzel-parser.)

cmake_minimum_required(VERSION 3.20)
project(routegraph2_parser)

add_subdirectory(../ls3render ls3render)

# Ohne Duplikate zu ls3render/CMakeLists.txt (u. a. Vec3::*, Dateiverknuepfung::Dateiname).
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

    SignalFrame::Datei
    SignalFrame::p
    SignalFrame::phi
    SignalFrame::sk

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
