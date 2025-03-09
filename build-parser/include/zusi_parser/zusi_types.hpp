#pragma once
#include "zusi_parser/zusi_types_fwd.hpp"
#include "boost/container/small_vector.hpp"
#include <array>   // for std::array
#include <cmath>   // for std::hypot
#include <vector>  // for std::vector
#include <memory>  // for std::unique_ptr
#include <optional>// for std::optional
#include <string>  // for std::string
#include <ctime>   // for struct tm
struct ArgbColor {
  uint8_t a, r, g, b;
};
namespace zusixml {
  template <typename T>
  using allocator = std::allocator<T>;
  template <typename T>
  using deleter = std::default_delete<T>;
}
struct Dateiverknuepfung {
  /** Dateipfade sind entweder
- Pfade relativ zum Zusi-Datenverzeichnis (RollingStock\Deutschland\Lok.ls3), wobei ein führender Backslash erlaubt ist und ignoriert wird, oder
- wenn sie keinen Backslash enthalten, Dateien im selben Verzeichnis wie die Datei, in der der Pfad steht.
Insbesondere können Dateien, die auf einem anderen Laufwerk als das Zusi-Datenverzeichnis liegen, nicht referenziert werden.*/
  std::basic_string<char, std::char_traits<char>, zusixml::allocator<char>> Dateiname;
};
struct StrModul {
 struct Dateiverknuepfung Datei; // inlined
};
struct Fahrplan {
  std::vector<std::unique_ptr<struct StrModul, zusixml::deleter<struct StrModul>>, zusixml::allocator<std::unique_ptr<struct StrModul, zusixml::deleter<struct StrModul>>>> children_StrModul;
};
struct ReferenzElement {
  int32_t ReferenzNr;
  int32_t StrElement;
  bool StrNorm;
  int32_t RefTyp;
  std::basic_string<char, std::char_traits<char>, zusixml::allocator<char>> Info;
};
struct NachfolgerAnderesModul {
  int64_t Nr;
 struct Dateiverknuepfung Datei; // inlined
};
struct NachfolgerSelbesModul {
  int32_t Nr;
};
struct Signal {
  std::basic_string<char, std::char_traits<char>, zusixml::allocator<char>> NameBetriebsstelle;
  std::basic_string<char, std::char_traits<char>, zusixml::allocator<char>> Stellwerk;
  std::basic_string<char, std::char_traits<char>, zusixml::allocator<char>> Signalname;
  int32_t SignalTyp;
};
struct StreckenelementRichtungsInfo {
  float vMax;
  std::unique_ptr<struct Signal, zusixml::deleter<struct Signal>> Signal;
};
struct Vec3 {
  float X;
  float Y;
  float Z;
};
struct StrElement {
  int32_t Nr;
  float Ueberh;
  float kr;
  int32_t Anschluss;
  int64_t Fkt;
  std::basic_string<char, std::char_traits<char>, zusixml::allocator<char>> Oberbau;
  int32_t Volt;
  float Drahthoehe;
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
  bool nachfolgerElementeNormSindInAnderemModul { false };
  bool nachfolgerElementeGegenSindInAnderemModul { false };
  float Neigung() const {
    if (!NeigungCache) {
      NeigungCache = (b.Z - g.Z) / std::hypot(b.X - g.X, b.Y - g.Y, b.Z - g.Z);
    }
    return *NeigungCache;
  }};
struct UTM {
  int32_t UTM_WE;
  int32_t UTM_NS;
  int32_t UTM_Zone;
};
struct Strecke {
  std::unique_ptr<struct UTM, zusixml::deleter<struct UTM>> UTM;
  std::vector<std::unique_ptr<struct ReferenzElement, zusixml::deleter<struct ReferenzElement>>, zusixml::allocator<std::unique_ptr<struct ReferenzElement, zusixml::deleter<struct ReferenzElement>>>> children_ReferenzElemente;
  std::vector<std::unique_ptr<struct StrElement, zusixml::deleter<struct StrElement>>, zusixml::allocator<std::unique_ptr<struct StrElement, zusixml::deleter<struct StrElement>>>> children_StrElement;
};
struct Zusi {
  std::unique_ptr<struct Fahrplan, zusixml::deleter<struct Fahrplan>> Fahrplan;
  std::unique_ptr<struct Strecke, zusixml::deleter<struct Strecke>> Strecke;
};
