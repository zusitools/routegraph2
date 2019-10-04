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
    void clear();
    [[nodiscard]] bool empty();

private:
    std::unordered_map<std::string, std::unique_ptr<Strecke>> m_strecken;
    std::vector<std::pair<zusixml::ZusiPfad, StrElement*>> m_elementeMitUnaufgeloestemNachfolger;

public:
    typename decltype(m_strecken)::const_iterator cbegin() const { return m_strecken.cbegin(); }
    typename decltype(m_strecken)::const_iterator cend() const { return m_strecken.cend(); }
};

#endif // STRECKENNETZ_H
