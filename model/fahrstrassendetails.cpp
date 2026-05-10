#include "model/fahrstrassendetails.h"

#include "streckennetz.h"

#include "zusi_parser/zusi_types.hpp"

#include <cmath>
#include <optional>
#include <sstream>
#include <string_view>
#include <unordered_set>
#include <utility>

namespace {

const char* richtungBeschriftung(StreckenelementRichtung richtung) {
    return richtung == StreckenelementRichtung::Norm ? "blau" : "grün";
}

/** Ergebnis einer Referenz-Auflösung. */
struct AufloesungsErgebnis {
    StreckenelementUndRichtung elementUndRichtung;
    zusixml::ZusiPfad modul = zusixml::ZusiPfad::vonZusiPfad("");
    std::string fehler;  // leer = OK
};

/**
 * Identisch zur Hilfsfunktion in fahrstrasse.cpp; zusätzlich liefert sie den
 * Modulpfad, weil wir ihn fürs Anhängen relativer LS3-Pfade brauchen.
 *
 * Sonderfall: ein leerer `dateinameRef` bedeutet "im selben Modul wie die
 * referenzierende Datei" (so verwendet z. B. <KoppelSignal> die leere
 * Dateiverknüpfung für modul-interne Kopplungen). Da
 * `ZusiPfad::vonZusiPfad("", parent)` den Elternpfad NICHT zurückliefert,
 * sondern den leeren Pfad, behandeln wir den Fall hier explizit.
 */
AufloesungsErgebnis loeseReferenzAuf(const Streckennetz& netz,
                                     const zusixml::ZusiPfad& fahrstrasseModul,
                                     std::string_view dateinameRef,
                                     int64_t refIndex,
                                     const char* refKontext) {
    const auto modulPfad = dateinameRef.empty()
        ? fahrstrasseModul
        : zusixml::ZusiPfad::vonZusiPfad(dateinameRef, fahrstrasseModul);
    const Strecke* strecke = netz.get(modulPfad);
    if (!strecke) {
        std::ostringstream os;
        os << refKontext << ": Modul \"" << modulPfad.alsZusiPfad() << "\" nicht geladen";
        return { {}, modulPfad, os.str() };
    }

    if ((refIndex < 0) || (static_cast<size_t>(refIndex) >= strecke->children_ReferenzElemente.size())) {
        std::ostringstream os;
        os << refKontext << ": Referenz-Index " << refIndex << " in Modul \""
           << modulPfad.alsZusiPfad() << "\" außerhalb des gültigen Bereichs (0-"
           << strecke->children_ReferenzElemente.size() << ")";
        return { {}, modulPfad, os.str() };
    }

    const auto& refEl = strecke->children_ReferenzElemente[refIndex];
    if (!refEl) {
        std::ostringstream os;
        os << refKontext << ": Referenzelement " << refIndex << " in Modul \""
           << modulPfad.alsZusiPfad() << "\" nicht vorhanden";
        return { {}, modulPfad, os.str() };
    }

    const auto strElIndex = refEl->StrElement;
    const auto richtung = static_cast<StreckenelementRichtung>(refEl->StrNorm);
    if ((strElIndex < 0) || (static_cast<size_t>(strElIndex) >= strecke->children_StrElement.size())) {
        std::ostringstream os;
        os << refKontext << ": Streckenelement-Nr " << strElIndex
           << " (" << richtungBeschriftung(richtung) << ") in Modul \""
           << modulPfad.alsZusiPfad() << "\" außerhalb des gültigen Bereichs";
        return { {}, modulPfad, os.str() };
    }

    const auto* strEl = strecke->children_StrElement[strElIndex].get();
    if (!strEl) {
        std::ostringstream os;
        os << refKontext << ": Streckenelement Nr " << strElIndex
           << " (" << richtungBeschriftung(richtung) << ") in Modul \""
           << modulPfad.alsZusiPfad() << "\" nicht vorhanden";
        return { {}, modulPfad, os.str() };
    }

    return { StreckenelementUndRichtung{ strEl, richtung }, modulPfad, std::string{} };
}

std::string signalLabel(const StreckenelementUndRichtung& er, const char* fallback) {
    if (!er.getStreckenelement()) {
        return fallback;
    }
    const auto& info = er.richtungsInfo();
    if (!info.has_value() || !info->Signal) {
        std::ostringstream os;
        os << fallback << " (Element Nr " << er->Nr << ", " << richtungBeschriftung(er.getRichtung()) << ")";
        return os.str();
    }
    const auto& sig = *info->Signal;
    std::ostringstream os;
    os << fallback;
    if (!sig.NameBetriebsstelle.empty() || !sig.Signalname.empty()) {
        os << ": ";
        if (!sig.NameBetriebsstelle.empty()) {
            os << sig.NameBetriebsstelle;
        }
        if (!sig.Signalname.empty()) {
            if (!sig.NameBetriebsstelle.empty()) {
                os << " ";
            }
            os << sig.Signalname;
        }
    }
    return os.str();
}

const Signal* signalAn(const StreckenelementUndRichtung& er) {
    if (!er.getStreckenelement()) {
        return nullptr;
    }
    const auto& info = er.richtungsInfo();
    if (!info.has_value() || !info->Signal) {
        return nullptr;
    }
    return info->Signal.get();
}

std::string elementBeschreibung(const StreckenelementUndRichtung& er) {
    if (!er.getStreckenelement()) {
        return {};
    }
    std::ostringstream os;
    os << "Element Nr. " << er->Nr << " (" << richtungBeschriftung(er.getRichtung()) << ")";
    return os.str();
}

/**
 * Wandelt eine HsigGeschw/VsigGeschw aus einem HsigBegriff/VsigBegriff in km/h
 * um. Konvention der Quelldaten: positive Werte sind in m/s gespeichert
 * (×3.6 in km/h); negative Werte werden übernommen.
 */
int geschwindigkeitInKmh(float wert) {
    const double kmh = wert < 0.0f ? wert : static_cast<double>(wert) * 3.6;
    return static_cast<int>(std::lround(kmh));
}

/**
 * Liefert die HsigGeschw der angegebenen Zeile aus der HsigBegriff-Liste eines
 * Signals, sofern Index und Eintrag gültig sind. Wird für die Anreicherung der
 * Anzeige um " (xx km/h)" verwendet.
 */
std::optional<float> hsigGeschwAnZeile(const Signal& sig, int zeile) {
    if (zeile < 0 || static_cast<size_t>(zeile) >= sig.children_HsigBegriff.size()) {
        return std::nullopt;
    }
    const auto& hb = sig.children_HsigBegriff[zeile];
    if (!hb) return std::nullopt;
    return hb->HsigGeschw;
}

/**
 * Liefert die VsigGeschw der angegebenen Spalte aus der VsigBegriff-Liste
 * eines Signals, sofern Index und Eintrag gültig sind. Analog zu
 * hsigGeschwAnZeile.
 */
std::optional<float> vsigGeschwAnSpalte(const Signal& sig, int spalte) {
    if (spalte < 0 || static_cast<size_t>(spalte) >= sig.children_VsigBegriff.size()) {
        return std::nullopt;
    }
    const auto& vb = sig.children_VsigBegriff[spalte];
    if (!vb) return std::nullopt;
    return vb->VsigGeschw;
}

/**
 * Liefert die ErsatzsigBezeichnung des Ersatzsignal-Eintrags der angegebenen
 * Zeile, oder std::nullopt, falls Index/Eintrag ungültig.
 */
std::optional<std::string> ersatzsignalBezeichnung(const Signal& sig, int zeile) {
    if (zeile < 0 || static_cast<size_t>(zeile) >= sig.children_Ersatzsignal.size()) {
        return std::nullopt;
    }
    const auto& es = sig.children_Ersatzsignal[zeile];
    if (!es) return std::nullopt;
    return es->ErsatzsigBezeichnung;
}

/**
 * Liefert die Register-Nummer der zur Element-Richtung gehörenden
 * RichtungsInfo (Attribut Reg), oder 0 falls nicht vorhanden bzw. Reg=0.
 */
int registerNrAn(const StreckenelementUndRichtung& er) {
    if (!er.getStreckenelement()) return 0;
    const auto& info = er.richtungsInfo();
    if (!info.has_value()) return 0;
    return info->Reg;
}

}  // namespace

std::vector<FahrstrasseDetailEintrag> ermittleFahrstrasseDetails(
    const Streckennetz& netz,
    const ResolvedFahrstrasse& fs)
{
    std::vector<FahrstrasseDetailEintrag> result;
    if (!fs.quelle) {
        return result;
    }

    const auto& fahrstrasseModul = fs.fahrstrasseModul;

    // Start
    {
        FahrstrasseDetailEintrag e;
        e.typ = FahrstrasseDetailEintrag::Typ::Start;
        e.elementUndRichtung = fs.start;
        e.label = signalLabel(fs.start, "Start");
        e.beschreibung = elementBeschreibung(fs.start);
        e.signal = signalAn(fs.start);
        e.signalModul = fahrstrasseModul;
        if (fs.quelle->FahrstrStart) {
            // Modulpfad des Start-Signals = das in FahrstrStart referenzierte Modul.
            e.signalModul = zusixml::ZusiPfad::vonZusiPfad(
                fs.quelle->FahrstrStart->Datei.Dateiname, fahrstrasseModul);
        }
        result.push_back(std::move(e));
    }

    // Ziel
    {
        FahrstrasseDetailEintrag e;
        e.typ = FahrstrasseDetailEintrag::Typ::Ziel;
        e.elementUndRichtung = fs.ziel;
        e.label = signalLabel(fs.ziel, "Ziel");
        e.beschreibung = elementBeschreibung(fs.ziel);
        e.signal = signalAn(fs.ziel);
        e.signalModul = fahrstrasseModul;
        if (fs.quelle->FahrstrZiel) {
            e.signalModul = zusixml::ZusiPfad::vonZusiPfad(
                fs.quelle->FahrstrZiel->Datei.Dateiname, fahrstrasseModul);
        }
        result.push_back(std::move(e));
    }

    // Hilfslambda für die Liste der Referenz-Kinder.
    const auto fuegeRefHinzu = [&](FahrstrasseDetailEintrag::Typ typ,
                                   const char* fallbackLabel,
                                   const char* refKontext,
                                   std::string_view dateiname,
                                   int64_t refIndex,
                                   bool istSignal) {
        FahrstrasseDetailEintrag e;
        e.typ = typ;
        auto res = loeseReferenzAuf(netz, fahrstrasseModul, dateiname, refIndex, refKontext);
        e.elementUndRichtung = res.elementUndRichtung;
        e.fehler = res.fehler;
        if (res.fehler.empty()) {
            if (istSignal) {
                e.label = signalLabel(res.elementUndRichtung, fallbackLabel);
                e.signal = signalAn(res.elementUndRichtung);
                e.signalModul = res.modul;
            } else {
                e.label = fallbackLabel;
                e.label += ": ";
                e.label += elementBeschreibung(res.elementUndRichtung);
            }
            e.beschreibung = elementBeschreibung(res.elementUndRichtung);
        } else {
            e.label = fallbackLabel;
            e.label += " (Auflösefehler)";
            e.beschreibung = res.fehler;
        }
        result.push_back(std::move(e));
    };

    // Verkettung der <KoppelSignal>-Einträge: ausgehend von dem zuletzt eingefügten
    // Signal/Vorsignal-Eintrag wird die Koppelsignal-Kette aufgelöst und jedes
    // gekoppelte Signal als neuer Eintrag (Typ Koppelsignal) hinter das jeweils
    // gekoppelte Signal gehängt. Matrix-Zeile/Spalte und Ersatzsignal-Flag werden
    // vom Wurzelsignal geerbt (Zusi stellt das gekoppelte Signal auf denselben
    // Index wie sein Bezugssignal). Eine Zykluserkennung über Signal-Pointer
    // verhindert Endlosschleifen bei rekursiven Kopplungen.
    const auto fuegeKoppelsignaleHinzu = [&](bool kopplungZuVsig) {
        if (result.empty()) {
            return;
        }
        const int wurzelIndex = static_cast<int>(result.size()) - 1;
        const int wurzelMatrixZeile = result[wurzelIndex].matrixZeile;
        const int wurzelMatrixSpalte = result[wurzelIndex].matrixSpalte;
        const bool wurzelErsatzsignal = result[wurzelIndex].ersatzsignal;

        std::unordered_set<const Signal*> besucht;
        if (result[wurzelIndex].signal) {
            besucht.insert(result[wurzelIndex].signal);
        }

        int parentIndex = wurzelIndex;
        int tiefe = 1;
        while (true) {
            const Signal* parentSignal = result[parentIndex].signal;
            if (!parentSignal || !parentSignal->KoppelSignal) {
                break;
            }
            const auto& koppel = *parentSignal->KoppelSignal;
            const auto parentSignalModul = result[parentIndex].signalModul;

            FahrstrasseDetailEintrag e;
            e.typ = FahrstrasseDetailEintrag::Typ::Koppelsignal;
            e.kopplungsTiefe = tiefe;
            e.kopplungZuVsig = kopplungZuVsig;
            e.matrixZeile = wurzelMatrixZeile;
            e.matrixSpalte = wurzelMatrixSpalte;
            e.ersatzsignal = wurzelErsatzsignal;

            auto res = loeseReferenzAuf(netz, parentSignalModul,
                                        koppel.Datei.Dateiname,
                                        static_cast<int64_t>(koppel.ReferenzNr),
                                        "KoppelSignal");
            e.elementUndRichtung = res.elementUndRichtung;
            e.fehler = res.fehler;
            if (res.fehler.empty()) {
                e.label = signalLabel(res.elementUndRichtung, "Koppelsignal");
                e.signal = signalAn(res.elementUndRichtung);
                e.signalModul = res.modul;
                e.beschreibung = elementBeschreibung(res.elementUndRichtung);
            } else {
                e.label = "Koppelsignal (Auflösefehler)";
                e.beschreibung = res.fehler;
            }

            // Zykluserkennung: dasselbe Signal darf in der Koppelkette nicht erneut
            // auftreten (sonst Endlosschleife).
            if (e.signal && !besucht.insert(e.signal).second) {
                std::ostringstream os;
                os << "Zyklische Kopplung erkannt (Signal Element Nr "
                   << e.elementUndRichtung->Nr << ", "
                   << richtungBeschriftung(e.elementUndRichtung.getRichtung())
                   << " erscheint mehrfach in der Koppelkette)";
                e.fehler = os.str();
                e.label = "Koppelsignal (Zyklus)";
                e.beschreibung = e.fehler;
                e.signal = nullptr;  // Kette hier abbrechen, nicht weiter folgen
                result.push_back(std::move(e));
                break;
            }

            const bool weiterMitKette = e.fehler.empty() && e.signal != nullptr;
            result.push_back(std::move(e));
            if (!weiterMitKette) {
                break;
            }
            parentIndex = static_cast<int>(result.size()) - 1;
            ++tiefe;
        }
    };

    // Reihenfolge gemäß Plan: Signale (Vor- und Hauptsignale), dann Weichen, Register,
    // Auflösungen, Teilauflösungen, Signalhaltfall.
    for (const auto& s : fs.quelle->children_FahrstrSignal) {
        if (!s) continue;
        fuegeRefHinzu(FahrstrasseDetailEintrag::Typ::FahrstrSignal,
                      "Signal", "FahrstrSignal", s->Datei.Dateiname, s->Ref, /*istSignal=*/true);
        if (!result.empty()) {
            result.back().matrixZeile = s->FahrstrSignalZeile;
            result.back().matrixSpalte = 0;
            result.back().ersatzsignal = s->FahrstrSignalErsatzsignal;
            std::ostringstream os;
            os << result.back().label;
            if (s->FahrstrSignalErsatzsignal) {
                // Hauptsignal mit Ersatzsignal: " -> Ersatzsignalzeile X (<Bezeichnung>)"
                os << " -> Ersatzsignalzeile " << s->FahrstrSignalZeile;
                if (result.back().signal) {
                    if (auto bez = ersatzsignalBezeichnung(*result.back().signal, s->FahrstrSignalZeile)) {
                        if (!bez->empty()) {
                            os << " (" << *bez << ")";
                        }
                    }
                }
            } else {
                // Hauptsignal ohne Ersatzsignal: " -> Zeile X (yy km/h)"
                os << " -> Zeile " << s->FahrstrSignalZeile;
                if (result.back().signal) {
                    if (auto v = hsigGeschwAnZeile(*result.back().signal, s->FahrstrSignalZeile)) {
                        os << " (" << geschwindigkeitInKmh(*v) << " km/h)";
                    }
                }
            }
            result.back().label = os.str();
        }
        fuegeKoppelsignaleHinzu(/*kopplungZuVsig=*/false);
    }
    for (const auto& s : fs.quelle->children_FahrstrVSignal) {
        if (!s) continue;
        fuegeRefHinzu(FahrstrasseDetailEintrag::Typ::FahrstrVSignal,
                      "Vorsignal", "FahrstrVSignal", s->Datei.Dateiname, s->Ref, /*istSignal=*/true);
        if (!result.empty()) {
            result.back().matrixZeile = 0;
            result.back().matrixSpalte = s->FahrstrSignalSpalte;
            result.back().ersatzsignal = false;
            std::ostringstream os;
            os << result.back().label << " -> Spalte " << s->FahrstrSignalSpalte;
            // Vorsignal: " -> Spalte X (yy km/h)"
            if (result.back().signal) {
                if (auto v = vsigGeschwAnSpalte(*result.back().signal, s->FahrstrSignalSpalte)) {
                    os << " (" << geschwindigkeitInKmh(*v) << " km/h)";
                }
            }
            result.back().label = os.str();
        }
        fuegeKoppelsignaleHinzu(/*kopplungZuVsig=*/true);
    }
    for (const auto& w : fs.quelle->children_FahrstrWeiche) {
        if (!w) continue;
        fuegeRefHinzu(FahrstrasseDetailEintrag::Typ::FahrstrWeiche,
                      "Weiche", "FahrstrWeiche", w->Datei.Dateiname, w->Ref, /*istSignal=*/false);
        // Weichenlage anhängen
        if (!result.empty()) {
            std::ostringstream os;
            os << result.back().label << " -> Nachfolger " << w->FahrstrWeichenlage;
            result.back().label = os.str();
        }
    }
    for (const auto& r : fs.quelle->children_FahrstrRegister) {
        if (!r) continue;
        fuegeRefHinzu(FahrstrasseDetailEintrag::Typ::FahrstrRegister,
                      "Register", "FahrstrRegister", r->Datei.Dateiname, r->Ref, /*istSignal=*/false);
        if (!result.empty() && result.back().fehler.empty()) {
            // Register-Nummer aus der RichtungsInfo des aufgelösten Elements anhängen.
            const int regNr = registerNrAn(result.back().elementUndRichtung);
            if (regNr != 0) {
                std::ostringstream os;
                os << result.back().label << " -> Register " << regNr;
                result.back().label = os.str();
            }
        }
    }
    for (const auto& a : fs.quelle->children_FahrstrAufloesung) {
        if (!a) continue;
        fuegeRefHinzu(FahrstrasseDetailEintrag::Typ::FahrstrAufloesung,
                      "Auflösepunkt", "FahrstrAufloesung", a->Datei.Dateiname, a->Ref, /*istSignal=*/false);
    }
    for (const auto& t : fs.quelle->children_FahrstrTeilaufloesung) {
        if (!t) continue;
        fuegeRefHinzu(FahrstrasseDetailEintrag::Typ::FahrstrTeilaufloesung,
                      "Teilauflösung", "FahrstrTeilaufloesung", t->Datei.Dateiname, t->Ref, /*istSignal=*/false);
    }
    for (const auto& h : fs.quelle->children_FahrstrSigHaltfall) {
        if (!h) continue;
        fuegeRefHinzu(FahrstrasseDetailEintrag::Typ::FahrstrSigHaltfall,
                      "Signalhaltfall", "FahrstrSigHaltfall", h->Datei.Dateiname, h->Ref, /*istSignal=*/false);
    }

    return result;
}
