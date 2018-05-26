#ifndef STRECKENELEMENT_H
#define STRECKENELEMENT_H

#include "zusi_parser/zusi_types.hpp"

enum class StreckenelementRichtung : bool {
    Norm = true,
    Gegen = false
};

inline StreckenelementRichtung umkehren(StreckenelementRichtung richtung) {
    return static_cast<StreckenelementRichtung>(!static_cast<bool>(richtung));
}

enum class SignalTyp : decltype(Signal::SignalTyp) {
    Unbestimmt = 0,
    Tafel = 1,
    Weiche = 2,
    Gleissperre = 3,
    Bahnuebergang = 4,
    Rangiersignal = 5,
    Vorsignal = 6,
    Einfahrsignal = 7,
    Zwischensignal = 8,
    Ausfahrsignal = 9,
    Blocksignal = 10,
    Deckungssignal = 11,
    LZB_Block = 12,
    Hilfshauptsignal = 13,
    Sonstiges = 14,
};

enum class ReferenzpunktTyp : decltype(ReferenzElement::RefTyp) {
    Aufgleispunkt = 0,
    Modulgrenze = 1,
    Register = 2,
    Weiche = 3,
    Signal = 4,
    Aufloesepunkt = 5,
    Signalhaltfall = 6
};

// Funktions-Flags eines Streckenelements.
enum class StreckenelementFlag : uint32_t {
    Tunnel = 1 << 0,
    KeineGleisfunktion = 1 << 1,
    Weichenbausatz = 1 << 2,
    KeineSchulterRechts = 1 << 3,
    KeineSchulterLinks = 1 << 4,
};

// Ein Verweis auf eine Richtung eines Streckenelements.
// Auf die Properties und Methoden des Streckenelements kann mit dem Operator -> zugegriffen werden.
struct StreckenelementUndRichtung {
    // Das Streckenelement, auf das sich die Referenz bezieht.
    const StrElement* streckenelement;

    // Die Richtung des Streckenelements, auf das sich die Referenz bezieht.
    StreckenelementRichtung richtung;

    inline const std::vector<StrElement*>& nachfolgerElemente() const {
        return this->richtung == StreckenelementRichtung::Norm ?
            this->streckenelement->nachfolgerElementeNorm :
            this->streckenelement->nachfolgerElementeGegen;
    }

    inline const std::vector<StrElement*>& vorgaengerElemente() const {
        return this->richtung == StreckenelementRichtung::Norm ?
            this->streckenelement->nachfolgerElementeGegen :
            this->streckenelement->nachfolgerElementeNorm;
    }

    inline StreckenelementUndRichtung nachfolger(const size_t index = 0) const {
        const auto anschluss_shift = index + (this->richtung == StreckenelementRichtung::Gegen ? 8 : 0);
        return StreckenelementUndRichtung {
            this->nachfolgerElemente().at(index),
            ((this->streckenelement->Anschluss >> anschluss_shift) & 1) == 0 ?
                StreckenelementRichtung::Norm : StreckenelementRichtung::Gegen };
    }

    inline bool hatNachfolger(const size_t index = 0) const {
        const auto& nachfolger = this->nachfolgerElemente();
        return (index < nachfolger.size()) && (nachfolger[index] != nullptr);
    }

    inline size_t anzahlNachfolger() const {
        return this->nachfolgerElemente().size();
    }

    inline StreckenelementUndRichtung vorgaenger(const size_t index = 0) const {
        const auto anschluss_shift = index + (this->richtung == StreckenelementRichtung::Norm ? 8 : 0);

        return StreckenelementUndRichtung {
            this->vorgaengerElemente().at(index),
            ((this->streckenelement->Anschluss >> anschluss_shift) & 1) == 0 ?
                StreckenelementRichtung::Gegen : StreckenelementRichtung::Norm };
    }

    inline bool hatVorgaenger(const size_t index = 0) const {
        const auto& vorgaenger = this->vorgaengerElemente();
        return (index < vorgaenger.size()) && (vorgaenger[index] != nullptr);
    }

    inline size_t anzahlVorgaenger() const {
        return this->vorgaengerElemente().size();
    }

    inline StreckenelementUndRichtung gegenrichtung() const {
        return StreckenelementUndRichtung { this->streckenelement, umkehren(this->richtung) };
    }

    inline const std::optional<StreckenelementRichtungsInfo>& richtungsInfo() const {
        return this->richtung == StreckenelementRichtung::Norm ? this->streckenelement->InfoNormRichtung : this->streckenelement->InfoGegenRichtung;
    }

    inline const Vec3& endpunkt() const {
        return this->richtung == StreckenelementRichtung::Norm ? this->streckenelement->b : this->streckenelement->g;
    }

    inline bool operator==(const StreckenelementUndRichtung &other) const {
      return this->streckenelement == other.streckenelement && this->richtung == other.richtung;
    }
    inline bool operator!=(const StreckenelementUndRichtung &other) const {
      return !(*this == other);
    }

    inline operator bool() const {
        return this->streckenelement != nullptr;
    }

    inline const StrElement* operator->() const {
        return this->streckenelement;
    }

    inline const StrElement& operator*() const {
        return *(this->streckenelement);
    }
};

// TODO
inline Vec3 operator-(const Vec3& left, const Vec3& right)
{
    return { left.X - right.X, left.Y - right.Y, left.Z - right.Z };
}

// TODO
inline bool hatFktFlag(const StrElement& element, StreckenelementFlag flag) {
    return (element.Fkt & static_cast<uint32_t>(flag)) != 0;
}

// TODO
inline StreckenelementUndRichtung richtung(const StrElement& element, StreckenelementRichtung richtung) {
    return StreckenelementUndRichtung { &element, richtung };
}

#endif // STRECKENELEMENT_H
