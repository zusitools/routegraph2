#include "segmentierer.h"

bool Segmentierer::istSegmentGrenze(const StreckenelementUndRichtung, const StreckenelementUndRichtung) const
{
    return false;
}

bool Segmentierer::istSegmentStart(const StreckenelementUndRichtung elementUndRichtung) const
{
    if (!elementUndRichtung.hatVorgaenger() || elementUndRichtung.vorgaengerElementeSindInAnderemModul())
    {
        return true;
    }

    const auto& vorgaenger = elementUndRichtung.vorgaenger();
    return !vorgaenger.hatNachfolger(0) || vorgaenger.nachfolgerElementeSindInAnderemModul() ||
            (vorgaenger.nachfolger(0) != elementUndRichtung) ||
            istSegmentGrenze(vorgaenger, elementUndRichtung);
}

bool Segmentierer::istSegmentEnde(const StreckenelementUndRichtung elementUndRichtung) const
{
    return istSegmentStart(elementUndRichtung.gegenrichtung());
}

bool Segmentierer::beideRichtungen() const
{
    return false;
}

bool RichtungsInfoSegmentierer::istSegmentGrenze(const StreckenelementUndRichtung vorgaenger, const StreckenelementUndRichtung nachfolger) const
{
    const auto& vorgaengerRichtungsInfo = vorgaenger.richtungsInfo();
    const auto& nachfolgerRichtungsInfo = nachfolger.richtungsInfo();
    if (vorgaengerRichtungsInfo.has_value() != nachfolgerRichtungsInfo.has_value()) {
        return true;
    } else {
        return !vorgaengerRichtungsInfo.has_value() || istSegmentGrenze(*vorgaengerRichtungsInfo, *nachfolgerRichtungsInfo);
    }
}

bool RichtungsInfoSegmentierer::istSegmentEnde(const StreckenelementUndRichtung elementUndRichtung) const
{
    if (!elementUndRichtung.hatNachfolger())
    {
        return true;
    }

    auto nachfolger = elementUndRichtung.nachfolger();
    return !nachfolger.hatVorgaenger() || nachfolger.vorgaengerElementeSindInAnderemModul() ||
            (nachfolger.vorgaenger(0) != elementUndRichtung) ||
            istSegmentGrenze(elementUndRichtung, nachfolger);
}

bool RichtungsInfoSegmentierer::beideRichtungen() const
{
    return true;
}

bool GleisfunktionSegmentierer::istSegmentGrenze(const StreckenelementUndRichtung vorgaenger, const StreckenelementUndRichtung nachfolger) const
{
    return hatFktFlag(*vorgaenger, StreckenelementFlag::KeineGleisfunktion) !=
            hatFktFlag(*nachfolger, StreckenelementFlag::KeineGleisfunktion);
}

bool GeschwindigkeitSegmentierer::istSegmentGrenze(const StreckenelementRichtungsInfo &vorgaenger, const StreckenelementRichtungsInfo &nachfolger) const
{
    return vorgaenger.vMax != nachfolger.vMax;
}
