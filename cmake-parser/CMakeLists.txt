# Diese Datei dient dazu, den Zusi-Parser mit CMake zu bauen.
# Der Rest des Programms wird mit qmake gebaut.
#
# cd path/to/routegraph2/
# mkdir build-parser
# cd build-parser
# cmake ../cmake-parser
# cmake --build . --target zusi_parser_includes

cmake_minimum_required(VERSION 3.10)
project(routegraph2_parser)

add_subdirectory(../parser parser)
generate_zusi_parser(zusi_parser ${CMAKE_CURRENT_BINARY_DIR}/include
  WHITELIST
    Zusi::Strecke
    Zusi::Fahrplan

    Strecke::StrElement
    Strecke::UTM
    Strecke::ReferenzElemente

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

    NachfolgerSelbesModul::Nr
    NachfolgerAnderesModul::Nr
    NachfolgerAnderesModul::Datei

    Fahrplan::StrModul
    StrModul::Datei
    Vec3::X
    Vec3::Y
    Vec3::Z

    Dateiverknuepfung::Dateiname

    Signal::SignalTyp
    Signal::Signalname
    Signal::NameBetriebsstelle
    Signal::Stellwerk
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
  IGNORE_UNKNOWN)
