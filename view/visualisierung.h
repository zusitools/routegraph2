#ifndef VISUALISIERUNG_H
#define VISUALISIERUNG_H

#include "view/graphicsitems/streckensegmentitem.h"

class Visualisierung
{
public:
    virtual bool beideRichtungen() const { return false; }
    virtual float offset() const { return 0.0f; }
    virtual void setzeDarstellung(StreckensegmentItem&) const { }
};

class GleisfunktionVisualisierung : public Visualisierung
{
public:
    virtual void setzeDarstellung(StreckensegmentItem& item) const override;
};

class GeschwindigkeitVisualisierung : public Visualisierung
{
public:
    virtual bool beideRichtungen() const override { return true; }
    virtual float offset() const override { return 0.49f; }
    virtual void setzeDarstellung(StreckensegmentItem& item) const override;
};

#endif // VISUALISIERUNG_H
