#include "streckennetz.h"

#include "zusi_parser/utils.hpp"

void Streckennetz::add(const zusixml::ZusiPfad& pfad, std::unique_ptr<Strecke> strecke) {
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
