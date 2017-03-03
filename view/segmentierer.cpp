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

bool RichtungsInfoSegmentierer::istSegmentGrenze(const StreckenelementUndRichtung &vorgaenger, const StreckenelementUndRichtung &nachfolger) const
{
    return istSegmentGrenze(vorgaenger.richtungsInfo(), nachfolger.richtungsInfo()) ||
            istSegmentGrenze(nachfolger.gegenrichtung().richtungsInfo(), vorgaenger.gegenrichtung().richtungsInfo());
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
