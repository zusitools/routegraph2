#include "model/fahrstrassendetails.h"

#include "streckennetz.h"

#include "zusi_parser/zusi_types.hpp"

#include <sstream>
#include <string_view>
#include <utility>

namespace {

/** Ergebnis einer Referenz-Auflösung. */
struct AufloesungsErgebnis {
    StreckenelementUndRichtung elementUndRichtung;
    zusixml::ZusiPfad modul = zusixml::ZusiPfad::vonZusiPfad("");
    std::string fehler;  // leer = OK
};

/**
 * Identisch zur Hilfsfunktion in fahrstrasse.cpp; zusätzlich liefert sie den
 * Modulpfad, weil wir ihn fürs Anhängen relativer LS3-Pfade brauchen.
 */
AufloesungsErgebnis loeseReferenzAuf(const Streckennetz& netz,
                                     const zusixml::ZusiPfad& fahrstrasseModul,
                                     std::string_view dateinameRef,
                                     int64_t refIndex,
                                     const char* refKontext) {
    const auto modulPfad = zusixml::ZusiPfad::vonZusiPfad(dateinameRef, fahrstrasseModul);
    const Strecke* strecke = netz.get(modulPfad);
    if (!strecke) {
        std::ostringstream os;
        os << refKontext << ": Modul \"" << dateinameRef << "\" nicht geladen";
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
    const char* richtungStr = richtung == StreckenelementRichtung::Norm ? "Norm" : "Gegen";
    if ((strElIndex < 0) || (static_cast<size_t>(strElIndex) >= strecke->children_StrElement.size())) {
        std::ostringstream os;
        os << refKontext << ": Streckenelement-Nr " << strElIndex
           << " (" << richtungStr << ") in Modul \""
           << modulPfad.alsZusiPfad() << "\" außerhalb des gültigen Bereichs";
        return { {}, modulPfad, os.str() };
    }

    const auto* strEl = strecke->children_StrElement[strElIndex].get();
    if (!strEl) {
        std::ostringstream os;
        os << refKontext << ": Streckenelement Nr " << strElIndex
           << " (" << richtungStr << ") in Modul \""
           << modulPfad.alsZusiPfad() << "\" nicht vorhanden";
        return { {}, modulPfad, os.str() };
    }

    return { StreckenelementUndRichtung{ strEl, richtung }, modulPfad, std::string{} };
}

const char* richtungBeschriftung(StreckenelementRichtung richtung) {
    return richtung == StreckenelementRichtung::Norm ? "Norm" : "Gegen";
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
    os << "Element Nr " << er->Nr << " (" << richtungBeschriftung(er.getRichtung()) << ")";
    return os.str();
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
        }
    }
    for (const auto& s : fs.quelle->children_FahrstrVSignal) {
        if (!s) continue;
        fuegeRefHinzu(FahrstrasseDetailEintrag::Typ::FahrstrVSignal,
                      "Vorsignal", "FahrstrVSignal", s->Datei.Dateiname, s->Ref, /*istSignal=*/true);
        if (!result.empty()) {
            result.back().matrixZeile = 0;
            result.back().matrixSpalte = s->FahrstrSignalSpalte;
            result.back().ersatzsignal = false;
        }
    }
    for (const auto& w : fs.quelle->children_FahrstrWeiche) {
        if (!w) continue;
        fuegeRefHinzu(FahrstrasseDetailEintrag::Typ::FahrstrWeiche,
                      "Weiche", "FahrstrWeiche", w->Datei.Dateiname, w->Ref, /*istSignal=*/false);
        // Weichenlage anhängen
        if (!result.empty()) {
            std::ostringstream os;
            os << result.back().label << " -> Lage " << w->FahrstrWeichenlage;
            result.back().label = os.str();
        }
    }
    for (const auto& r : fs.quelle->children_FahrstrRegister) {
        if (!r) continue;
        fuegeRefHinzu(FahrstrasseDetailEintrag::Typ::FahrstrRegister,
                      "Register", "FahrstrRegister", r->Datei.Dateiname, r->Ref, /*istSignal=*/false);
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
