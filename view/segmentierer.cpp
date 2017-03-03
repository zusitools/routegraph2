#include "segmentierer.h"

bool Segmentierer::istSegmentGrenze(const StreckenelementUndRichtung&, const StreckenelementUndRichtung&) const
{
    return false;
}

bool Segmentierer::istSegmentStart(const StreckenelementUndRichtung &elementUndRichtung) const
{
    if (!elementUndRichtung.hatVorgaenger())
    {
        return true;
    }

    auto vorgaenger = elementUndRichtung.vorgaenger();
    return !vorgaenger.hatNachfolger() ||
            (vorgaenger.nachfolger(0) != elementUndRichtung) ||
            istSegmentGrenze(vorgaenger, elementUndRichtung);
}

bool Segmentierer::istSegmentEnde(const StreckenelementUndRichtung &elementUndRichtung) const
{
    return istSegmentStart(elementUndRichtung.gegenrichtung());
}

bool Segmentierer::beideRichtungen() const
{
    return false;
}

bool RichtungsInfoSegmentierer::istSegmentGrenze(const StreckenelementUndRichtung &vorgaenger, const StreckenelementUndRichtung &nachfolger) const
{
    return istSegmentGrenze(vorgaenger.richtungsInfo(), nachfolger.richtungsInfo());
}

bool RichtungsInfoSegmentierer::istSegmentEnde(const StreckenelementUndRichtung &elementUndRichtung) const
{
    if (!elementUndRichtung.hatNachfolger())
    {
        return true;
    }

    auto nachfolger = elementUndRichtung.nachfolger();
    return !nachfolger.hatVorgaenger() ||
            (nachfolger.vorgaenger(0) != elementUndRichtung) ||
            istSegmentGrenze(elementUndRichtung, nachfolger);
}

bool RichtungsInfoSegmentierer::beideRichtungen() const
{
    return true;
}

bool GleisfunktionSegmentierer::istSegmentGrenze(const StreckenelementUndRichtung &vorgaenger, const StreckenelementUndRichtung &nachfolger) const
{
    return vorgaenger->hatFktFlag(StreckenelementFlag::KeineGleisfunktion) !=
            nachfolger->hatFktFlag(StreckenelementFlag::KeineGleisfunktion);
}

bool GeschwindigkeitSegmentierer::istSegmentGrenze(const StreckenelementRichtungsInfo &vorgaenger, const StreckenelementRichtungsInfo &nachfolger) const
{
    return vorgaenger.vmax != nachfolger.vmax;
}
