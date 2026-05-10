#include "view/fahrstrassendetailswindow.h"

#include "model/fahrstrasse.h"
#include "model/fahrstrassendetails.h"
#include "streckennetz.h"

#include "zusi_parser/utils.hpp"
#include "zusi_parser/zusi_types.hpp"

#include "ls3render/ls3render.h"

#include <QGroupBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QListWidget>
#include <QListWidgetItem>
#include <QMutex>
#include <QMutexLocker>
#include <QPixmap>
#include <QScrollArea>
#include <QSignalBlocker>
#include <QSizePolicy>
#include <QSplitter>
#include <QStringList>
#include <QThread>
#include <QVBoxLayout>
#include <QVector>

#include <array>
#include <atomic>
#include <cstdint>
#include <cstring>
#include <list>
#include <optional>
#include <string>
#include <string_view>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

namespace {

// Globaler Mutex, der ALLE ls3render_*-Aufrufe serialisiert. ls3render hält global state
// (eine GLFW-/OpenGL-Szene); paralleles Rendering ist nicht zulässig.
QMutex& ls3renderMutex() {
    static QMutex mutex;
    return mutex;
}

// Wird einmalig (bei erster Nutzung) initialisiert. Danach ohne neues Init-/Cleanup.
bool& ls3renderInitialisiert() {
    static bool flag = false;
    return flag;
}

constexpr int Ls3PixelProMeter = 40;

// Maximale Anzahl an Einträgen im Bilder-LRU-Cache (siehe Ls3BildCache).
constexpr size_t Ls3BildCacheKapazitaet = 30;

/**
 * Beschreibt einen einzelnen <SignalFrame>-Aufruf (Parameter von
 * ls3render_AddSignalFrame): den OS-Pfad der LS3-Datei und die 9 Float-
 * Parameter (Position p.x/y/z, Rotation phi.x/y/z, Skalierung sk.x/y/z).
 */
struct Ls3FrameKey {
    std::string osPfad;
    std::array<float, 9> params;

    bool operator==(const Ls3FrameKey& other) const noexcept {
        return osPfad == other.osPfad && params == other.params;
    }
};

/**
 * Vollständiger Cache-Schlüssel für ein gerendertes Bild: enthält den
 * Parameter von ls3render_SetSignalbild (Signalbild-ID) und alle Parameter
 * der zugehörigen ls3render_AddSignalFrame-Aufrufe in genau der Reihenfolge,
 * in der sie an ls3render übergeben werden – nur dieselbe Reihenfolge ergibt
 * dasselbe Bild.
 */
struct Ls3CacheKey {
    uint64_t signalbild = 0;
    std::vector<Ls3FrameKey> frames;

    bool operator==(const Ls3CacheKey& other) const noexcept {
        return signalbild == other.signalbild && frames == other.frames;
    }
};

struct Ls3CacheKeyHash {
    static void hashCombine(size_t& seed, size_t v) noexcept {
        seed ^= v + 0x9e3779b97f4a7c15ULL + (seed << 6) + (seed >> 2);
    }

    size_t operator()(const Ls3CacheKey& k) const noexcept {
        size_t h = std::hash<uint64_t>{}(k.signalbild);
        for (const auto& f : k.frames) {
            hashCombine(h, std::hash<std::string>{}(f.osPfad));
            for (float v : f.params) {
                // Float bitweise hashen, damit -0.0/+0.0 und NaN deterministisch
                // behandelt werden (Bitmuster-Identität entspricht der
                // operator==-Semantik von std::array<float,9>).
                uint32_t bits = 0;
                std::memcpy(&bits, &v, sizeof(bits));
                hashCombine(h, std::hash<uint32_t>{}(bits));
            }
        }
        return h;
    }
};

/**
 * LRU-Cache für von ls3render erzeugte Bilder, mit konstanter maximaler
 * Eintragszahl. Synchronisiert intern über einen QMutex – sowohl Lese- als
 * auch Schreibzugriffe modifizieren die LRU-Reihenfolge, daher reicht ein
 * exklusiver Mutex (und ein RW-Lock brächte hier keinen Vorteil).
 */
class Ls3BildCache {
public:
    explicit Ls3BildCache(size_t kapazitaet) : m_kapazitaet(kapazitaet) {}

    /**
     * Liefert das gecachte Bild zum Schlüssel und schiebt den Eintrag an die
     * LRU-Spitze, oder std::nullopt bei Cache-Miss.
     */
    std::optional<QImage> hole(const Ls3CacheKey& key) {
        QMutexLocker lock(&m_mutex);
        const auto it = m_index.find(key);
        if (it == m_index.end()) return std::nullopt;
        m_eintraege.splice(m_eintraege.begin(), m_eintraege, it->second);
        return it->second->image;
    }

    /**
     * Fügt ein Bild unter dem gegebenen Schlüssel ein bzw. überschreibt einen
     * vorhandenen Eintrag. Verdrängt bei Überschreitung der Kapazität die
     * jeweils ältesten Einträge.
     */
    void einfuegen(Ls3CacheKey key, QImage image) {
        QMutexLocker lock(&m_mutex);
        auto it = m_index.find(key);
        if (it != m_index.end()) {
            it->second->image = std::move(image);
            m_eintraege.splice(m_eintraege.begin(), m_eintraege, it->second);
            return;
        }
        m_eintraege.push_front({std::move(key), std::move(image)});
        m_index.emplace(m_eintraege.front().key, m_eintraege.begin());
        while (m_eintraege.size() > m_kapazitaet) {
            m_index.erase(m_eintraege.back().key);
            m_eintraege.pop_back();
        }
    }

private:
    struct Eintrag {
        Ls3CacheKey key;
        QImage image;
    };

    QMutex m_mutex;
    size_t m_kapazitaet;
    std::list<Eintrag> m_eintraege;  // vorne: zuletzt verwendet; hinten: am ältesten
    std::unordered_map<Ls3CacheKey, std::list<Eintrag>::iterator, Ls3CacheKeyHash> m_index;
};

Ls3BildCache& ls3BildCache() {
    static Ls3BildCache cache(Ls3BildCacheKapazitaet);
    return cache;
}

/**
 * Liest die Signalbild-ID aus der Signalmatrix bzw. Ersatzsignal-Matrix anhand der
 * im FahrstrasseDetailEintrag gespeicherten Indizes.
 * Bei Fehler (Matrix leer, Index ausserhalb, MatrixEintrag fehlt) wird std::nullopt
 * zurueckgegeben und *fehler mit einem aussagekraeftigen Text gefuellt.
 */
std::optional<uint64_t> berechneSignalbild(const Signal& sig, int zeile, int spalte,
                                           bool ersatz, QString* fehler) {
    if (ersatz) {
        if (zeile < 0 || static_cast<size_t>(zeile) >= sig.children_Ersatzsignal.size()) {
            *fehler = QObject::tr("Ersatzsignal-Zeile %1 außerhalb (0-%2)")
                          .arg(zeile)
                          .arg(static_cast<int>(sig.children_Ersatzsignal.size()));
            return std::nullopt;
        }
        const auto& es = sig.children_Ersatzsignal[zeile];
        if (!es) {
            *fehler = QObject::tr("Ersatzsignal-Zeile %1 leer").arg(zeile);
            return std::nullopt;
        }
        // Ersatzsignal::MatrixEintrag ist ein Wert (InlineChildStrategy im Parsergen),
        // also kein unique_ptr und damit immer vorhanden.
        return static_cast<uint64_t>(es->MatrixEintrag.Signalbild);
    }

    const size_t anzSpalten = sig.children_VsigBegriff.size();
    if (anzSpalten == 0) {
        *fehler = QObject::tr("Signal hat keine Vorsignal-Spalten (VsigBegriff)");
        return std::nullopt;
    }
    if (zeile < 0 || spalte < 0 || static_cast<size_t>(spalte) >= anzSpalten) {
        *fehler = QObject::tr("Matrix-Index Zeile %1 / Spalte %2 außerhalb (0-%3)")
                      .arg(zeile).arg(spalte).arg(static_cast<int>(anzSpalten - 1));
        return std::nullopt;
    }
    const size_t idx = static_cast<size_t>(zeile) * anzSpalten + static_cast<size_t>(spalte);
    if (idx >= sig.children_MatrixEintrag.size()) {
        *fehler = QObject::tr("Matrix-Eintrag %1 außerhalb (Matrix hat %2 Einträge)")
                      .arg(static_cast<int>(idx))
                      .arg(static_cast<int>(sig.children_MatrixEintrag.size()));
        return std::nullopt;
    }
    // Signal::children_MatrixEintrag enthaelt MatrixEintrag-Werte (InlineChildStrategy).
    const auto& me = sig.children_MatrixEintrag[idx];
    return static_cast<uint64_t>(me.Signalbild);
}

/**
 * Ergebnis einer Referenz-Auflösung: das aufgelöste StreckenelementUndRichtung
 * sowie der Modulpfad, in dem es liegt. Implicit-Bool gibt an, ob die Auflösung
 * geklappt hat (das StreckenelementUndRichtung also einen gültigen Pointer trägt).
 */
struct AufloesungMitModul {
    StreckenelementUndRichtung er;
    zusixml::ZusiPfad modul = zusixml::ZusiPfad::vonZusiPfad("");

    explicit operator bool() const { return static_cast<bool>(er); }
};

/**
 * Löst eine FahrstrStart/FahrstrZiel/FahrstrSignal/FahrstrVSignal/KoppelSignal-
 * artige Referenz (Modul-Datei + Index in children_ReferenzElemente) auf das
 * bezeichnete StreckenelementUndRichtung auf, samt Modulpfad des Ziels (für
 * weitergehende Auflösungen, z. B. KoppelSignal-Verkettung). Liefert ein leeres
 * Ergebnis bei jeglichem Auflöse-Fehler – Fehler werden hier nicht propagiert,
 * weil die Funktion nur zur Identitätsbildung bzw. zum Builder-internen Lookup
 * verwendet wird.
 *
 * Sonderfall leerer `dateinameRef` -> dieselbe Datei wie das referenzierende
 * Modul (gleiche Semantik wie `loeseReferenzAuf` in fahrstrassendetails.cpp).
 */
AufloesungMitModul loeseReferenzAuf(const Streckennetz& netz,
                                    const zusixml::ZusiPfad& parentModul,
                                    std::string_view dateinameRef,
                                    int64_t refIndex) {
    const auto modulPfad = dateinameRef.empty()
        ? parentModul
        : zusixml::ZusiPfad::vonZusiPfad(dateinameRef, parentModul);
    const Strecke* strecke = netz.get(modulPfad);
    if (!strecke) return { {}, modulPfad };
    if (refIndex < 0 || static_cast<size_t>(refIndex) >= strecke->children_ReferenzElemente.size()) {
        return { {}, modulPfad };
    }
    const auto& refEl = strecke->children_ReferenzElemente[refIndex];
    if (!refEl) return { {}, modulPfad };
    const auto strElIndex = refEl->StrElement;
    const auto richtung = static_cast<StreckenelementRichtung>(refEl->StrNorm);
    if (strElIndex < 0 || static_cast<size_t>(strElIndex) >= strecke->children_StrElement.size()) {
        return { {}, modulPfad };
    }
    const auto* strEl = strecke->children_StrElement[strElIndex].get();
    if (!strEl) return { {}, modulPfad };
    return { StreckenelementUndRichtung{ strEl, richtung }, modulPfad };
}

/** Eintrag der Hauptsignal-Liste einer Fahrstraße fürs Vorgänger-Matching. */
struct VorgaengerSignalInfo {
    int matrixZeile;
    bool ersatz;
};

/**
 * Sammelt alle FahrstrSignal-Einträge der gegebenen Fahrstraße und liefert eine
 * Map vom Signal-Standort (StreckenelementUndRichtung-val) auf die zugehörigen
 * Matrix-Indizes (Zeile + Ersatzsignal-Flag). Nicht auflösbare Einträge werden
 * übersprungen.
 */
std::unordered_map<intptr_t, VorgaengerSignalInfo> sammleVorgaengerHauptsignale(
        const Streckennetz& netz, const ResolvedFahrstrasse& fs) {
    std::unordered_map<intptr_t, VorgaengerSignalInfo> result;
    if (!fs.quelle) return result;
    for (const auto& s : fs.quelle->children_FahrstrSignal) {
        if (!s) continue;
        const auto res = loeseReferenzAuf(netz, fs.fahrstrasseModul,
                                          s->Datei.Dateiname, s->Ref);
        if (!res) continue;
        result[res.er.val] = { s->FahrstrSignalZeile, s->FahrstrSignalErsatzsignal };
    }
    return result;
}

/** Eintrag der Vorsignal-Liste einer Fahrstraße fürs Nachfolger-Matching. */
struct NachfolgerSignalInfo {
    int matrixSpalte;
};

std::unordered_map<intptr_t, NachfolgerSignalInfo> sammleNachfolgerVorsignale(
        const Streckennetz& netz, const ResolvedFahrstrasse& fs) {
    std::unordered_map<intptr_t, NachfolgerSignalInfo> result;
    if (!fs.quelle) return result;
    for (const auto& s : fs.quelle->children_FahrstrVSignal) {
        if (!s) continue;
        const auto res = loeseReferenzAuf(netz, fs.fahrstrasseModul,
                                          s->Datei.Dateiname, s->Ref);
        if (!res) continue;
        result[res.er.val] = { s->FahrstrSignalSpalte };
    }
    return result;
}

/**
 * Liefert den Index der Spalte mit Vorsignalgeschwindigkeit 0 (entspricht
 * "Halt erwarten") in der VsigBegriff-Liste des Signals, oder -1, falls
 * kein solcher VsigBegriff vorhanden ist.
 *
 * Hinweis: Ein `<VsigBegriff/>`-Eintrag ohne `VsigGeschw`-Attribut wird vom
 * Parser auf 0.0f initialisiert und zählt damit als "Halt erwarten"-Spalte.
 */
int findeVsigGeschwNullSpalte(const Signal& sig) {
    for (size_t i = 0; i < sig.children_VsigBegriff.size(); ++i) {
        const auto& vb = sig.children_VsigBegriff[i];
        if (!vb) continue;
        if (vb->VsigGeschw == 0.0f) {
            return static_cast<int>(i);
        }
    }
    return -1;
}

/**
 * Liefert den Index der Zeile mit Hauptsignalgeschwindigkeit 0 (entspricht
 * "Halt") in der HsigBegriff-Liste des Signals, oder -1, falls kein solcher
 * HsigBegriff vorhanden ist. Analog zu findeVsigGeschwNullSpalte.
 */
int findeHsigGeschwNullZeile(const Signal& sig) {
    for (size_t i = 0; i < sig.children_HsigBegriff.size(); ++i) {
        const auto& hb = sig.children_HsigBegriff[i];
        if (!hb) continue;
        if (hb->HsigGeschw == 0.0f) {
            return static_cast<int>(i);
        }
    }
    return -1;
}

/** Liefert das am Standort `er` montierte Signal, oder nullptr. */
const Signal* signalAn(const StreckenelementUndRichtung& er) {
    if (!er) return nullptr;
    const auto& info = er.richtungsInfo();
    if (!info.has_value() || !info->Signal) return nullptr;
    return info->Signal.get();
}

/**
 * Berechnet die Signalstellungen im Streckennetz,
 * wenn eine oder mehrere Fahrstraßen nacheinander eingestellt werden.
 */
class FahrstrassenEinstellung {
public:
    struct Stellung {
        int zeile = 0;
        int spalte = 0;
        bool ersatz = false;
    };

    using StellungenMap = std::unordered_map<StreckenelementUndRichtung, Stellung>;

    explicit FahrstrassenEinstellung(const Streckennetz& netz) : m_netz(netz) {}

    const StellungenMap& stellungen() const {
        return m_stellungen;
    }

    void fahrstrasseEinstellen(const ResolvedFahrstrasse& fs) {
        if (!fs.quelle) return;
        const auto& fahrstrasseModul = fs.fahrstrasseModul;

        // Pfad-Mitgliedschaft kumulieren: Startelement explizit, Pfad enthält
        // bereits das Zielelement (siehe model/fahrstrasse.cpp). Wird unten in
        // Schritt 2 als Filter für Vorsignale verwendet.
        if (fs.start) m_eingestellteElemente.insert(fs.start);
        for (const auto& e : fs.pfad) {
            if (e) m_eingestellteElemente.insert(e);
        }

        // Schritt 1: alle Hauptsignale stellen.
        for (const auto& s : fs.quelle->children_FahrstrSignal) {
            if (!s) continue;
            const auto res = loeseReferenzAuf(m_netz, fahrstrasseModul,
                                              s->Datei.Dateiname, s->Ref);
            if (!res) continue;
            std::unordered_set<intptr_t> besucht;
            setzeHsig(res.er, res.modul,
                      s->FahrstrSignalZeile, s->FahrstrSignalErsatzsignal,
                      besucht);
        }

        // Stelle keine Vorsignale, wenn Startsignal (in Zusi: letztes Hauptsignal) auf Ersatzsignal steht.
        const auto letztesHsigIt = std::find_if(
            fs.quelle->children_FahrstrSignal.rbegin(),
            fs.quelle->children_FahrstrSignal.rend(),
            [&] (const auto& ptr) { return ptr != nullptr; }
        );
        if (letztesHsigIt != fs.quelle->children_FahrstrSignal.rend() && (*letztesHsigIt)->FahrstrSignalErsatzsignal) {
            return;
        }

        // Schritt 2: Vorsignale stellen, aber nur an Standorten, deren Element
        // zur aktuellen oder einer zuvor eingestellten Fahrstraße gehört
        // (m_eingestellteElemente enthält Pfad inkl. Start- und Zielelement).
        // Das simuliert die Logik von Zusi, Vorsignale nur innerhalb gestellter
        // Fahrstraßen zu suchen.
        for (const auto& s : fs.quelle->children_FahrstrVSignal) {
            if (!s) continue;
            const auto res = loeseReferenzAuf(m_netz, fahrstrasseModul,
                                              s->Datei.Dateiname, s->Ref);
            if (!res) continue;
            if (m_eingestellteElemente.find(res.er) == m_eingestellteElemente.end()) continue;
            std::unordered_set<intptr_t> besucht;
            setzeVsig(res.er, res.modul, s->FahrstrSignalSpalte, besucht);
        }
    }

private:
    // Aktuelle Stellung am Standort `er`: gespeicherte Stellung, sonst
    // Grundstellung (Zeile = HsigGeschw==0, sonst 0; Spalte = VsigGeschw==0,
    // sonst 0; Ersatz = false). Liefert eine Default-Stellung (0,0,false),
    // wenn am Standort kein Signal montiert ist. Wird ausschließlich
    // builder-intern benutzt, um vor einer (Teil-)Aktualisierung den
    // bisherigen Wert für unveränderte Komponenten zu erhalten.
    Stellung aktuelleStellung(const StreckenelementUndRichtung& er) const {
        const auto it = m_stellungen.find(er);
        if (it != m_stellungen.end()) return it->second;
        Stellung s;
        const Signal* signal = signalAn(er);
        if (!signal) return s;
        const int z = findeHsigGeschwNullZeile(*signal);
        s.zeile = (z >= 0) ? z : 0;
        const int sp = findeVsigGeschwNullSpalte(*signal);
        s.spalte = (sp >= 0) ? sp : 0;
        return s;
    }

    // Setzt am Standort `er` Zeile und Ersatz auf die übergebenen Werte; die
    // Spalte bleibt unverändert (Grundstellungs-Spalte falls noch nie berührt).
    // Folgt rekursiv der KoppelSignal-Kette und setzt jedes Koppelsignal auf
    // dieselbe (jetzt aktualisierte) Zeile/Spalte/Ersatz – analog zur Zusi-
    // Semantik. Zykluserkennung über `besucht`.
    void setzeHsig(const StreckenelementUndRichtung& er,
                   const zusixml::ZusiPfad& modul,
                   int zeile, bool ersatz,
                   std::unordered_set<intptr_t>& besucht) {
        if (!er) return;
        Stellung neu = aktuelleStellung(er);
        neu.zeile = zeile;
        neu.ersatz = ersatz;
        setzeStellung(er, modul, neu, besucht);
    }

    // Setzt am Standort `er` die Spalte; Zeile/Ersatz bleiben unverändert.
    // Sonst analog zu setzeHsig.
    void setzeVsig(const StreckenelementUndRichtung& er,
                   const zusixml::ZusiPfad& modul,
                   int spalte,
                   std::unordered_set<intptr_t>& besucht) {
        if (!er) return;
        Stellung neu = aktuelleStellung(er);
        neu.spalte = spalte;
        setzeStellung(er, modul, neu, besucht);
    }

    // Schreibt die komplette Stellung an einen Standort und propagiert sie
    // rekursiv auf alle gekoppelten Signale (KoppelSignal-Kette). Setzt
    // konsistent dieselben (Zeile, Spalte, Ersatz)-Werte für jedes Glied
    // der Kette.
    void setzeStellung(const StreckenelementUndRichtung& er,
                       const zusixml::ZusiPfad& modul,
                       Stellung neu,
                       std::unordered_set<intptr_t>& besucht) {
        if (!er) return;
        if (!besucht.insert(er.val).second) return;
        m_stellungen[er] = neu;

        const Signal* signal = signalAn(er);
        if (!signal || !signal->KoppelSignal) return;
        const auto& koppel = *signal->KoppelSignal;
        const auto res = loeseReferenzAuf(m_netz, modul,
                                          koppel.Datei.Dateiname,
                                          static_cast<int64_t>(koppel.ReferenzNr));
        if (!res) return;
        setzeStellung(res.er, res.modul, neu, besucht);
    }

    const Streckennetz& m_netz;
    StellungenMap m_stellungen;
    // Kumulierte Menge aller Streckenelemente, die zu einer der bisher per
    // fahrstrasseEinstellen() einbezogenen Fahrstraßen gehören (inkl. deren
    // Start- und Zielelementen). Wird für die Filterung von Vorsignalen
    // herangezogen: ein Vorsignal wird nur gestellt, wenn sein Element zu
    // einer aktuell oder zuvor eingestellten Fahrstraße gehört.
    std::unordered_set<StreckenelementUndRichtung> m_eingestellteElemente;
};

}  // namespace

class FahrstrassenDetailsLs3Worker : public QObject
{
    Q_OBJECT
public:
    explicit FahrstrassenDetailsLs3Worker(std::atomic<int>* generationRef)
        : m_generationRef(generationRef) {}

public slots:
    /**
     * Rendert einen Signal-Slot, dessen Frames als parallele Listen übergeben
     * werden:
     *   - osPfade[i]: OS-Pfad der LS3-Datei des i-ten Frames
     *   - params[9*i + 0..2]: Position (p.x/y/z) aus <SignalFrame>
     *   - params[9*i + 3..5]: Rotation (phi.x/y/z) aus <SignalFrame>
     *   - params[9*i + 6..8]: Skalierung (sk.x/y/z) aus <SignalFrame>
     *
     * `slotKey` wird unverändert mit dem Ergebnis zurückgegeben und dient zur
     * eindeutigen Adressierung des Ziel-Slots im Aufrufer (mehrere Varianten
     * desselben Eintrags ergeben verschiedene Slots).
     *
     * Die parallelen Listen werden verwendet, weil Qt-Standardtypen ohne weiteres
     * über Qt::QueuedConnection übergeben werden können (kein Q_DECLARE_METATYPE).
     */
    void renderEinzeln(int generation, int slotKey,
                       QStringList osPfade, QVector<float> params, quint64 signalbild) {
        if (m_generationRef->load() != generation) {
            return;
        }

        Q_ASSERT(params.size() == osPfade.size() * 9);

        // Cache-Schlüssel umfasst alle Parameter, die in dieser Render-Session an
        // ls3render_SetSignalbild und ls3render_AddSignalFrame übergeben werden.
        Ls3CacheKey cacheKey;
        cacheKey.signalbild = static_cast<uint64_t>(signalbild);
        cacheKey.frames.reserve(static_cast<size_t>(osPfade.size()));
        for (int i = 0; i < osPfade.size(); ++i) {
            Ls3FrameKey fk;
            fk.osPfad = osPfade.at(i).toStdString();
            const float* p = params.constData() + i * 9;
            for (int j = 0; j < 9; ++j) fk.params[j] = p[j];
            cacheKey.frames.push_back(std::move(fk));
        }

        // Cache-Lookup ohne ls3renderMutex – Cache-Hits sollen nicht unnötig
        // gegen laufende Rendervorgänge serialisiert werden.
        if (auto cached = ls3BildCache().hole(cacheKey)) {
            emit fertig(generation, slotKey, *cached, QString());
            return;
        }

        QImage image;
        QString fehler;

        QMutexLocker lock(&ls3renderMutex());
        if (!ls3renderInitialisiert()) {
            if (ls3render_Init() == 1) {
                ls3render_SetMultisampling(4);
                ls3render_SetPixelProMeter(Ls3PixelProMeter);
                ls3render_SetAxonometrieParameter(0.1f, 0.25f);
                ls3renderInitialisiert() = true;
            } else {
                fehler = QObject::tr("ls3render_Init fehlgeschlagen");
            }
        }

        if (fehler.isEmpty()) {
            ls3render_Reset();
            ls3render_SetSignalbild(static_cast<uint64_t>(signalbild));

            int hinzugefuegt = 0;
            for (int i = 0; i < osPfade.size(); ++i) {
                const QByteArray utf8 = osPfade.at(i).toUtf8();
                const float* p = params.constData() + i * 9;
                if (ls3render_AddSignalFrame(utf8.constData(),
                                             p[0], p[1], p[2],
                                             p[3], p[4], p[5],
                                             p[6], p[7], p[8]) == 1) {
                    ++hinzugefuegt;
                }
            }

            if (hinzugefuegt == 0) {
                fehler = QObject::tr("ls3render_AddSignalFrame fehlgeschlagen für alle %1 Frame(s)")
                             .arg(osPfade.size());
            } else {
                const int bufsize = ls3render_GetAusgabepufferGroesse();
                if (bufsize <= 0) {
                    fehler = QObject::tr("Nichts zu rendern");
                } else {
                    QByteArray buf;
                    buf.resize(bufsize);
                    if (ls3render_Render(buf.data()) != 1) {
                        fehler = QObject::tr("ls3render_Render fehlgeschlagen");
                    } else {
                        const int w = ls3render_GetBildbreite();
                        const int h = ls3render_GetBildhoehe();
                        // ls3render liefert 32-Bit-Pixeldaten in BGRA-Byte-Reihenfolge
                        // (siehe ls3render.h: glReadPixels mit GL_BGRA), zeilenweise
                        // von unten nach oben. QImage::Format_ARGB32 ist als
                        // 0xAARRGGBB-Integer in nativer Byte-Reihenfolge definiert,
                        // entspricht auf Little-Endian-Systemen also genau B,G,R,A
                        // im Speicher. mirrored(false, true) dreht die Zeilen um und
                        // erzeugt eine eigene Kopie, sodass `buf` nach Rückkehr
                        // verworfen werden darf.
                        QImage tmp(reinterpret_cast<const uchar*>(buf.constData()),
                                   w, h, w * 4, QImage::Format_ARGB32);
                        image = tmp.mirrored(false, true);
                    }
                }
            }
            ls3render_Reset();
        }
        lock.unlock();

        // Bei einem Renderfehler wird nichts gecacht (insbesondere damit ein
        // transienter Fehler nicht dauerhaft zugeordnet bleibt).
        if (fehler.isEmpty()) {
            ls3BildCache().einfuegen(std::move(cacheKey), image);
        }

        emit fertig(generation, slotKey, image, fehler);
    }

signals:
    void fertig(int generation, int slotKey, QImage image, QString fehler);

private:
    std::atomic<int>* m_generationRef;
};

FahrstrassenDetailsWindow::FahrstrassenDetailsWindow(QWidget* parent)
    : QWidget(parent, Qt::Window)
    , m_eintragsListe(new QListWidget(this))
    , m_vorgaengerListe(new QListWidget(this))
    , m_nachfolgerListe(new QListWidget(this))
    , m_signalScroll(new QScrollArea(this))
    , m_signalContainer(new QWidget(this))
    , m_signalLayout(new QHBoxLayout(m_signalContainer))
{
    setWindowTitle(tr("Fahrstraßen-Details"));
    resize(1100, 600);

    auto* zentralLayout = new QVBoxLayout(this);
    zentralLayout->setContentsMargins(4, 4, 4, 4);

    auto* hauptSplitter = new QSplitter(Qt::Horizontal, this);
    hauptSplitter->setChildrenCollapsible(false);
    zentralLayout->addWidget(hauptSplitter, 1);

    // Linker Bereich: Elementliste
    auto* linksBox = new QGroupBox(tr("Elemente"), this);
    auto* linksLayout = new QVBoxLayout(linksBox);
    linksLayout->setContentsMargins(4, 4, 4, 4);
    m_eintragsListe->setSelectionMode(QAbstractItemView::SingleSelection);
    linksLayout->addWidget(m_eintragsListe);
    hauptSplitter->addWidget(linksBox);

    // Rechter Bereich: oben Splitter mit Vorgänger/Nachfolger, unten Signal-Vorschau
    auto* rechtsSplitter = new QSplitter(Qt::Vertical, this);
    rechtsSplitter->setChildrenCollapsible(false);

    auto* obenSplitter = new QSplitter(Qt::Horizontal, this);
    obenSplitter->setChildrenCollapsible(false);
    auto* vorBox = new QGroupBox(tr("Vorgänger-Fahrstraßen"), this);
    auto* vorLayout = new QVBoxLayout(vorBox);
    vorLayout->setContentsMargins(4, 4, 4, 4);
    vorLayout->addWidget(m_vorgaengerListe);
    obenSplitter->addWidget(vorBox);
    auto* nachBox = new QGroupBox(tr("Nachfolger-Fahrstraßen"), this);
    auto* nachLayout = new QVBoxLayout(nachBox);
    nachLayout->setContentsMargins(4, 4, 4, 4);
    nachLayout->addWidget(m_nachfolgerListe);
    obenSplitter->addWidget(nachBox);
    rechtsSplitter->addWidget(obenSplitter);

    auto* signalBox = new QGroupBox(tr("Signalstellungen"), this);
    auto* signalBoxLayout = new QVBoxLayout(signalBox);
    signalBoxLayout->setContentsMargins(4, 4, 4, 4);
    m_signalScroll->setWidgetResizable(true);
    m_signalLayout->setContentsMargins(4, 4, 4, 4);
    m_signalLayout->setSpacing(8);
    m_signalLayout->addStretch(1);
    m_signalScroll->setWidget(m_signalContainer);
    signalBoxLayout->addWidget(m_signalScroll);
    rechtsSplitter->addWidget(signalBox);

    rechtsSplitter->setStretchFactor(0, 0);
    rechtsSplitter->setStretchFactor(1, 1);
    hauptSplitter->addWidget(rechtsSplitter);
    hauptSplitter->setStretchFactor(0, 0);
    hauptSplitter->setStretchFactor(1, 1);

    connect(m_eintragsListe, &QListWidget::itemSelectionChanged,
            this, &FahrstrassenDetailsWindow::onEintragAusgewaehlt);
    connect(m_eintragsListe, &QListWidget::itemDoubleClicked,
            this, &FahrstrassenDetailsWindow::onEintragDoppelklick);
    connect(m_vorgaengerListe, &QListWidget::itemSelectionChanged,
            this, &FahrstrassenDetailsWindow::onVorgaengerNachfolgerAusgewaehlt);
    connect(m_nachfolgerListe, &QListWidget::itemSelectionChanged,
            this, &FahrstrassenDetailsWindow::onVorgaengerNachfolgerAusgewaehlt);

    // QVector<float> ist kein Qt-Standard-Meta-Typ; einmalig registrieren, damit
    // er via Qt::QueuedConnection ge-Q_ARG()-fähig ist.
    qRegisterMetaType<QVector<float>>("QVector<float>");

    // Worker-Thread
    m_workerThread = new QThread(this);
    m_worker = new FahrstrassenDetailsLs3Worker(&m_generation);
    m_worker->moveToThread(m_workerThread);
    connect(m_workerThread, &QThread::finished, m_worker, &QObject::deleteLater);
    connect(m_worker, &FahrstrassenDetailsLs3Worker::fertig, this,
            [this](int generation, int slotKey, QImage image, QString fehler) {
                if (generation != m_generation.load()) {
                    return;
                }
                onLs3Fertig(slotKey, std::move(image), std::move(fehler));
            });
    m_workerThread->start();
}

FahrstrassenDetailsWindow::~FahrstrassenDetailsWindow()
{
    m_generation.fetch_add(1);
    if (m_workerThread) {
        m_workerThread->quit();
        m_workerThread->wait();
    }
}

const FahrstrasseDetailEintrag* FahrstrassenDetailsWindow::aktuellerEintrag() const
{
    const auto* item = m_eintragsListe->currentItem();
    if (!item) {
        return nullptr;
    }
    const int idx = item->data(Qt::UserRole).toInt();
    if (idx < 0 || static_cast<size_t>(idx) >= m_eintraege.size()) {
        return nullptr;
    }
    return &m_eintraege[idx];
}

void FahrstrassenDetailsWindow::zeigeFahrstrasse(const Streckennetz* netz,
                                                 const std::vector<ResolvedFahrstrasse>* alleFahrstrassen,
                                                 int aktiverIndex)
{
    m_generation.fetch_add(1);

    m_netz = netz;
    m_alleFahrstrassen = alleFahrstrassen;
    m_aktiverIndex = aktiverIndex;

    const ResolvedFahrstrasse* fs = nullptr;
    if (netz && alleFahrstrassen
            && aktiverIndex >= 0
            && static_cast<size_t>(aktiverIndex) < alleFahrstrassen->size()) {
        fs = &(*alleFahrstrassen)[aktiverIndex];
    }

    if (!fs) {
        m_eintraege.clear();
        setWindowTitle(tr("Fahrstraßen-Details"));
        aktualisiereEintraege();
        aktualisiereVorgaengerNachfolger();
        stoppeLs3Rendering();
        return;
    }

    if (!fs->fehler.empty() || !fs->quelle) {
        m_eintraege.clear();
        if (!fs->fehler.empty()) {
            setWindowTitle(tr("Fahrstraßen-Details — %1  (%2)")
                               .arg(QString::fromStdString(fs->name))
                               .arg(QString::fromStdString(fs->fehler)));
        } else {
            setWindowTitle(tr("Fahrstraßen-Details — %1")
                               .arg(QString::fromStdString(fs->name)));
        }
        aktualisiereEintraege();
        aktualisiereVorgaengerNachfolger();
        stoppeLs3Rendering();
        return;
    }

    setWindowTitle(tr("Fahrstraßen-Details — %1")
                       .arg(QString::fromStdString(fs->name)));

    m_eintraege = ermittleFahrstrasseDetails(*netz, *fs);
    aktualisiereEintraege();
    aktualisiereVorgaengerNachfolger();

    starteLs3Rendering();
}

void FahrstrassenDetailsWindow::aktualisiereVorgaengerNachfolger()
{
    // Liste-Aktualisierung soll keinen Re-Render auslösen – Signal-Block.
    QSignalBlocker blockVor(m_vorgaengerListe);
    QSignalBlocker blockNach(m_nachfolgerListe);

    m_vorgaengerListe->clear();
    m_nachfolgerListe->clear();

    if (!m_alleFahrstrassen
            || m_aktiverIndex < 0
            || static_cast<size_t>(m_aktiverIndex) >= m_alleFahrstrassen->size()) {
        return;
    }
    const auto& aktuell = (*m_alleFahrstrassen)[m_aktiverIndex];
    if (!aktuell.fehler.empty() || !aktuell.quelle) {
        return;
    }

    // Signal-Standorte der aktuellen Fahrstraße einmalig sammeln, um pro
    // Vorgänger-/Nachfolger-Kandidat prüfen zu können, ob er Einfluss auf
    // deren Signalstellungen hat:
    //   - aktuelleHsigStandorte: Standorte der FahrstrSignal-Einträge
    //     (Hauptsignale) der aktuellen Fahrstraße.
    //   - aktuelleVsigStandorte: Standorte der FahrstrVSignal-Einträge
    //     (Vorsignale) der aktuellen Fahrstraße.
    // Eine Vorgänger-FS ist nur dann relevant, wenn mindestens eines ihrer
    // Hauptsignale (FahrstrSignal) als Vorsignal in der aktuellen FS
    // (FahrstrVSignal) vorkommt – analog für Nachfolger (deren Vorsignale
    // müssen Hauptsignale in der aktuellen FS sein).
    const auto aktuelleHsigStandorte = m_netz
        ? sammleVorgaengerHauptsignale(*m_netz, aktuell)
        : std::unordered_map<intptr_t, VorgaengerSignalInfo>{};
    const auto aktuelleVsigStandorte = m_netz
        ? sammleNachfolgerVorsignale(*m_netz, aktuell)
        : std::unordered_map<intptr_t, NachfolgerSignalInfo>{};

    // Sentinel-Einträge: speichern -1 in Qt::UserRole, damit das
    // Standardverhalten leicht unterscheidbar ist.
    auto* alleItem = new QListWidgetItem(tr("(alle)"));
    alleItem->setData(Qt::UserRole, -1);
    m_vorgaengerListe->addItem(alleItem);

    auto* keineItem = new QListWidgetItem(tr("(keine)"));
    keineItem->setData(Qt::UserRole, -1);
    m_nachfolgerListe->addItem(keineItem);

    for (size_t i = 0; i < m_alleFahrstrassen->size(); ++i) {
        if (static_cast<int>(i) == m_aktiverIndex) continue;
        const auto& other = (*m_alleFahrstrassen)[i];
        if (!other.fehler.empty() || !other.quelle) continue;

        if (other.ziel && aktuell.start && other.ziel == aktuell.start) {
            auto* item = new QListWidgetItem(QString::fromStdString(other.name));
            item->setData(Qt::UserRole, static_cast<int>(i));

            // Hat die Vorgänger-FS Einfluss auf die Vorsignale der aktuellen FS?
            bool relevant = false;
            if (m_netz) {
                const auto vorgHsig = sammleVorgaengerHauptsignale(*m_netz, other);
                for (const auto& kv : vorgHsig) {
                    if (aktuelleVsigStandorte.count(kv.first) > 0) {
                        relevant = true;
                        break;
                    }
                }
            }
            if (!relevant) {
                item->setFlags(item->flags() & ~(Qt::ItemIsEnabled | Qt::ItemIsSelectable));
                item->setToolTip(tr("Hat keinen Einfluss auf die Signalstellungen der"
                                    " aktuellen Fahrstraße: keines der Hauptsignale"
                                    " dieser Vorgänger-Fahrstraße kommt als Vorsignal"
                                    " in der aktuellen Fahrstraße vor."));
            }
            m_vorgaengerListe->addItem(item);
        }
        if (other.start && aktuell.ziel && other.start == aktuell.ziel) {
            auto* item = new QListWidgetItem(QString::fromStdString(other.name));
            item->setData(Qt::UserRole, static_cast<int>(i));

            // Hat die Nachfolger-FS Einfluss auf die Hauptsignale der aktuellen FS?
            bool relevant = false;
            if (m_netz) {
                const auto nachVsig = sammleNachfolgerVorsignale(*m_netz, other);
                for (const auto& kv : nachVsig) {
                    if (aktuelleHsigStandorte.count(kv.first) > 0) {
                        relevant = true;
                        break;
                    }
                }
            }
            if (!relevant) {
                item->setFlags(item->flags() & ~(Qt::ItemIsEnabled | Qt::ItemIsSelectable));
                item->setToolTip(tr("Hat keinen Einfluss auf die Signalstellungen der"
                                    " aktuellen Fahrstraße: keines der Vorsignale"
                                    " dieser Nachfolger-Fahrstraße kommt als Hauptsignal"
                                    " in der aktuellen Fahrstraße vor."));
            }
            m_nachfolgerListe->addItem(item);
        }
    }

    // Default-Auswahl: Sentinel-Eintrag (entspricht „bisheriges Verhalten“).
    m_vorgaengerListe->setCurrentRow(0);
    m_nachfolgerListe->setCurrentRow(0);
}

int FahrstrassenDetailsWindow::aktuelleVorgaengerFsIndex() const
{
    const auto* item = m_vorgaengerListe->currentItem();
    if (!item) return -1;
    return item->data(Qt::UserRole).toInt();
}

int FahrstrassenDetailsWindow::aktuelleNachfolgerFsIndex() const
{
    const auto* item = m_nachfolgerListe->currentItem();
    if (!item) return -1;
    return item->data(Qt::UserRole).toInt();
}

void FahrstrassenDetailsWindow::aktualisiereEintraege()
{
    m_eintragsListe->blockSignals(true);
    m_eintragsListe->clear();
    for (size_t i = 0; i < m_eintraege.size(); ++i) {
        const auto& e = m_eintraege[i];
        // Koppelsignale werden entsprechend ihrer Kopplungs-Tiefe eingerückt
        // (1 Tiefe = 4 Leerzeichen).
        QString text = QString::fromStdString(e.label);
        if (e.typ == FahrstrasseDetailEintrag::Typ::Koppelsignal && e.kopplungsTiefe > 0) {
            text = QString(e.kopplungsTiefe * 4, QLatin1Char(' ')) + text;
        }
        auto* item = new QListWidgetItem(text);
        item->setData(Qt::UserRole, static_cast<int>(i));
        if (!e.fehler.empty()) {
            item->setToolTip(QString::fromStdString(e.fehler));
            item->setFlags(item->flags() & ~Qt::ItemIsSelectable);
            QFont font = item->font();
            font.setItalic(true);
            item->setFont(font);
        } else if (!e.beschreibung.empty()) {
            item->setToolTip(QString::fromStdString(e.beschreibung));
        }
        m_eintragsListe->addItem(item);
    }
    m_eintragsListe->blockSignals(false);
}

void FahrstrassenDetailsWindow::onEintragAusgewaehlt()
{
    const auto* item = m_eintragsListe->currentItem();
    if (!item) {
        emit detailAusgewaehlt(-1);
        return;
    }
    const int idx = item->data(Qt::UserRole).toInt();
    if (idx < 0 || static_cast<size_t>(idx) >= m_eintraege.size() || !m_eintraege[idx].fehler.empty()) {
        emit detailAusgewaehlt(-1);
        return;
    }
    emit detailAusgewaehlt(idx);
}

void FahrstrassenDetailsWindow::onEintragDoppelklick(QListWidgetItem* item)
{
    if (!item) {
        return;
    }
    const int idx = item->data(Qt::UserRole).toInt();
    if (idx < 0 || static_cast<size_t>(idx) >= m_eintraege.size() || !m_eintraege[idx].fehler.empty()) {
        return;
    }
    emit detailDoppelklick(idx);
}

void FahrstrassenDetailsWindow::onVorgaengerNachfolgerAusgewaehlt()
{
    // Eine neue Auswahl macht eventuell laufende LS3-Aufträge obsolet (sie haben
    // ggf. die alten Matrix-Indizes verwendet). Generation hochzählen, dann neu
    // aufbauen – bisherige Slots werden in starteLs3Rendering komplett ersetzt.
    m_generation.fetch_add(1);
    starteLs3Rendering();
}

void FahrstrassenDetailsWindow::stoppeLs3Rendering()
{
    // Layout leeren (alle Slots entfernen).
    while (m_signalLayout->count() > 0) {
        QLayoutItem* item = m_signalLayout->takeAt(0);
        if (auto* w = item->widget()) {
            w->deleteLater();
        }
        delete item;
    }
    m_signalLayout->addStretch(1);
    m_signalSlots.clear();
}

std::vector<FahrstrassenDetailsWindow::EintragAnzeige>
FahrstrassenDetailsWindow::berechneEintragAnzeige() const
{
    std::vector<EintragAnzeige> anzeige(m_eintraege.size());

    if (!m_netz || !m_alleFahrstrassen
            || m_aktiverIndex < 0
            || static_cast<size_t>(m_aktiverIndex) >= m_alleFahrstrassen->size()) {
        return anzeige;
    }
    const auto& aktuell = (*m_alleFahrstrassen)[m_aktiverIndex];
    if (!aktuell.fehler.empty() || !aktuell.quelle) {
        return anzeige;
    }

    const int vorgIdx = aktuelleVorgaengerFsIndex();
    const int nachIdx = aktuelleNachfolgerFsIndex();

    const ResolvedFahrstrasse* nachfolger = nullptr;
    if (nachIdx >= 0 && static_cast<size_t>(nachIdx) < m_alleFahrstrassen->size()) {
        const auto& cand = (*m_alleFahrstrassen)[nachIdx];
        if (cand.fehler.empty() && cand.quelle) nachfolger = &cand;
    }

    // Ein einzelner Durchlauf: Vorgänger (optional), aktuell, Nachfolger (optional)
    // einstellen, anschließend für jeden Visualisierungs-Eintrag die
    // resultierende Stellung aus der Builder-Map abgreifen und – sofern noch
    // nicht in der Variantenliste vorhanden – aufnehmen.
    //
    // Signale, die im Durchlauf nicht berührt wurden (also nicht in der Map
    // erscheinen), erzeugen für diesen Durchlauf KEINE Variante – das wird
    // beim Render-Loop dazu führen, dass solche Einträge nicht visualisiert
    // werden (genau das vom Aufrufer gewünschte Verhalten).
    auto erfasseDurchlauf = [&](const ResolvedFahrstrasse* vorgaenger) {
        FahrstrassenEinstellung fe(*m_netz);
        if (vorgaenger) fe.fahrstrasseEinstellen(*vorgaenger);
        fe.fahrstrasseEinstellen(aktuell);
        if (nachfolger) fe.fahrstrasseEinstellen(*nachfolger);
        const auto& stellungen = fe.stellungen();

        for (size_t i = 0; i < m_eintraege.size(); ++i) {
            const auto& e = m_eintraege[i];
            const bool relevant =
                e.typ == FahrstrasseDetailEintrag::Typ::FahrstrSignal
             || e.typ == FahrstrasseDetailEintrag::Typ::FahrstrVSignal
             || e.typ == FahrstrasseDetailEintrag::Typ::Koppelsignal;
            if (!relevant) continue;
            if (!e.elementUndRichtung) continue;

            const auto it = stellungen.find(e.elementUndRichtung);
            if (it == stellungen.end()) continue;
            const auto& st = it->second;

            EintragAnzeige::Variant v;
            v.matrixZeile = st.zeile;
            v.matrixSpalte = st.spalte;
            v.ersatzsignal = st.ersatz;

            // Dieselbe (Zeile, Spalte, Ersatzsignal)-Kombination nur einmal aufnehmen.
            if (std::find_if(
                anzeige[i].varianten.cbegin(),
                anzeige[i].varianten.cend(),
                [&] (const auto& bestehend) {
                    return bestehend.matrixZeile == v.matrixZeile
                        && bestehend.matrixSpalte == v.matrixSpalte
                        && bestehend.ersatzsignal == v.ersatzsignal;
                })
            == anzeige[i].varianten.cend()) {
                anzeige[i].varianten.push_back(v);
            }
        }
    };

    if (vorgIdx >= 0 && static_cast<size_t>(vorgIdx) < m_alleFahrstrassen->size()) {
        // Konkreter Vorgänger ausgewählt: ein einzelner Durchlauf.
        const auto& vorg = (*m_alleFahrstrassen)[vorgIdx];
        const ResolvedFahrstrasse* vorgPtr =
            (vorg.fehler.empty() && vorg.quelle) ? &vorg : nullptr;
        erfasseDurchlauf(vorgPtr);
    } else {
        // Sentinel "(alle)": über alle in der UI gelisteten Vorgänger-Kandidaten
        // iterieren (alle FSen, deren Ziel mit dem Start der aktuellen FS
        // zusammenfällt; entspricht der Liste in m_vorgaengerListe).
        bool mindestensEinDurchlauf = false;
        for (size_t i = 0; i < m_alleFahrstrassen->size(); ++i) {
            if (static_cast<int>(i) == m_aktiverIndex) continue;
            const auto& other = (*m_alleFahrstrassen)[i];
            if (!other.fehler.empty() || !other.quelle) continue;
            if (!(other.ziel && aktuell.start && other.ziel == aktuell.start)) continue;
            erfasseDurchlauf(&other);
            mindestensEinDurchlauf = true;
        }
        // Falls keine Vorgänger-Kandidaten existieren: einmaliger Durchlauf
        // ohne Vorgänger-FS, damit zumindest die Stellungen aus aktueller +
        // (ggf.) Nachfolger-FS sichtbar werden.
        if (!mindestensEinDurchlauf) {
            erfasseDurchlauf(nullptr);
        }

        // Fallback für Vorsignale, die nach allen "(alle)"-Durchläufen leer
        // geblieben sind (typischerweise: Vorsignal-Element gehört zu keiner
        // der einbezogenen Fahrstraßen-Pfade): synthetische Variante mit
        // Grundstellung der Wurzel + der im FahrstrVSignal-Eintrag
        // angegebenen Spalte (entspricht "stelle auf Grundstellung, dann auf
        // angegebene Spalte"). Anschließend dieselbe Variante an alle
        // unmittelbar folgenden, an dieses Vsig gekoppelten Koppelsignal-
        // Einträge weiterreichen (entspricht "stelle auch Koppelsignale").
        for (size_t i = 0; i < m_eintraege.size(); ++i) {
            const auto& e = m_eintraege[i];
            if (e.typ != FahrstrasseDetailEintrag::Typ::FahrstrVSignal) continue;
            if (!anzeige[i].varianten.empty()) continue;
            if (!e.elementUndRichtung || !e.signal) continue;

            EintragAnzeige::Variant v;
            const int z = findeHsigGeschwNullZeile(*e.signal);
            v.matrixZeile = (z >= 0) ? z : 0;
            v.matrixSpalte = e.matrixSpalte;
            v.ersatzsignal = false;

            auto fuegeEinmaligEin = [](EintragAnzeige& a, const EintragAnzeige::Variant& neu) {
                if (std::find_if(a.varianten.cbegin(), a.varianten.cend(),
                        [&](const auto& bestehend) {
                            return bestehend.matrixZeile == neu.matrixZeile
                                && bestehend.matrixSpalte == neu.matrixSpalte
                                && bestehend.ersatzsignal == neu.ersatzsignal;
                        }) == a.varianten.cend()) {
                    a.varianten.push_back(neu);
                }
            };
            fuegeEinmaligEin(anzeige[i], v);

            // Koppel-Kette dieses Vorsignals: gleiche Iteration wie in
            // sammleGruppe (Wurzel-Typ FahrstrVSignal → kopplungZuVsig=true).
            for (size_t k = i + 1; k < m_eintraege.size(); ++k) {
                const auto& kE = m_eintraege[k];
                if (kE.typ != FahrstrasseDetailEintrag::Typ::Koppelsignal) break;
                if (!kE.kopplungZuVsig) break;
                fuegeEinmaligEin(anzeige[k], v);
            }
        }
    }

    return anzeige;
}

void FahrstrassenDetailsWindow::starteLs3Rendering()
{
    stoppeLs3Rendering();

    const auto anzeige = berechneEintragAnzeige();

    // Reihenfolge: erst Vorsignale, dann Hauptsignale (entspricht dem Plan
    // „links nach rechts“; pro Gruppe in Eintragsreihenfolge). Koppelsignale
    // werden jeweils direkt hinter ihrem Wurzelsignal in derselben Gruppe
    // mitgerendert (kopplungZuVsig markiert die Wurzel-Zugehörigkeit).
    //
    // Bei mehreren Varianten pro Wurzel-Eintrag (Vorsignale mit >1 Zeile bei
    // Sentinel "(alle)") werden die Varianten szenenweise interleaved:
    //   Wurzel-Var0, Koppel1-Var0, ..., Wurzel-Var1, Koppel1-Var1, ...
    // So bleibt jede Signal-Stellung (Wurzel + zugehörige Koppelsignale) als
    // zusammenhängender Block sichtbar.
    struct SlotPlan {
        int eintragIndex;
        int variantIndex;
    };
    std::vector<SlotPlan> reihenfolge;
    auto sammleGruppe = [&](FahrstrasseDetailEintrag::Typ wurzelTyp, bool zuVsig) {
        for (size_t i = 0; i < m_eintraege.size(); ++i) {
            if (m_eintraege[i].typ != wurzelTyp) {
                continue;
            }
            // Ende der Koppel-Kette für diesen Wurzel-Eintrag bestimmen.
            size_t kettenEnde = i + 1;
            while (kettenEnde < m_eintraege.size()) {
                const auto& cand = m_eintraege[kettenEnde];
                if (cand.typ != FahrstrasseDetailEintrag::Typ::Koppelsignal) break;
                if (cand.kopplungZuVsig != zuVsig) break;
                ++kettenEnde;
            }
            // Pro Wurzel-Variante alle Kettenglieder mit derselben Varianten-
            // Position zu einem Block zusammenführen (Koppelsignale erben die
            // Variantenliste der Wurzel; daher haben Wurzel und Koppel hier
            // dieselbe Anzahl an Varianten).
            const size_t vCount = anzeige[i].varianten.size();
            for (size_t v = 0; v < vCount; ++v) {
                for (size_t k = i; k < kettenEnde; ++k) {
                    if (v < anzeige[k].varianten.size()) {
                        reihenfolge.push_back({static_cast<int>(k), static_cast<int>(v)});
                    }
                }
            }
        }
    };
    sammleGruppe(FahrstrasseDetailEintrag::Typ::FahrstrVSignal, /*zuVsig=*/true);
    sammleGruppe(FahrstrasseDetailEintrag::Typ::FahrstrSignal, /*zuVsig=*/false);

    if (reihenfolge.empty()) {
        return;
    }

    // Layout aufbauen: Stretch hinten lassen, Slots links davor einfügen.
    // Wir entfernen den Stretch erst und hängen ihn am Ende wieder an.
    while (m_signalLayout->count() > 0) {
        QLayoutItem* item = m_signalLayout->takeAt(0);
        delete item;
    }

    const int gen = m_generation.load();

    for (const SlotPlan& plan : reihenfolge) {
        const auto& e = m_eintraege[plan.eintragIndex];
        const auto& variant = anzeige[plan.eintragIndex].varianten[plan.variantIndex];
        const bool mehrfach = anzeige[plan.eintragIndex].varianten.size() > 1;

        auto* slotWidget = new QWidget(m_signalContainer);
        auto* slotLayout = new QVBoxLayout(slotWidget);
        slotLayout->setContentsMargins(0, 0, 0, 0);
        slotLayout->setSpacing(2);

        auto* bildLabel = new QLabel(slotWidget);
        bildLabel->setMinimumSize(120, 160);
        bildLabel->setAlignment(Qt::AlignCenter);
        bildLabel->setText(tr("Wird gerendert …"));
        bildLabel->setFrameShape(QFrame::StyledPanel);
        slotLayout->addWidget(bildLabel);

        QString caption = QString::fromStdString(e.label);
        // Label hat das Fallback-Präfix; für die Caption die reinen Signal-Daten verwenden.
        if (e.signal) {
            const QString bs = QString::fromStdString(e.signal->NameBetriebsstelle);
            const QString sn = QString::fromStdString(e.signal->Signalname);
            if (!bs.isEmpty() || !sn.isEmpty()) {
                caption = bs;
                if (!bs.isEmpty() && !sn.isEmpty()) caption += QStringLiteral(" ");
                caption += sn;
            }
        }
        // Koppelsignal-Einträge auch in der Visualisierung als solche kennzeichnen.
        if (e.typ == FahrstrasseDetailEintrag::Typ::Koppelsignal) {
            caption = tr("Koppelsignal: %1").arg(caption);
        }
        // Bei mehreren Varianten pro Eintrag die Variante in der Caption
        // kennzeichnen, damit die Slots unterscheidbar bleiben:
        //  - Hauptsignale (inkl. Hsig-gekoppelte Koppelsignale) variieren bei
        //    Sentinel "(alle)" typischerweise in der Spalte → "(Spalte X)".
        //  - Vorsignale (inkl. Vsig-gekoppelte Koppelsignale) variieren in der
        //    Zeile bzw. dem Ersatzsignal-Flag → "(Zeile X)" / "(Ersatzsignal-Zeile X)".
        if (mehrfach) {
            const bool istHsigArt =
                e.typ == FahrstrasseDetailEintrag::Typ::FahrstrSignal
             || (e.typ == FahrstrasseDetailEintrag::Typ::Koppelsignal && !e.kopplungZuVsig);
            QString suffix;
            if (istHsigArt) {
                suffix = tr(" (Spalte %1)").arg(variant.matrixSpalte);
            } else {
                suffix = variant.ersatzsignal
                    ? tr(" (Ersatzsignal-Zeile %1)").arg(variant.matrixZeile)
                    : tr(" (Zeile %1)").arg(variant.matrixZeile);
            }
            caption += suffix;
        }
        auto* textLabel = new QLabel(caption, slotWidget);
        textLabel->setAlignment(Qt::AlignCenter);
        textLabel->setWordWrap(true);
        slotLayout->addWidget(textLabel);

        m_signalLayout->addWidget(slotWidget);

        SignalSlot slot;
        slot.eintragIndex = plan.eintragIndex;
        slot.variantIndex = plan.variantIndex;
        slot.bildLabel = bildLabel;
        slot.textLabel = textLabel;
        m_signalSlots.push_back(slot);
    }

    m_signalLayout->addStretch(1);

    // Aufträge dispatchen: pro Slot werden alle <SignalFrame>-Einträge mit
    // nicht-leerer LS3-Datei gerendert (Multi-Frame-Signale). Der Slot-Key
    // ist die Position in m_signalSlots, damit unterschiedliche Varianten
    // desselben Eintrags eindeutig adressierbar sind.
    for (size_t s = 0; s < m_signalSlots.size(); ++s) {
        const SignalSlot& slot = m_signalSlots[s];
        const auto& e = m_eintraege[slot.eintragIndex];
        if (!e.signal) {
            slot.bildLabel->setText(tr("(kein Signal)"));
            continue;
        }

        // Signalbild aus der Signalmatrix bzw. Ersatzsignal-Matrix bestimmen.
        // Indizes wurden in berechneEintragAnzeige() je Variante festgelegt.
        const auto& variant = anzeige[slot.eintragIndex].varianten[slot.variantIndex];
        QString signalbildFehler;
        const auto signalbild = berechneSignalbild(*e.signal,
                                                   variant.matrixZeile, variant.matrixSpalte,
                                                   variant.ersatzsignal, &signalbildFehler);
        if (!signalbild.has_value()) {
            slot.bildLabel->setText(signalbildFehler);
            slot.bildLabel->setToolTip(signalbildFehler);
            continue;
        }

        QStringList osPfade;
        QVector<float> params;
        params.reserve(static_cast<int>(e.signal->children_SignalFrame.size()) * 9);
        for (const auto& frame : e.signal->children_SignalFrame) {
            if (!frame) continue;
            osPfade.append(frame->Datei.Dateiname.empty() ?
                QString{} :
                QString::fromStdString(zusixml::ZusiPfad::vonZusiPfad(frame->Datei.Dateiname, e.signalModul).alsOsPfad()));
            params.append(frame->p.x);    params.append(frame->p.y);    params.append(frame->p.z);
            params.append(frame->phi.x);  params.append(frame->phi.y);  params.append(frame->phi.z);
            params.append(frame->sk.x);   params.append(frame->sk.y);   params.append(frame->sk.z);
        }
        if (osPfade.isEmpty()) {
            slot.bildLabel->setText(tr("(keine LS3-Datei)"));
            continue;
        }

        // QueuedConnection: Worker liegt in eigenem Thread.
        QMetaObject::invokeMethod(m_worker, "renderEinzeln", Qt::QueuedConnection,
                                  Q_ARG(int, gen),
                                  Q_ARG(int, static_cast<int>(s)),
                                  Q_ARG(QStringList, osPfade),
                                  Q_ARG(QVector<float>, params),
                                  Q_ARG(quint64, static_cast<quint64>(*signalbild)));
    }
}

void FahrstrassenDetailsWindow::onLs3Fertig(int slotKey, QImage image, QString fehler)
{
    if (slotKey < 0 || static_cast<size_t>(slotKey) >= m_signalSlots.size()) {
        return;
    }
    const SignalSlot& slot = m_signalSlots[slotKey];
    if (!fehler.isEmpty()) {
        slot.bildLabel->setText(fehler);
        slot.bildLabel->setToolTip(fehler);
    } else if (image.isNull()) {
        slot.bildLabel->setText(tr("(leeres Bild)"));
    } else {
        QPixmap pm = QPixmap::fromImage(image);
        // Auf eine handhabbare Höhe skalieren.
        const int maxH = 220;
        if (pm.height() > maxH) {
            pm = pm.scaledToHeight(maxH, Qt::SmoothTransformation);
        }
        slot.bildLabel->setPixmap(pm);
        slot.bildLabel->setToolTip({});
        slot.bildLabel->setMinimumSize(pm.size().expandedTo(QSize(120, 160)));
    }
}

#include "fahrstrassendetailswindow.moc"
