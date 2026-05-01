#include "model/fahrstrasse.h"

#include "streckennetz.h"

#include "zusi_parser/utils.hpp"
#include "zusi_parser/zusi_types.hpp"

#include <algorithm>
#include <map>
#include <sstream>
#include <string_view>
#include <utility>

namespace {

/** Ergebnis einer Referenz-Auflösung (FahrstrStart/Ziel/Weiche -> StreckenelementUndRichtung). */
struct AufloesungsErgebnis {
    StreckenelementUndRichtung elementUndRichtung;
    std::string fehler;  // leer = OK
};

/**
 * Löst eine Fahrstraßen-Referenz (Modul-Datei + Ref-Index in children_ReferenzElemente)
 * auf das Streckenelement und die Richtung, die das ReferenzElement bezeichnet.
 * Gibt im Fehlerfall einen für den Nutzer lesbaren Text in `fehler` zurück.
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
        return { {}, os.str() };
    }

    if ((refIndex < 0) || (static_cast<size_t>(refIndex) >= strecke->children_ReferenzElemente.size())) {
        std::ostringstream os;
        os << refKontext << ": Referenz-Index " << refIndex << " in Modul \""
           << modulPfad.alsZusiPfad() << "\" außerhalb des gültigen Bereichs (0-"
           << strecke->children_ReferenzElemente.size() << ")";
        return { {}, os.str() };
    }

    const auto& refEl = strecke->children_ReferenzElemente[refIndex];
    if (!refEl) {
        std::ostringstream os;
        os << refKontext << ": Referenzelement " << refIndex << " in Modul \""
           << modulPfad.alsZusiPfad() << "\" nicht vorhanden";
        return { {}, os.str() };
    }

    const auto strElIndex = refEl->StrElement;
    const auto richtung = static_cast<StreckenelementRichtung>(refEl->StrNorm);
    const char* richtungStr = richtung == StreckenelementRichtung::Norm ? "Norm" : "Gegen";
    if ((strElIndex < 0) || (static_cast<size_t>(strElIndex) >= strecke->children_StrElement.size())) {
        std::ostringstream os;
        os << refKontext << ": Streckenelement-Nr " << strElIndex
           << " (" << richtungStr << ") in Modul \""
           << modulPfad.alsZusiPfad() << "\" außerhalb des gültigen Bereichs";
        return { {}, os.str() };
    }

    const auto* strEl = strecke->children_StrElement[strElIndex].get();
    if (!strEl) {
        std::ostringstream os;
        os << refKontext << ": Streckenelement Nr " << strElIndex
           << " (" << richtungStr << ") in Modul \""
           << modulPfad.alsZusiPfad() << "\" nicht vorhanden";
        return { {}, os.str() };
    }

    return { StreckenelementUndRichtung{ strEl, richtung }, std::string{} };
}

/** Liefert eine textuelle Beschreibung der Element-Richtung für Fehlermeldungen. */
const char* richtungBeschriftung(StreckenelementRichtung richtung) {
    return richtung == StreckenelementRichtung::Norm ? "Norm" : "Gegen";
}

/**
 * Berechnet den Pfad zwischen Start- und Zielelement der Fahrstraße. Folgt an
 * Verzweigungselementen ausschließlich der durch FahrstrWeiche-Einträge
 * konfigurierten Weichenlage. Fehlt diese, wird mit einem Fehler abgebrochen.
 *
 * Das Startelement (an dessen Ende das Startsignal steht) ist NICHT Teil des
 * Pfades; das Zielelement (an dessen Ende das Zielsignal steht) hingegen schon.
 */
std::string ermittlePfadIntern(const StreckenelementUndRichtung start,
                               const StreckenelementUndRichtung ziel,
                               const std::map<std::pair<const StrElement*, bool>, int>& weichenlagen,
                               size_t maxSchritte,
                               std::vector<StreckenelementUndRichtung>& pfadOut) {
    if (!start.getStreckenelement() || !ziel.getStreckenelement()) {
        return "Start- oder Zielelement nicht aufgelöst";
    }

    pfadOut.clear();

    // Start- und Zielelement sind identisch -> Fahrstraße hat keine Streckenelemente
    // im Pfad (Start und Ziel sind beide am Ende desselben Elements).
    if (start.getStreckenelement() == ziel.getStreckenelement()) {
        return std::string{};
    }

    auto cur = start;

    for (size_t i = 0; i < maxSchritte; ++i) {
        const auto anzN = cur.anzahlNachfolger();
        if (anzN == 0) {
            std::ostringstream os;
            os << "Kein Nachfolger an Streckenelement Nr " << cur->Nr
               << " (" << richtungBeschriftung(cur.getRichtung()) << ")"
               << " vor Erreichen des Zielelements";
            return os.str();
        }

        size_t idx = 0;
        if (anzN > 1) {
            const auto it = weichenlagen.find({ cur.getStreckenelement(), cur.getRichtung() == StreckenelementRichtung::Norm });
            if (it == weichenlagen.end()) {
                std::ostringstream os;
                os << "Weichenlage für Streckenelement Nr " << cur->Nr
                   << " (" << richtungBeschriftung(cur.getRichtung()) << ") fehlt";
                return os.str();
            }
            idx = static_cast<size_t>(it->second) - 1;
            if (idx >= anzN) {
                std::ostringstream os;
                os << "Weichenlage " << it->second << " an Streckenelement Nr "
                   << cur->Nr << " (" << richtungBeschriftung(cur.getRichtung()) << ")"
                   << " außerhalb des gültigen Bereichs (1-" << anzN << ")";
                return os.str();
            }
        }

        if (!cur.hatNachfolger(idx)) {
            std::ostringstream os;
            os << "Nachfolger " << (idx + 1) << " an Streckenelement Nr " << cur->Nr
               << " (" << richtungBeschriftung(cur.getRichtung()) << ")"
               << " nicht aufgelöst (vermutlich Modulgrenze)";
            return os.str();
        }

        cur = cur.nachfolger(idx);
        pfadOut.push_back(cur);

        if (cur.getStreckenelement() == ziel.getStreckenelement()) {
            return std::string{};
        }
    }

    std::ostringstream os;
    os << "Maximale Schrittzahl " << maxSchritte << " überschritten – Endlosschleife?";
    return os.str();
}

/** Liefert die NameBetriebsstelle des Signals an einer Element-Richtung, oder leer. */
std::string betriebsstelleVon(const StreckenelementUndRichtung& er) {
    if (!er.getStreckenelement()) {
        return {};
    }
    const auto& info = er.richtungsInfo();
    if (!info.has_value() || !info->Signal) {
        return {};
    }
    return info->Signal->NameBetriebsstelle;
}

}  // namespace

std::vector<ResolvedFahrstrasse> ermittleFahrstrassen(const Streckennetz& netz) {
    std::vector<ResolvedFahrstrasse> result;

    // Gesamtanzahl Streckenelemente -> Schrittzahl-Grenze
    size_t gesamtElemente = 0;
    for (auto it = netz.cbegin(); it != netz.cend(); ++it) {
        if (it->second) {
            gesamtElemente += it->second->children_StrElement.size();
        }
    }
    const size_t maxSchritte = std::max<size_t>(gesamtElemente + 1, 16);

    for (auto it = netz.cbegin(); it != netz.cend(); ++it) {
        const auto fahrstrasseModulPfad = zusixml::ZusiPfad::vonZusiPfad(it->first);
        const auto& strecke = it->second;
        if (!strecke) {
            continue;
        }

        for (const auto& fs : strecke->children_Fahrstrasse) {
            if (!fs) {
                continue;
            }

            ResolvedFahrstrasse rf;
            rf.name = fs->FahrstrName;
            rf.typ = fs->FahrstrTyp;

            std::string fehler;

            // Start auflösen
            if (!fs->FahrstrStart) {
                fehler = "FahrstrStart fehlt";
            } else {
                auto res = loeseReferenzAuf(netz, fahrstrasseModulPfad,
                                            fs->FahrstrStart->Datei.Dateiname,
                                            fs->FahrstrStart->Ref, "FahrstrStart");
                if (!res.fehler.empty()) {
                    fehler = std::move(res.fehler);
                } else {
                    rf.start = res.elementUndRichtung;
                }
            }

            // Ziel auflösen
            if (fehler.empty()) {
                if (!fs->FahrstrZiel) {
                    fehler = "FahrstrZiel fehlt";
                } else {
                    auto res = loeseReferenzAuf(netz, fahrstrasseModulPfad,
                                                fs->FahrstrZiel->Datei.Dateiname,
                                                fs->FahrstrZiel->Ref, "FahrstrZiel");
                    if (!res.fehler.empty()) {
                        fehler = std::move(res.fehler);
                    } else {
                        rf.ziel = res.elementUndRichtung;
                    }
                }
            }

            // Weichenlagen auflösen
            std::map<std::pair<const StrElement*, bool>, int> weichenlagen;
            if (fehler.empty()) {
                for (const auto& w : fs->children_FahrstrWeiche) {
                    if (!w) {
                        continue;
                    }
                    auto res = loeseReferenzAuf(netz, fahrstrasseModulPfad,
                                                w->Datei.Dateiname,
                                                w->Ref, "FahrstrWeiche");
                    if (!res.fehler.empty()) {
                        fehler = std::move(res.fehler);
                        break;
                    }
                    const auto* el = res.elementUndRichtung.getStreckenelement();
                    const bool norm = res.elementUndRichtung.getRichtung() == StreckenelementRichtung::Norm;
                    weichenlagen[{ el, norm }] = w->FahrstrWeichenlage;
                }
            }

            // Pfad berechnen
            if (fehler.empty()) {
                fehler = ermittlePfadIntern(rf.start, rf.ziel, weichenlagen, maxSchritte, rf.pfad);
                if (!fehler.empty()) {
                    rf.pfad.clear();
                }
            }

            // Betriebsstelle: bevorzugt vom Start-, sonst vom Zielsignal
            rf.betriebsstelle = betriebsstelleVon(rf.start);
            if (rf.betriebsstelle.empty()) {
                rf.betriebsstelle = betriebsstelleVon(rf.ziel);
            }

            rf.fehler = std::move(fehler);
            result.push_back(std::move(rf));
        }
    }

    std::sort(result.begin(), result.end(), [](const ResolvedFahrstrasse& a, const ResolvedFahrstrasse& b) {
        if (a.betriebsstelle != b.betriebsstelle) {
            return a.betriebsstelle < b.betriebsstelle;
        }
        return a.name < b.name;
    });

    return result;
}
