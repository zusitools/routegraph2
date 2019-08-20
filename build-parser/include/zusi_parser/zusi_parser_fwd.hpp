#pragma once
#include "zusi_parser/zusi_types_fwd.hpp"
namespace zusixml {
  static void parse_element_StrModul(const Ch *&, StrModul*);
  static void parse_element_Ersatzsignal(const Ch *&, Ersatzsignal*);
  static void parse_element_Beschreibung(const Ch *&, Beschreibung*);
  static void parse_element_LoeschFahrstrasse(const Ch *&, LoeschFahrstrasse*);
  static void parse_element_FahrstrSigHaltfall(const Ch *&, FahrstrSigHaltfall*);
  static void parse_element_Strecke(const Ch *&, Strecke*);
  static void parse_element_VsigBegriff(const Ch *&, VsigBegriff*);
  static void parse_element_Stufe(const Ch *&, Stufe*);
  static void parse_element_Pkt(const Ch *&, Pkt*);
  static void parse_element_Sound(const Ch *&, Sound*);
  static void parse_element_SignalFrame(const Ch *&, SignalFrame*);
  static void parse_element_NachfolgerSelbesModul(const Ch *&, NachfolgerSelbesModul*);
  static void parse_element_Zusi(const Ch *&, Zusi*);
  static void parse_element_Kennfeld(const Ch *&, Kennfeld*);
  static void parse_element_Skybox(const Ch *&, Skybox*);
  static void parse_element_FahrplanZugVerknuepfung(const Ch *&, FahrplanZugVerknuepfung*);
  static void parse_element_AutorEintrag(const Ch *&, AutorEintrag*);
  static void parse_element_StreckenStandort(const Ch *&, StreckenStandort*);
  static void parse_element_Dateiverknuepfung(const Ch *&, Dateiverknuepfung*);
  static void parse_element_Info(const Ch *&, Info*);
  static void parse_element_StrElement(const Ch *&, StrElement*);
  static void parse_element_FahrstrVSignal(const Ch *&, FahrstrVSignal*);
  static void parse_element_Vec3(const Ch *&, Vec3*);
  static void parse_element_UTM(const Ch *&, UTM*);
  static void parse_element_KoppelSignal(const Ch *&, KoppelSignal*);
  static void parse_element_Abhaengigkeit(const Ch *&, Abhaengigkeit*);
  static void parse_element_Huellkurve(const Ch *&, Huellkurve*);
  static void parse_element_Ereignis(const Ch *&, Ereignis*);
  static void parse_element_Fahrplan(const Ch *&, Fahrplan*);
  static void parse_element_SkyDome(const Ch *&, SkyDome*);
  static void parse_element_FahrstrAufloesung(const Ch *&, FahrstrAufloesung*);
  static void parse_element_Fahrstrasse(const Ch *&, Fahrstrasse*);
  static void parse_element_FahrstrTeilaufloesung(const Ch *&, FahrstrTeilaufloesung*);
  static void parse_element_ReferenzElement(const Ch *&, ReferenzElement*);
  static void parse_element_StreckenelementRichtungsInfo(const Ch *&, StreckenelementRichtungsInfo*);
  static void parse_element_NachfolgerAnderesModul(const Ch *&, NachfolgerAnderesModul*);
  static void parse_element_FahrstrStart(const Ch *&, FahrstrStart*);
  static void parse_element_FahrstrZiel(const Ch *&, FahrstrZiel*);
  static void parse_element_MatrixEintrag(const Ch *&, MatrixEintrag*);
  static void parse_element_FahrstrRegister(const Ch *&, FahrstrRegister*);
  static void parse_element_FahrstrWeiche(const Ch *&, FahrstrWeiche*);
  static void parse_element_ModulDateiVerknuepfung(const Ch *&, ModulDateiVerknuepfung*);
  static void parse_element_FahrstrSignal(const Ch *&, FahrstrSignal*);
  static void parse_element_Signal(const Ch *&, Signal*);
  static void parse_element_HsigBegriff(const Ch *&, HsigBegriff*);
}  // namespace zusixml
