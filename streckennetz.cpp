#include "streckennetz.h"

#include "zusi_parser/utils.hpp"

#include <cassert>

void Streckennetz::add(const zusixml::ZusiPfad& pfad, std::unique_ptr<Strecke> strecke) {
    decltype(this->m_elementeMitUnaufgeloestemNachfolger) elementeMitUnaufgeloestemNachfolgerNeu;

    // Neues Modul verknuepfen mit anderen geladenen Modulen
    for (auto& streckenelement : strecke->children_StrElement) {
        if (!streckenelement) {
            continue;
        }
        const auto& verknuepfeMitNachbarmodul = [this, &streckenelement, &pfad, &elementeMitUnaufgeloestemNachfolgerNeu](
                const decltype(streckenelement->children_NachNormModul)& nachfolgerAnderesModul,
                decltype(streckenelement->nachfolgerElementeNorm)& nachfolgerVerknuepft,
                bool istNorm) {
            for (const auto& nachfolger : nachfolgerAnderesModul) {
                const auto* nachfolgerStrecke = this->get(zusixml::ZusiPfad::vonZusiPfad(nachfolger.Datei.Dateiname, pfad));
                if (!nachfolgerStrecke) {
                    nachfolgerVerknuepft.push_back(nullptr);
                    elementeMitUnaufgeloestemNachfolgerNeu.emplace_back(pfad, streckenelement.get());
                    continue;
                }
                const auto referenzNr = nachfolger.Nr;
                if ((referenzNr < 0) || (static_cast<size_t>(referenzNr) >= nachfolgerStrecke->children_ReferenzElemente.size())) {
                    nachfolgerVerknuepft.push_back(nullptr);
                    continue;
                }
                const auto& referenzElement = nachfolgerStrecke->children_ReferenzElemente[referenzNr];
                if (!referenzElement) {
                    nachfolgerVerknuepft.push_back(nullptr);
                    continue;
                }
                const auto elementNr = referenzElement->StrElement;
                if ((elementNr < 0) || (static_cast<size_t>(elementNr) >= nachfolgerStrecke->children_StrElement.size())) {
                    nachfolgerVerknuepft.push_back(nullptr);
                    continue;
                }
                nachfolgerVerknuepft.push_back(nachfolgerStrecke->children_StrElement[elementNr].get());
                (istNorm ? streckenelement->nachfolgerElementeNormSindInAnderemModul : streckenelement->nachfolgerElementeGegenSindInAnderemModul) = true;
                const auto anschluss_shift = nachfolgerVerknuepft.size() + (istNorm ? 0 : 8);
                if (referenzElement->StrNorm) {
                    // Element zeigt mit Normrichtung zur Modulgrenze
                    // -> Nachfolgerelement ist in Gegenrichtung
                    // -> Setze Bit auf 1
                    streckenelement->Anschluss |= (1 << anschluss_shift);
                } else {
                    // Nachfolgerelement ist in Normrichtung
                    // -> Setze Bit auf 0
                    streckenelement->Anschluss &= ~(1 << anschluss_shift);
                }
            }
        };
        if (streckenelement->children_NachNorm.empty()) {
            assert(streckenelement->nachfolgerElementeNorm.empty());
            verknuepfeMitNachbarmodul(streckenelement->children_NachNormModul, streckenelement->nachfolgerElementeNorm, true);
        }
        if (streckenelement->children_NachGegen.empty()) {
            assert(streckenelement->nachfolgerElementeGegen.empty());
            verknuepfeMitNachbarmodul(streckenelement->children_NachGegenModul, streckenelement->nachfolgerElementeGegen, false);
        }
    }

    // Andere vorher geladene Module mit diesem Modul verknuepfen
    m_elementeMitUnaufgeloestemNachfolger.erase(std::remove_if(m_elementeMitUnaufgeloestemNachfolger.begin(), m_elementeMitUnaufgeloestemNachfolger.end(), [&](auto& kv) -> bool {
        const auto& pfad = kv.first;
        auto& streckenelement = kv.second;
        // Gibt true zurueck, wenn alle Verknuepfungen aufgeloest werden konnten
        const auto& verknuepfeMitNachbarmodul = [this, &streckenelement, &pfad](
                const decltype(streckenelement->children_NachNormModul)& nachfolgerAnderesModul,
                decltype(streckenelement->nachfolgerElementeNorm)& nachfolgerVerknuepft,
                bool istNorm) -> bool {
            assert(nachfolgerAnderesModul.size() == nachfolgerVerknuepft.size());
            bool result = true;
            for (size_t i = 0, len = nachfolgerAnderesModul.size(); i < len; ++i) {
                if (nachfolgerVerknuepft[i] != nullptr) {
                    continue;
                }
                const auto* nachfolgerStrecke = this->get(zusixml::ZusiPfad::vonZusiPfad(nachfolgerAnderesModul[i].Datei.Dateiname, pfad));
                if (!nachfolgerStrecke) {
                    result = false;
                    continue;
                }
                const auto referenzNr = nachfolgerAnderesModul[i].Nr;
                if ((referenzNr < 0) || (static_cast<size_t>(referenzNr) >= nachfolgerStrecke->children_ReferenzElemente.size())) {
                    continue;
                }
                const auto& referenzElement = nachfolgerStrecke->children_ReferenzElemente[referenzNr];
                if (!referenzElement) {
                    continue;
                }
                const auto elementNr = referenzElement->StrElement;
                if ((elementNr < 0) || (static_cast<size_t>(elementNr) >= nachfolgerStrecke->children_StrElement.size())) {
                    continue;
                }
                nachfolgerVerknuepft[i] = nachfolgerStrecke->children_StrElement[elementNr].get();
                (istNorm ? streckenelement->nachfolgerElementeNormSindInAnderemModul : streckenelement->nachfolgerElementeGegenSindInAnderemModul) = true;
                const auto anschluss_shift = nachfolgerVerknuepft.size() + (istNorm ? 0 : 8);
                if (referenzElement->StrNorm) {
                    // Element zeigt mit Normrichtung zur Modulgrenze
                    // -> Nachfolgerelement ist in Gegenrichtung
                    // -> Setze Bit auf 1
                    streckenelement->Anschluss |= (1 << anschluss_shift);
                } else {
                    // Nachfolgerelement ist in Normrichtung
                    // -> Setze Bit auf 0
                    streckenelement->Anschluss &= ~(1 << anschluss_shift);
                }
            }
            return result;
        };
        bool success = true;
        if (streckenelement->children_NachNorm.empty()) {
            success = verknuepfeMitNachbarmodul(streckenelement->children_NachNormModul, streckenelement->nachfolgerElementeNorm, true) && success;
        }
        if (streckenelement->children_NachGegen.empty()) {
            success = verknuepfeMitNachbarmodul(streckenelement->children_NachGegenModul, streckenelement->nachfolgerElementeGegen, false) && success;
        }
        return success;
    }), m_elementeMitUnaufgeloestemNachfolger.end());

    if (m_elementeMitUnaufgeloestemNachfolger.empty()) {
        m_elementeMitUnaufgeloestemNachfolger = std::move(elementeMitUnaufgeloestemNachfolgerNeu);
    } else {
        m_elementeMitUnaufgeloestemNachfolger.insert(
                    std::end(m_elementeMitUnaufgeloestemNachfolger),
                    std::make_move_iterator(elementeMitUnaufgeloestemNachfolgerNeu.begin()),
                    std::make_move_iterator(elementeMitUnaufgeloestemNachfolgerNeu.end()));
    }

    std::string key { pfad.alsZusiPfad() };
    // TODO: to upper case
    m_strecken.emplace(std::move(key), std::move(strecke));
}

Strecke* Streckennetz::get(const zusixml::ZusiPfad& pfad) const {
    std::string key { pfad.alsZusiPfad() };
    // TODO: to upper case
    if (const auto& it = m_strecken.find(key); it != m_strecken.end()) {
        return it->second.get();
    }
    return nullptr;
}

void Streckennetz::clear() {
    m_strecken.clear();
}

bool Streckennetz::empty() {
    return m_strecken.empty();
}
