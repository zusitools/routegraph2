#pragma once
#include "boost/container/small_vector.hpp"
#include <vector>  // for std::vector
#include <memory>  // for std::unique_ptr
#include <optional>// for std::optional
#include <ctime>   // for struct tm
struct ArgbColor {
  uint8_t a, r, g, b;
};
struct FahrplanZugVerknuepfung;
struct Fahrplan;
struct VsigBegriff;
struct HsigBegriff;
struct KoppelSignal;
struct FahrstrVSignal;
struct Pkt;
struct NachfolgerSelbesModul;
struct Stufe;
struct Kennfeld;
struct FahrstrSignal;
struct AutorEintrag;
struct SkyDome;
struct FahrstrSigHaltfall;
struct Info;
struct Beschreibung;
struct Fahrstrasse;
struct FahrstrStart;
struct Vec2;
struct Zusi;
struct StreckenStandort;
struct FahrstrTeilaufloesung;
struct Abhaengigkeit;
struct Dateiverknuepfung;
struct Ersatzsignal;
struct UTM;
struct SoundMitAbhaengigkeit;
struct FahrstrWeiche;
struct Huellkurve;
struct SignalFrame;
struct Quaternion;
struct Sound;
struct Vec3;
struct Strecke;
struct StrModul;
struct Ereignis;
struct LoeschFahrstrasse;
struct Skybox;
struct Signal;
struct ModulDateiVerknuepfung;
struct FahrstrRegister;
struct ReferenzElement;
struct FahrstrZiel;
struct FahrstrAufloesung;
struct StrElement;
struct StreckenelementRichtungsInfo;
struct MatrixEintrag;
struct NachfolgerAnderesModul;
struct Vec2 {
  float X;
  float Y;
};
struct UTM {
  int32_t UTM_WE;
  int32_t UTM_NS;
  int32_t UTM_Zone;
  std::string UTM_Zone2;
};
struct Dateiverknuepfung {
  /** Dateipfade sind entweder
- Pfade relativ zum Zusi-Datenverzeichnis (RollingStock\Deutschland\Lok.ls3), wobei ein führender Backslash erlaubt ist und ignoriert wird, oder
- wenn sie keinen Backslash enthalten, Dateien im selben Verzeichnis wie die Datei, in der der Pfad steht.
Insbesondere können Dateien, die auf einem anderen Laufwerk als das Zusi-Datenverzeichnis liegen, nicht referenziert werden.*/
  std::string Dateiname;
  bool NurInfo;
};
struct FahrplanZugVerknuepfung {
  struct Dateiverknuepfung Datei; // inlined: size = 33
};
struct Vec3 {
  float X;
  float Y;
  float Z;
};
struct StrModul {
  struct Dateiverknuepfung Datei; // inlined: size = 33
  struct Vec3 p; // inlined: size = 12
  struct Vec3 phi; // inlined: size = 12
};
struct Fahrplan {
  struct tm AnfangsZeit;
  int32_t Zeitmodus;
  struct Dateiverknuepfung BefehlsKonfiguration; // inlined: size = 33
  struct Dateiverknuepfung Begruessungsdatei; // inlined: size = 33
  std::vector<std::unique_ptr<struct FahrplanZugVerknuepfung>> children_Zug;
  std::vector<std::unique_ptr<struct StrModul>> children_StrModul;
  std::unique_ptr<struct UTM> UTM;
};
struct AutorEintrag {
  /** Die durch die Firma Hölscher vergebene Autoren-ID. Autor-IDs mit negativen Werten werden an Programme vergeben, die selbstständig ohne großen menschlichen Input Zusi-Inhalte generieren.*/
  int32_t AutorID;
  /** Der Name des Autors. Bei Autoren mit AutorenID der Firma Hölscher muss die genaue Schreibweise des AutorNamens mit Firma Hölscher abgestimmt sein, da dort eine Plausibilitätsprüfung stattfindet (= passt die ID zum Namen?)*/
  std::string AutorName;
  /** Optionale Angabe einer Mailadresse des Autors.*/
  std::string AutorEmail;
  /** Der zur Erstellung des Objekts benötigte Zeitaufwand, gemessen in 'Haus-Einheiten'. 1 Haus-Einheit entspricht dem Zeitaufwand zur Erstellung eines durchschnittlichen 3D-Hauses.*/
  float AutorAufwand;
  /** Lizenz, unter der das Objekt steht und die die Verteilung der Einnahmen aus der kommerziellen Nutzung der Zusi-Objekte regelt. Es gilt: 0 = Einnahmen an den Autor [Standard], 1 = Einnahmen auf alle anderen Autoren im Pool verteilen, 2 = Einnahmen gehen an Firma Carsten Hölscher Software, 3 = Objekt ist in der Public Domain (Autor verzichtet auf seine Autorenrechte), 4 = Private nicht zur Veröffentlichung vorgesehene Datei, 5 = Kommerzielle Sondernutzung*/
  int32_t AutorLizenz;
  /** Dient vor allem bei Gemeinschaftsproduktionen der näheren Beschreibung, was genau der Autor zum Objekt beigetragen hat.*/
  std::string AutorBeschreibung;
};
struct Info {
  std::string DateiTyp;
  std::string Version;
  /** MinVersion und Version müssen im aktuellen ls3-Dateiformat immer 'A.1' sein. Das ältere A.0-Dateiformat ist nicht Gegenstand dieser xsd-Datei. A.0 wird seit April 2007 von den Zusi-Editoren nicht mehr neu geschrieben und sollte deshalb nicht mehr in freier Wildbahn vorkommen.*/
  std::string MinVersion;
  /** Eine Nummer für das Objekt. Das Attribut wird derzeit normalerweise nicht genutzt. Hier könnte es sein, dass zukünftig eine zentral vorgegebene Nummerierung eingeführt wird.*/
  int32_t ObjektID;
  /** Freitext-Beschreibung des Landschaftsobjekts, die von Editoren ggfs. dem Benutzer angezeigt werden kann. Mehrere Zeilen werden mit Semikolon getrennt.*/
  std::string Beschreibung;
  /** EinsatzAb und EinsatzBis können dem Benutzer Hinweise geben, in welcher Epoche das Objekt zeitlich sinnvoll einsetzbar ist.*/
  struct tm EinsatzAb;
  /** EinsatzAb und EinsatzBis können dem Benutzer Hinweise geben, in welcher Epoche das Objekt zeitlich sinnvoll einsetzbar ist.*/
  struct tm EinsatzBis;
  /** Frei vergebene Kategorisierung der Datei. Das Attribut wird derzeit normalerweise nicht genutzt. Für einheitliche Vorgaben zur Kategorisierung ist zukünftig die bei Zusi 3 mitgelieferte Datei _Setup/categories.txt vorgesehen.*/
  std::string DateiKategorie;
  std::vector<std::unique_ptr<struct AutorEintrag>> children_AutorEintrag;
  /** Beschreibt Dateien, die von dieser Datei benötigt werden, aber nicht in Knoten vom Typ "Dateiverknuepfung" auftauchen (sondern beispielsweise in Textfeldern von Ereignissen).*/
  std::vector<std::unique_ptr<struct Dateiverknuepfung>> children_Datei;
};
struct Huellkurve {
  std::vector<std::unique_ptr<struct Vec3>> children_PunktXYZ;
};
struct Beschreibung {
  std::string Beschreibung;
};
struct FahrstrSignal {
  int32_t Ref;
  int32_t FahrstrSignalZeile;
  bool FahrstrSignalErsatzsignal;
  struct Dateiverknuepfung Datei; // inlined: size = 33
};
struct FahrstrWeiche {
  int32_t Ref;
  int32_t FahrstrWeichenlage;
  struct Dateiverknuepfung Datei; // inlined: size = 33
};
struct FahrstrVSignal {
  int32_t Ref;
  int32_t FahrstrSignalSpalte;
  struct Dateiverknuepfung Datei; // inlined: size = 33
};
struct FahrstrStart {
  int32_t Ref;
  struct Dateiverknuepfung Datei; // inlined: size = 33
};
struct FahrstrTeilaufloesung {
  int32_t Ref;
  struct Dateiverknuepfung Datei; // inlined: size = 33
};
struct FahrstrSigHaltfall {
  int32_t Ref;
  struct Dateiverknuepfung Datei; // inlined: size = 33
};
struct FahrstrRegister {
  int32_t Ref;
  struct Dateiverknuepfung Datei; // inlined: size = 33
};
struct FahrstrZiel {
  int32_t Ref;
  struct Dateiverknuepfung Datei; // inlined: size = 33
};
struct FahrstrAufloesung {
  int32_t Ref;
  struct Dateiverknuepfung Datei; // inlined: size = 33
};
struct Fahrstrasse {
  std::string FahrstrName;
  std::string FahrstrStrecke;
  int32_t RglGgl;
  std::string FahrstrTyp;
  float ZufallsWert;
  float Laenge;
  std::unique_ptr<struct FahrstrStart> FahrstrStart;
  std::unique_ptr<struct FahrstrZiel> FahrstrZiel;
  std::vector<std::unique_ptr<struct FahrstrRegister>> children_FahrstrRegister;
  std::vector<std::unique_ptr<struct FahrstrAufloesung>> children_FahrstrAufloesung;
  std::vector<std::unique_ptr<struct FahrstrTeilaufloesung>> children_FahrstrTeilaufloesung;
  std::vector<std::unique_ptr<struct FahrstrSigHaltfall>> children_FahrstrSigHaltfall;
  std::vector<std::unique_ptr<struct FahrstrWeiche>> children_FahrstrWeiche;
  std::vector<std::unique_ptr<struct FahrstrSignal>> children_FahrstrSignal;
  std::vector<std::unique_ptr<struct FahrstrVSignal>> children_FahrstrVSignal;
};
struct StreckenStandort {
  std::string StrInfo;
  struct Vec3 p; // inlined: size = 12
  struct Vec3 lookat; // inlined: size = 12
  struct Vec3 up; // inlined: size = 12
};
struct SkyDome {
  struct Dateiverknuepfung HimmelTex; // inlined: size = 33
  struct Dateiverknuepfung SonneTex; // inlined: size = 33
  struct Dateiverknuepfung SonneHorizontTex; // inlined: size = 33
  struct Dateiverknuepfung MondTex; // inlined: size = 33
  struct Dateiverknuepfung SternTex; // inlined: size = 33
};
struct LoeschFahrstrasse {
  std::string FahrstrName;
};
struct Skybox {
  struct Dateiverknuepfung Skybox0; // inlined: size = 33
  struct Dateiverknuepfung Skybox1; // inlined: size = 33
  struct Dateiverknuepfung Skybox2; // inlined: size = 33
  struct Dateiverknuepfung Skybox3; // inlined: size = 33
  struct Dateiverknuepfung Skybox4; // inlined: size = 33
  struct Dateiverknuepfung Skybox5; // inlined: size = 33
};
struct ModulDateiVerknuepfung {
  struct Dateiverknuepfung Datei; // inlined: size = 33
};
struct ReferenzElement {
  int32_t ReferenzNr;
  int32_t StrElement;
  bool StrNorm;
  int32_t RefTyp;
  std::string Info;
};
struct NachfolgerSelbesModul {
  int32_t Nr;
};
struct Ereignis {
  int32_t Er;
  std::string Beschr;
  float Wert;
};
struct MatrixEintrag {
  int64_t Signalbild;
  float MatrixGeschw;
  int32_t SignalID;
  std::vector<std::unique_ptr<struct Ereignis>> children_Ereignis;
};
struct Ersatzsignal {
  std::string ErsatzsigBezeichnung;
  int32_t ErsatzsigID;
  bool SigAssiErgaenzen;
  struct MatrixEintrag MatrixEintrag; // inlined: size = 40
};
struct SignalFrame {
  int32_t WeichenbaugruppeIndex;
  int32_t WeichenbaugruppeNr;
  std::string WeichenbaugruppeBeschreibung;
  bool WeichenbaugruppePos0;
  bool WeichenbaugruppePos1;
  struct Vec3 p; // inlined: size = 12
  struct Vec3 phi; // inlined: size = 12
  struct Dateiverknuepfung Datei; // inlined: size = 33
  std::vector<std::unique_ptr<struct Ereignis>> children_Ereignis;
};
struct VsigBegriff {
  float VsigGeschw;
};
struct HsigBegriff {
  float HsigGeschw;
  int64_t FahrstrTyp;
};
struct KoppelSignal {
  int32_t ReferenzNr;
  struct Dateiverknuepfung Datei; // inlined: size = 33
};
struct Signal {
  std::string NameBetriebsstelle;
  std::string Stellwerk;
  std::string Signalname;
  float ZufallsWert;
  int32_t SignalFlags;
  int32_t SignalTyp;
  int32_t Weichenbauart;
  int32_t WeichenGrundstellung;
  int32_t BoundingR;
  float Zwangshelligkeit;
  struct Vec3 p; // inlined: size = 12
  struct Vec3 phi; // inlined: size = 12
  std::unique_ptr<struct KoppelSignal> KoppelSignal;
  std::vector<std::unique_ptr<struct SignalFrame>> children_SignalFrame;
  std::vector<std::unique_ptr<struct HsigBegriff>> children_HsigBegriff;
  std::vector<std::unique_ptr<struct VsigBegriff>> children_VsigBegriff;
  std::vector<std::unique_ptr<struct MatrixEintrag>> children_MatrixEintrag;
  std::vector<std::unique_ptr<struct Ersatzsignal>> children_Ersatzsignal;
};
struct StreckenelementRichtungsInfo {
  float vMax;
  float km;
  bool pos;
  int32_t Reg;
  int32_t KoppelWeicheNr;
  bool KoppelWeicheNorm;
  std::vector<std::unique_ptr<struct Ereignis>> children_Ereignis;
  std::unique_ptr<struct Signal> Signal;
};
struct NachfolgerAnderesModul {
  int32_t Nr;
  struct Dateiverknuepfung Datei; // inlined: size = 33
};
struct StrElement {
  int32_t Nr;
  float Ueberh;
  float kr;
  float spTrass;
  int32_t Anschluss;
  int64_t Fkt;
  std::string Oberbau;
  int32_t Volt;
  float Drahthoehe;
  float Zwangshelligkeit;
  struct Vec3 g; // inlined: size = 12
  struct Vec3 b; // inlined: size = 12
  std::optional<struct StreckenelementRichtungsInfo> InfoNormRichtung;
  std::optional<struct StreckenelementRichtungsInfo> InfoGegenRichtung;
  boost::container::small_vector<struct NachfolgerSelbesModul, 2> children_NachNorm;
  boost::container::small_vector<struct NachfolgerSelbesModul, 2> children_NachGegen;
  boost::container::small_vector<struct NachfolgerAnderesModul, 2> children_NachNormModul;
  boost::container::small_vector<struct NachfolgerAnderesModul, 2> children_NachGegenModul;

  std::vector<StrElement*> nachfolgerElementeNorm = {};
  std::vector<StrElement*> nachfolgerElementeGegen = {};
};
struct Strecke {
  int32_t RekTiefe;
  int32_t Himmelsmodell;
  /** Die zur Strecke gehörende Landschaftsdatei.*/
  struct Dateiverknuepfung Datei; // inlined: size = 33
  std::vector<std::unique_ptr<struct LoeschFahrstrasse>> children_LoeschFahrstrasse;
  struct Dateiverknuepfung HintergrundDatei; // inlined: size = 33
  struct Dateiverknuepfung BefehlsKonfiguration; // inlined: size = 33
  struct Dateiverknuepfung Kachelpfad; // inlined: size = 33
  struct Beschreibung Beschreibung; // inlined: size = 32
  std::unique_ptr<struct UTM> UTM;
  std::vector<std::unique_ptr<struct Huellkurve>> children_Huellkurve;
  std::unique_ptr<struct Skybox> Skybox;
  std::unique_ptr<struct SkyDome> SkyDome;
  std::vector<std::unique_ptr<struct StreckenStandort>> children_StreckenStandort;
  std::vector<std::unique_ptr<struct ModulDateiVerknuepfung>> children_ModulDateien;
  std::vector<std::unique_ptr<struct ReferenzElement>> children_ReferenzElemente;
  std::vector<std::unique_ptr<struct StrElement>> children_StrElement;
  std::vector<std::unique_ptr<struct Fahrstrasse>> children_Fahrstrasse;
};
struct Zusi {
  std::unique_ptr<struct Info> Info;
  std::unique_ptr<struct Strecke> Strecke;
  std::unique_ptr<struct Fahrplan> Fahrplan;
};
struct Stufe {
  float StufenWert;
};
struct Pkt {
  float PktX;
  float PktY;
};
struct Kennfeld {
  std::string xText;
  std::string yText;
  std::string Beschreibung;
  std::vector<std::unique_ptr<struct Pkt>> children_Pkt;
  std::vector<std::unique_ptr<struct Stufe>> children_Stufe;
};
struct Abhaengigkeit {
  int32_t PhysikGroesse;
  bool LautstaerkeAbh;
  bool SoundGeschwAbh;
  int32_t SoundOperator;
  int32_t Trigger;
  float TriggerGrenze;
  std::unique_ptr<struct Kennfeld> Kennfeld;
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
  struct Dateiverknuepfung Datei; // inlined: size = 33
};
struct SoundMitAbhaengigkeit {
  std::unique_ptr<struct Sound> Sound;
  std::vector<std::unique_ptr<struct Abhaengigkeit>> children_Abhaengigkeit;
};
struct Quaternion {
  float W;
  float X;
  float Y;
  float Z;
};
