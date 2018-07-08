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
    // Union aus "const StrElement*" und "StreckenelementRichtung"
    // à la llvm::PointerIntPair
    intptr_t val;

    enum {
        BoolMask = (uintptr_t) 0x1,
        PointerMask = ~(uintptr_t) 0x1,
    };

    StreckenelementUndRichtung() : val(reinterpret_cast<intptr_t>(nullptr)) {
        assert((reinterpret_cast<intptr_t>(nullptr) & 0x1) == 0);
    }

    StreckenelementUndRichtung(const StrElement* strElement, StreckenelementRichtung richtung)
        : val(reinterpret_cast<intptr_t>(strElement) | static_cast<intptr_t>(richtung)) {
        assert((reinterpret_cast<intptr_t>(strElement) & 0x1) == 0);
    }

private:
    explicit StreckenelementUndRichtung(intptr_t val) : val(val) {}

public:
    const StrElement* getStreckenelement() const {
       return reinterpret_cast<const StrElement*>(val & PointerMask);
    }

    StreckenelementRichtung getRichtung() const {
        return static_cast<StreckenelementRichtung>((val & BoolMask) != 0);
    }

    inline const std::vector<StrElement*>& nachfolgerElemente() const {
        return this->getRichtung() == StreckenelementRichtung::Norm ?
            this->getStreckenelement()->nachfolgerElementeNorm :
            this->getStreckenelement()->nachfolgerElementeGegen;
    }

    inline const std::vector<StrElement*>& vorgaengerElemente() const {
        return this->getRichtung() == StreckenelementRichtung::Norm ?
            this->getStreckenelement()->nachfolgerElementeGegen :
            this->getStreckenelement()->nachfolgerElementeNorm;
    }

    inline StreckenelementUndRichtung nachfolger(const size_t index = 0) const {
        const auto anschluss_shift = index + (this->getRichtung() == StreckenelementRichtung::Gegen ? 8 : 0);
        return StreckenelementUndRichtung(
            this->nachfolgerElemente().at(index),
            ((this->getStreckenelement()->Anschluss >> anschluss_shift) & 1) == 0 ?
                StreckenelementRichtung::Norm : StreckenelementRichtung::Gegen);
    }

    inline bool hatNachfolger(const size_t index = 0) const {
        const auto& nachfolger = this->nachfolgerElemente();
        return (index < nachfolger.size()) && (nachfolger[index] != nullptr);
    }

    inline size_t anzahlNachfolger() const {
        return this->nachfolgerElemente().size();
    }

    inline StreckenelementUndRichtung vorgaenger(const size_t index = 0) const {
        const auto anschluss_shift = index + (this->getRichtung() == StreckenelementRichtung::Norm ? 8 : 0);

        return StreckenelementUndRichtung(
            this->vorgaengerElemente().at(index),
            ((this->getStreckenelement()->Anschluss >> anschluss_shift) & 1) == 0 ?
                StreckenelementRichtung::Gegen : StreckenelementRichtung::Norm);
    }

    inline bool hatVorgaenger(const size_t index = 0) const {
        const auto& vorgaenger = this->vorgaengerElemente();
        return (index < vorgaenger.size()) && (vorgaenger[index] != nullptr);
    }

    inline size_t anzahlVorgaenger() const {
        return this->vorgaengerElemente().size();
    }

    inline StreckenelementUndRichtung gegenrichtung() const {
        return StreckenelementUndRichtung(this->val ^ BoolMask);
    }

    inline const std::optional<StreckenelementRichtungsInfo>& richtungsInfo() const {
        return this->getRichtung() == StreckenelementRichtung::Norm ? this->getStreckenelement()->InfoNormRichtung : this->getStreckenelement()->InfoGegenRichtung;
    }

    inline const Vec3& endpunkt() const {
        return this->getRichtung() == StreckenelementRichtung::Norm ? this->getStreckenelement()->b : this->getStreckenelement()->g;
    }

    inline bool operator==(const StreckenelementUndRichtung other) const {
      return this->val == other.val;
    }
    inline bool operator!=(const StreckenelementUndRichtung other) const {
      return !(*this == other);
    }

    inline operator bool() const {
        return this->getStreckenelement() != nullptr;
    }

    inline const StrElement* operator->() const {
        return this->getStreckenelement();
    }

    inline const StrElement& operator*() const {
        return *(this->getStreckenelement());
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
