#pragma once
#include "zusi_parser/zusi_types_fwd.hpp"
#include "boost/container/small_vector.hpp"
#include <vector>  // for std::vector
#include <memory>  // for std::unique_ptr
#include <optional>// for std::optional
#include <ctime>   // for struct tm
#include <cmath>   // for std::hypot
struct ArgbColor {
  uint8_t a, r, g, b;
};
namespace zusixml {
  template <typename T>
  using allocator = std::allocator<T>;
  template <typename T>
  using deleter = std::default_delete<T>;
}
struct IndusiZugdaten {
  int32_t BRH;
  std::basic_string<char, std::char_traits<char>, zusixml::allocator<char>> TfNummer;
  std::basic_string<char, std::char_traits<char>, zusixml::allocator<char>> Zugnummer;
  int32_t BRA;
  int32_t ZugsicherungHS;
  int32_t Lufthahn;
  int32_t PZBStoerschalter;
};
struct LZBZugdaten : IndusiZugdaten {
  int32_t VMZ;
  int32_t ZL;
  int32_t LZBStoerschalter;
  int32_t LZBGefuehrt;
};
struct Vec2 {
  float X;
  float Y;
};
struct Beschreibung {
  std::basic_string<char, std::char_traits<char>, zusixml::allocator<char>> Beschreibung;
};
struct LoeschFahrstrasse {
  std::basic_string<char, std::char_traits<char>, zusixml::allocator<char>> FahrstrName;
};
struct Dateiverknuepfung {
  /** Dateipfade sind entweder
- Pfade relativ zum Zusi-Datenverzeichnis (RollingStock\Deutschland\Lok.ls3), wobei ein führender Backslash erlaubt ist und ignoriert wird, oder
- wenn sie keinen Backslash enthalten, Dateien im selben Verzeichnis wie die Datei, in der der Pfad steht.
Insbesondere können Dateien, die auf einem anderen Laufwerk als das Zusi-Datenverzeichnis liegen, nicht referenziert werden.*/
  std::basic_string<char, std::char_traits<char>, zusixml::allocator<char>> Dateiname;
  bool inst;
  bool NurInfo;
};
struct Skybox {
  struct Dateiverknuepfung Skybox0; // inlined
  struct Dateiverknuepfung Skybox1; // inlined
  struct Dateiverknuepfung Skybox2; // inlined
  struct Dateiverknuepfung Skybox3; // inlined
  struct Dateiverknuepfung Skybox4; // inlined
  struct Dateiverknuepfung Skybox5; // inlined
};
struct Vec3 {
  float X;
  float Y;
  float Z;
};
struct StreckenStandort {
  std::basic_string<char, std::char_traits<char>, zusixml::allocator<char>> StrInfo;
  struct Vec3 p; // inlined
  struct Vec3 lookat; // inlined
  struct Vec3 up; // inlined
};
struct NachfolgerSelbesModul {
  int32_t Nr;
};
struct Ereignis {
  int32_t Er;
  std::basic_string<char, std::char_traits<char>, zusixml::allocator<char>> Beschr;
  float Wert;
};
struct MatrixEintrag {
  int64_t Signalbild;
  float MatrixGeschw;
  int32_t SignalID;
  std::vector<std::unique_ptr<struct Ereignis, zusixml::deleter<struct Ereignis>>, zusixml::allocator<std::unique_ptr<struct Ereignis, zusixml::deleter<struct Ereignis>>>> children_Ereignis;
};
struct Ersatzsignal {
  std::basic_string<char, std::char_traits<char>, zusixml::allocator<char>> ErsatzsigBezeichnung;
  int32_t ErsatzsigID;
  /** Ist dieses Attribut auf "1" gesetzt, wird der Signalassistent nur ein bereits bestehendes Ersatzsignal mit gleicher ErsatzsigID mit den Daten dieses Eintrags vervollständigen, aber nicht (wie bei Attributwert "0") ein neues Ersatzsignal anlegen, wenn noch keines mit gleicher ErsatzsigID existiert.*/
  bool SigAssiErgaenzen;
  struct MatrixEintrag MatrixEintrag; // inlined
};
struct VsigBegriff {
  float VsigGeschw;
};
struct SignalFrame {
  int32_t WeichenbaugruppeIndex;
  int32_t WeichenbaugruppeNr;
  std::basic_string<char, std::char_traits<char>, zusixml::allocator<char>> WeichenbaugruppeBeschreibung;
  bool WeichenbaugruppePos0;
  bool WeichenbaugruppePos1;
  struct Vec3 p; // inlined
  struct Vec3 phi; // inlined
  struct Dateiverknuepfung Datei; // inlined
  std::vector<std::unique_ptr<struct Ereignis, zusixml::deleter<struct Ereignis>>, zusixml::allocator<std::unique_ptr<struct Ereignis, zusixml::deleter<struct Ereignis>>>> children_Ereignis;
};
struct KoppelSignal {
  int32_t ReferenzNr;
  struct Dateiverknuepfung Datei; // inlined
};
struct HsigBegriff {
  float HsigGeschw;
  int64_t FahrstrTyp;
};
struct Signal {
  std::basic_string<char, std::char_traits<char>, zusixml::allocator<char>> NameBetriebsstelle;
  std::basic_string<char, std::char_traits<char>, zusixml::allocator<char>> Stellwerk;
  std::basic_string<char, std::char_traits<char>, zusixml::allocator<char>> Signalname;
  float ZufallsWert;
  int32_t SignalFlags;
  int32_t SignalTyp;
  int32_t Weichenbauart;
  int32_t WeichenGrundstellung;
  bool WeicheStumpfIgnorieren;
  int32_t BoundingR;
  float Zwangshelligkeit;
  struct Vec3 p; // inlined
  struct Vec3 phi; // inlined
  std::unique_ptr<struct KoppelSignal, zusixml::deleter<struct KoppelSignal>> KoppelSignal;
  std::vector<std::unique_ptr<struct SignalFrame, zusixml::deleter<struct SignalFrame>>, zusixml::allocator<std::unique_ptr<struct SignalFrame, zusixml::deleter<struct SignalFrame>>>> children_SignalFrame;
  std::vector<std::unique_ptr<struct HsigBegriff, zusixml::deleter<struct HsigBegriff>>, zusixml::allocator<std::unique_ptr<struct HsigBegriff, zusixml::deleter<struct HsigBegriff>>>> children_HsigBegriff;
  std::vector<std::unique_ptr<struct VsigBegriff, zusixml::deleter<struct VsigBegriff>>, zusixml::allocator<std::unique_ptr<struct VsigBegriff, zusixml::deleter<struct VsigBegriff>>>> children_VsigBegriff;
  std::vector<std::unique_ptr<struct MatrixEintrag, zusixml::deleter<struct MatrixEintrag>>, zusixml::allocator<std::unique_ptr<struct MatrixEintrag, zusixml::deleter<struct MatrixEintrag>>>> children_MatrixEintrag;
  std::vector<std::unique_ptr<struct Ersatzsignal, zusixml::deleter<struct Ersatzsignal>>, zusixml::allocator<std::unique_ptr<struct Ersatzsignal, zusixml::deleter<struct Ersatzsignal>>>> children_Ersatzsignal;
};
struct StreckenelementRichtungsInfo {
  float vMax;
  float km;
  bool pos;
  int32_t Reg;
  int32_t KoppelWeicheNr;
  bool KoppelWeicheNorm;
  std::vector<std::unique_ptr<struct Ereignis, zusixml::deleter<struct Ereignis>>, zusixml::allocator<std::unique_ptr<struct Ereignis, zusixml::deleter<struct Ereignis>>>> children_Ereignis;
  std::unique_ptr<struct Signal, zusixml::deleter<struct Signal>> Signal;
};
struct NachfolgerAnderesModul {
  int64_t Nr;
  struct Dateiverknuepfung Datei; // inlined
};
struct StrElement {
  int32_t Nr;
  float Ueberh;
  float kr;
  float spTrass;
  int32_t Anschluss;
  int64_t Fkt;
  std::basic_string<char, std::char_traits<char>, zusixml::allocator<char>> Oberbau;
  int32_t Volt;
  float Drahthoehe;
  float Zwangshelligkeit;
  int64_t Streckennummer;
  struct Vec3 g; // inlined
  struct Vec3 b; // inlined
  std::optional<struct StreckenelementRichtungsInfo> InfoNormRichtung;
  std::optional<struct StreckenelementRichtungsInfo> InfoGegenRichtung;
  boost::container::small_vector<struct NachfolgerSelbesModul, 2> children_NachNorm;
  boost::container::small_vector<struct NachfolgerSelbesModul, 2> children_NachGegen;
  boost::container::small_vector<struct NachfolgerAnderesModul, 2> children_NachNormModul;
  boost::container::small_vector<struct NachfolgerAnderesModul, 2> children_NachGegenModul;
  std::vector<StrElement*> nachfolgerElementeNorm;
  std::vector<StrElement*> nachfolgerElementeGegen;
  mutable std::optional<float> NeigungCache;
  float Neigung() const {
    if (!NeigungCache) {
      NeigungCache = (b.Z - g.Z) / std::hypot(b.X - g.X, b.Y - g.Y, b.Z - g.Z);
    }
    return *NeigungCache;
  }
};
struct UTM {
  int32_t UTM_WE;
  int32_t UTM_NS;
  int32_t UTM_Zone;
  std::basic_string<char, std::char_traits<char>, zusixml::allocator<char>> UTM_Zone2;
};
struct Huellkurve {
  std::vector<std::unique_ptr<struct Vec3, zusixml::deleter<struct Vec3>>, zusixml::allocator<std::unique_ptr<struct Vec3, zusixml::deleter<struct Vec3>>>> children_PunktXYZ;
};
struct SkyDome {
  struct Dateiverknuepfung HimmelTex; // inlined
  struct Dateiverknuepfung SonneTex; // inlined
  struct Dateiverknuepfung SonneHorizontTex; // inlined
  struct Dateiverknuepfung MondTex; // inlined
  struct Dateiverknuepfung SternTex; // inlined
};
struct FahrstrSigHaltfall {
  int64_t Ref;
  struct Dateiverknuepfung Datei; // inlined
};
struct FahrstrVSignal {
  int64_t Ref;
  int32_t FahrstrSignalSpalte;
  struct Dateiverknuepfung Datei; // inlined
};
struct FahrstrAufloesung {
  int64_t Ref;
  struct Dateiverknuepfung Datei; // inlined
};
struct FahrstrTeilaufloesung {
  int64_t Ref;
  struct Dateiverknuepfung Datei; // inlined
};
struct FahrstrStart {
  int64_t Ref;
  struct Dateiverknuepfung Datei; // inlined
};
struct FahrstrZiel {
  int64_t Ref;
  struct Dateiverknuepfung Datei; // inlined
};
struct FahrstrRegister {
  int64_t Ref;
  struct Dateiverknuepfung Datei; // inlined
};
struct FahrstrWeiche {
  int64_t Ref;
  int32_t FahrstrWeichenlage;
  struct Dateiverknuepfung Datei; // inlined
};
struct FahrstrSignal {
  int64_t Ref;
  int32_t FahrstrSignalZeile;
  bool FahrstrSignalErsatzsignal;
  struct Dateiverknuepfung Datei; // inlined
};
struct Fahrstrasse {
  std::basic_string<char, std::char_traits<char>, zusixml::allocator<char>> FahrstrName;
  std::basic_string<char, std::char_traits<char>, zusixml::allocator<char>> FahrstrStrecke;
  int32_t RglGgl;
  std::basic_string<char, std::char_traits<char>, zusixml::allocator<char>> FahrstrTyp;
  /** Wahrscheinlichkeit (zwischen 0 und 1), dass diese Fahrstrasse nicht zufallsgesteuert angewaehlt wird.*/
  float ZufallsWert;
  float Laenge;
  std::unique_ptr<struct FahrstrStart, zusixml::deleter<struct FahrstrStart>> FahrstrStart;
  std::unique_ptr<struct FahrstrZiel, zusixml::deleter<struct FahrstrZiel>> FahrstrZiel;
  std::vector<std::unique_ptr<struct FahrstrRegister, zusixml::deleter<struct FahrstrRegister>>, zusixml::allocator<std::unique_ptr<struct FahrstrRegister, zusixml::deleter<struct FahrstrRegister>>>> children_FahrstrRegister;
  std::vector<std::unique_ptr<struct FahrstrAufloesung, zusixml::deleter<struct FahrstrAufloesung>>, zusixml::allocator<std::unique_ptr<struct FahrstrAufloesung, zusixml::deleter<struct FahrstrAufloesung>>>> children_FahrstrAufloesung;
  std::vector<std::unique_ptr<struct FahrstrTeilaufloesung, zusixml::deleter<struct FahrstrTeilaufloesung>>, zusixml::allocator<std::unique_ptr<struct FahrstrTeilaufloesung, zusixml::deleter<struct FahrstrTeilaufloesung>>>> children_FahrstrTeilaufloesung;
  std::vector<std::unique_ptr<struct FahrstrSigHaltfall, zusixml::deleter<struct FahrstrSigHaltfall>>, zusixml::allocator<std::unique_ptr<struct FahrstrSigHaltfall, zusixml::deleter<struct FahrstrSigHaltfall>>>> children_FahrstrSigHaltfall;
  std::vector<std::unique_ptr<struct FahrstrWeiche, zusixml::deleter<struct FahrstrWeiche>>, zusixml::allocator<std::unique_ptr<struct FahrstrWeiche, zusixml::deleter<struct FahrstrWeiche>>>> children_FahrstrWeiche;
  std::vector<std::unique_ptr<struct FahrstrSignal, zusixml::deleter<struct FahrstrSignal>>, zusixml::allocator<std::unique_ptr<struct FahrstrSignal, zusixml::deleter<struct FahrstrSignal>>>> children_FahrstrSignal;
  std::vector<std::unique_ptr<struct FahrstrVSignal, zusixml::deleter<struct FahrstrVSignal>>, zusixml::allocator<std::unique_ptr<struct FahrstrVSignal, zusixml::deleter<struct FahrstrVSignal>>>> children_FahrstrVSignal;
};
struct ReferenzElement {
  int32_t ReferenzNr;
  int32_t StrElement;
  bool StrNorm;
  int32_t RefTyp;
  std::basic_string<char, std::char_traits<char>, zusixml::allocator<char>> Info;
};
struct ModulDateiVerknuepfung {
  struct Dateiverknuepfung Datei; // inlined
};
struct Strecke {
  int32_t RekTiefe;
  int32_t Himmelsmodell;
  /** Die zur Strecke gehörende Landschaftsdatei.*/
  struct Dateiverknuepfung Datei; // inlined
  std::vector<std::unique_ptr<struct LoeschFahrstrasse, zusixml::deleter<struct LoeschFahrstrasse>>, zusixml::allocator<std::unique_ptr<struct LoeschFahrstrasse, zusixml::deleter<struct LoeschFahrstrasse>>>> children_LoeschFahrstrasse;
  struct Dateiverknuepfung HintergrundDatei; // inlined
  struct Dateiverknuepfung BefehlsKonfiguration; // inlined
  struct Dateiverknuepfung Kachelpfad; // inlined
  struct Beschreibung Beschreibung; // inlined
  std::unique_ptr<struct UTM, zusixml::deleter<struct UTM>> UTM;
  std::vector<std::unique_ptr<struct Huellkurve, zusixml::deleter<struct Huellkurve>>, zusixml::allocator<std::unique_ptr<struct Huellkurve, zusixml::deleter<struct Huellkurve>>>> children_Huellkurve;
  std::unique_ptr<struct Skybox, zusixml::deleter<struct Skybox>> Skybox;
  std::unique_ptr<struct SkyDome, zusixml::deleter<struct SkyDome>> SkyDome;
  std::vector<std::unique_ptr<struct StreckenStandort, zusixml::deleter<struct StreckenStandort>>, zusixml::allocator<std::unique_ptr<struct StreckenStandort, zusixml::deleter<struct StreckenStandort>>>> children_StreckenStandort;
  std::vector<std::unique_ptr<struct ModulDateiVerknuepfung, zusixml::deleter<struct ModulDateiVerknuepfung>>, zusixml::allocator<std::unique_ptr<struct ModulDateiVerknuepfung, zusixml::deleter<struct ModulDateiVerknuepfung>>>> children_ModulDateien;
  std::vector<std::unique_ptr<struct ReferenzElement, zusixml::deleter<struct ReferenzElement>>, zusixml::allocator<std::unique_ptr<struct ReferenzElement, zusixml::deleter<struct ReferenzElement>>>> children_ReferenzElemente;
  std::vector<std::unique_ptr<struct StrElement, zusixml::deleter<struct StrElement>>, zusixml::allocator<std::unique_ptr<struct StrElement, zusixml::deleter<struct StrElement>>>> children_StrElement;
  std::vector<std::unique_ptr<struct Fahrstrasse, zusixml::deleter<struct Fahrstrasse>>, zusixml::allocator<std::unique_ptr<struct Fahrstrasse, zusixml::deleter<struct Fahrstrasse>>>> children_Fahrstrasse;
};
struct AutorEintrag {
  /** Die durch die Firma Hölscher vergebene Autoren-ID. Autor-IDs mit negativen Werten werden an Programme vergeben, die selbstständig ohne großen menschlichen Input Zusi-Inhalte generieren.*/
  int32_t AutorID;
  /** Der Name des Autors. Bei Autoren mit AutorenID der Firma Hölscher muss die genaue Schreibweise des AutorNamens mit Firma Hölscher abgestimmt sein, da dort eine Plausibilitätsprüfung stattfindet (= passt die ID zum Namen?)*/
  std::basic_string<char, std::char_traits<char>, zusixml::allocator<char>> AutorName;
  /** Optionale Angabe einer Mailadresse des Autors.*/
  std::basic_string<char, std::char_traits<char>, zusixml::allocator<char>> AutorEmail;
  /** Der zur Erstellung des Objekts benötigte Zeitaufwand, gemessen in 'Haus-Einheiten'. 1 Haus-Einheit entspricht dem Zeitaufwand zur Erstellung eines durchschnittlichen 3D-Hauses.*/
  float AutorAufwand;
  /** Lizenz, unter der das Objekt steht und die die Verteilung der Einnahmen aus der kommerziellen Nutzung der Zusi-Objekte regelt. Es gilt: 0 = Einnahmen an den Autor [Standard], 1 = Einnahmen auf alle anderen Autoren im Pool verteilen, 2 = Einnahmen gehen an Firma Carsten Hölscher Software, 3 = Objekt ist in der Public Domain (Autor verzichtet auf seine Autorenrechte), 4 = Private nicht zur Veröffentlichung vorgesehene Datei, 5 = Kommerzielle Sondernutzung*/
  int32_t AutorLizenz;
  /** Dient vor allem bei Gemeinschaftsproduktionen der näheren Beschreibung, was genau der Autor zum Objekt beigetragen hat.*/
  std::basic_string<char, std::char_traits<char>, zusixml::allocator<char>> AutorBeschreibung;
};
struct Info {
  std::basic_string<char, std::char_traits<char>, zusixml::allocator<char>> DateiTyp;
  std::basic_string<char, std::char_traits<char>, zusixml::allocator<char>> Version;
  /** MinVersion und Version müssen im aktuellen ls3-Dateiformat immer 'A.1' sein. Das ältere A.0-Dateiformat ist nicht Gegenstand dieser xsd-Datei. A.0 wird seit April 2007 von den Zusi-Editoren nicht mehr neu geschrieben und sollte deshalb nicht mehr in freier Wildbahn vorkommen.*/
  std::basic_string<char, std::char_traits<char>, zusixml::allocator<char>> MinVersion;
  /** Eine Nummer für das Objekt. Das Attribut wird derzeit normalerweise nicht genutzt. Hier könnte es sein, dass zukünftig eine zentral vorgegebene Nummerierung eingeführt wird.*/
  int32_t ObjektID;
  /** Freitext-Beschreibung des Landschaftsobjekts, die von Editoren ggfs. dem Benutzer angezeigt werden kann. Mehrere Zeilen werden mit Semikolon getrennt.*/
  std::basic_string<char, std::char_traits<char>, zusixml::allocator<char>> Beschreibung;
  /** EinsatzAb und EinsatzBis können dem Benutzer Hinweise geben, in welcher Epoche das Objekt zeitlich sinnvoll einsetzbar ist.*/
  struct tm EinsatzAb;
  /** EinsatzAb und EinsatzBis können dem Benutzer Hinweise geben, in welcher Epoche das Objekt zeitlich sinnvoll einsetzbar ist.*/
  struct tm EinsatzBis;
  /** Frei vergebene Kategorisierung der Datei. Das Attribut wird derzeit normalerweise nicht genutzt. Für einheitliche Vorgaben zur Kategorisierung ist zukünftig die bei Zusi 3 mitgelieferte Datei _Setup/categories.txt vorgesehen.*/
  std::basic_string<char, std::char_traits<char>, zusixml::allocator<char>> DateiKategorie;
  std::vector<std::unique_ptr<struct AutorEintrag, zusixml::deleter<struct AutorEintrag>>, zusixml::allocator<std::unique_ptr<struct AutorEintrag, zusixml::deleter<struct AutorEintrag>>>> children_AutorEintrag;
  /** Beschreibt Dateien, die von dieser Datei benötigt werden, aber nicht in Knoten vom Typ "Dateiverknuepfung" auftauchen (sondern beispielsweise in Textfeldern von Ereignissen).*/
  std::vector<std::unique_ptr<struct Dateiverknuepfung, zusixml::deleter<struct Dateiverknuepfung>>, zusixml::allocator<std::unique_ptr<struct Dateiverknuepfung, zusixml::deleter<struct Dateiverknuepfung>>>> children_Datei;
};
struct StrModul {
  struct Dateiverknuepfung Datei; // inlined
  struct Vec3 p; // inlined
  struct Vec3 phi; // inlined
};
struct FahrplanZugVerknuepfung {
  struct Dateiverknuepfung Datei; // inlined
};
struct Fahrplan {
  struct tm AnfangsZeit;
  int32_t Zeitmodus;
  struct Dateiverknuepfung BefehlsKonfiguration; // inlined
  struct Dateiverknuepfung Begruessungsdatei; // inlined
  std::vector<std::unique_ptr<struct FahrplanZugVerknuepfung, zusixml::deleter<struct FahrplanZugVerknuepfung>>, zusixml::allocator<std::unique_ptr<struct FahrplanZugVerknuepfung, zusixml::deleter<struct FahrplanZugVerknuepfung>>>> children_Zug;
  std::vector<std::unique_ptr<struct StrModul, zusixml::deleter<struct StrModul>>, zusixml::allocator<std::unique_ptr<struct StrModul, zusixml::deleter<struct StrModul>>>> children_StrModul;
  std::unique_ptr<struct UTM, zusixml::deleter<struct UTM>> UTM;
};
struct Zusi {
  std::unique_ptr<struct Info, zusixml::deleter<struct Info>> Info;
  std::unique_ptr<struct Strecke, zusixml::deleter<struct Strecke>> Strecke;
  std::unique_ptr<struct Fahrplan, zusixml::deleter<struct Fahrplan>> Fahrplan;
};
struct PZ80Zugdaten {
  int32_t IndusiZugart;
};
struct Sound {
  bool dreiD;
  bool Loop;
  bool Autostart;
  int32_t PosAnlauf;
  int32_t PosAuslauf;
  float Lautstaerke;
  bool GeschwAendern;
  float MinRadius;
  float MaxRadius;
  struct Dateiverknuepfung Datei; // inlined
};
struct Stufe {
  float StufenWert;
};
struct Pkt {
  float PktX;
  float PktY;
};
struct Kennfeld {
  std::basic_string<char, std::char_traits<char>, zusixml::allocator<char>> xText;
  std::basic_string<char, std::char_traits<char>, zusixml::allocator<char>> yText;
  std::basic_string<char, std::char_traits<char>, zusixml::allocator<char>> Beschreibung;
  std::vector<std::unique_ptr<struct Pkt, zusixml::deleter<struct Pkt>>, zusixml::allocator<std::unique_ptr<struct Pkt, zusixml::deleter<struct Pkt>>>> children_Pkt;
  std::vector<std::unique_ptr<struct Stufe, zusixml::deleter<struct Stufe>>, zusixml::allocator<std::unique_ptr<struct Stufe, zusixml::deleter<struct Stufe>>>> children_Stufe;
};
struct Abhaengigkeit {
  int32_t PhysikGroesse;
  bool LautstaerkeAbh;
  bool SoundGeschwAbh;
  int32_t SoundOperator;
  int32_t Trigger;
  float TriggerGrenze;
  std::unique_ptr<struct Kennfeld, zusixml::deleter<struct Kennfeld>> Kennfeld;
};
struct SoundMitAbhaengigkeit {
  std::unique_ptr<struct Sound, zusixml::deleter<struct Sound>> Sound;
  std::vector<std::unique_ptr<struct Abhaengigkeit, zusixml::deleter<struct Abhaengigkeit>>, zusixml::allocator<std::unique_ptr<struct Abhaengigkeit, zusixml::deleter<struct Abhaengigkeit>>>> children_Abhaengigkeit;
};
struct Quaternion {
  float W;
  float X;
  float Y;
  float Z;
};
struct IndusiAnalogZugdaten {
  int32_t IndusiZugart;
  int32_t ZugsicherungHS;
  int32_t Lufthahn;
  int32_t PZBStoerschalter;
};
