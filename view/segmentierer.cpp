#include "segmentierer.h"

bool Segmentierer::istSegmentGrenze(const StreckenelementUndRichtung &vorgaenger, const StreckenelementUndRichtung &nachfolger)
{
    (void)vorgaenger;
    (void)nachfolger;
    return false;
}

bool Segmentierer::istSegmentStart(const StreckenelementUndRichtung &elementUndRichtung)
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

bool RichtungsInfoSegmentierer::istSegmentGrenze(const StreckenelementUndRichtung &vorgaenger, const StreckenelementUndRichtung &nachfolger)
{
    return istSegmentGrenze(vorgaenger.richtungsInfo(), nachfolger.richtungsInfo()) ||
            istSegmentGrenze(vorgaenger.gegenrichtung().richtungsInfo(), nachfolger.gegenrichtung().richtungsInfo());
}

bool GleisfunktionSegmentierer::istSegmentGrenze(const StreckenelementUndRichtung &vorgaenger, const StreckenelementUndRichtung &nachfolger)
{
    return vorgaenger->hatFktFlag(StreckenelementFlag::KeineGleisfunktion) !=
            nachfolger->hatFktFlag(StreckenelementFlag::KeineGleisfunktion);
}

bool GeschwindigkeitSegmentierer::istSegmentGrenze(const StreckenelementRichtungsInfo &vorgaenger, const StreckenelementRichtungsInfo &nachfolger)
{
    return vorgaenger.vmax != nachfolger.vmax;
}
