#pragma once
#include "zusi_parser/zusi_types_fwd.hpp"
namespace zusixml {
  static void parse_element_NachfolgerAnderesModul(const Ch *&, NachfolgerAnderesModul*);
  static void parse_element_ReferenzElement(const Ch *&, ReferenzElement*);
  static void parse_element_NachfolgerSelbesModul(const Ch *&, NachfolgerSelbesModul*);
  static void parse_element_Strecke(const Ch *&, Strecke*);
  static void parse_element_StreckenelementRichtungsInfo(const Ch *&, StreckenelementRichtungsInfo*);
  static void parse_element_StrModul(const Ch *&, StrModul*);
  static void parse_element_Vec3(const Ch *&, Vec3*);
  static void parse_element_Zusi(const Ch *&, Zusi*);
  static void parse_element_Signal(const Ch *&, Signal*);
  static void parse_element_Dateiverknuepfung(const Ch *&, Dateiverknuepfung*);
  static void parse_element_UTM(const Ch *&, UTM*);
  static void parse_element_StrElement(const Ch *&, StrElement*);
  static void parse_element_Fahrplan(const Ch *&, Fahrplan*);
}  // namespace zusixml
