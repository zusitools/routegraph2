#ifndef STRECKENNETZ_H
#define STRECKENNETZ_H

#include "zusi_parser/utils.hpp"

#include <memory>
#include <unordered_map>
#include <vector>

class Strecke;
class StrElement;

class Streckennetz
{
public:
    void add(const zusixml::ZusiPfad& pfad, std::unique_ptr<Strecke> strecke);
    Strecke* get(const zusixml::ZusiPfad& pfad) const;
    [[nodiscard]] bool empty();

private:
    // Strecken dürfen nur hinzugefügt werden, nie entfernt.
    // Durch die modulübergreifende Verknüpfung verweisen die Strecken aufeinander.
    std::unordered_map<std::string, std::unique_ptr<Strecke>> m_strecken;

    // ZusiPfad ist der Pfad der Strecke, in der StrElement* liegt -- dient zum Auflösen relativer Pfade.
    std::vector<std::pair<zusixml::ZusiPfad, StrElement*>> m_elementeMitUnaufgeloestemNachfolger;

public:
    typename decltype(m_strecken)::const_iterator cbegin() const { return m_strecken.cbegin(); }
    typename decltype(m_strecken)::const_iterator cend() const { return m_strecken.cend(); }
};

#endif // STRECKENNETZ_H
