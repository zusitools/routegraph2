#ifndef SEGMENTIERER_H
#define SEGMENTIERER_H

#include "model/streckenelement.h"

class Segmentierer
{
protected:
    virtual bool istSegmentGrenze(const StreckenelementUndRichtung vorgaenger, const StreckenelementUndRichtung nachfolger) const;

public:
    /* Segment-Startelemente sind Elemente, die nicht der erste Nachfolger ihres Vorgaengers sind
     * oder keinen Vorgaenger haben. Zusaetzlich kann ein beliebiger Callback angegeben werden,
     * der fuer zwei aufeinanderfolgende Elemente entscheidet, ob sie getrennt werden sollen, weil sie
     * sich in einer relevanten Eigenschaft unterscheiden. */
    virtual bool istSegmentStart(const StreckenelementUndRichtung elementUndRichtung) const;
    virtual bool istSegmentEnde(const StreckenelementUndRichtung elementUndRichtung) const;
    virtual bool beideRichtungen() const;

    Segmentierer() {}
    Segmentierer(Segmentierer& other) = delete;

    virtual ~Segmentierer() = default;
};

class RichtungsInfoSegmentierer : public Segmentierer
{
protected:
    virtual bool istSegmentGrenze(const StreckenelementUndRichtung vorgaenger, const StreckenelementUndRichtung nachfolger) const override;
    virtual bool istSegmentGrenze(const StreckenelementRichtungsInfo &vorgaenger, const StreckenelementRichtungsInfo &nachfolger) const = 0;
    virtual bool istSegmentEnde(const StreckenelementUndRichtung elementUndRichtung) const override;
    virtual bool beideRichtungen() const override;
};

class GleisfunktionSegmentierer : public Segmentierer
{
protected:
    virtual bool istSegmentGrenze(const StreckenelementUndRichtung vorgaenger, const StreckenelementUndRichtung nachfolger) const override;
};

class GeschwindigkeitSegmentierer : public RichtungsInfoSegmentierer
{
protected:
    virtual bool istSegmentGrenze(const StreckenelementRichtungsInfo &vorgaenger, const StreckenelementRichtungsInfo &nachfolger) const override;
};

#endif // SEGMENTIERER_H
