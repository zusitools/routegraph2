#ifndef VISUALISIERUNG_H
#define VISUALISIERUNG_H

#include <QGraphicsScene>

#include "view/segmentierer.h"
#include "view/graphicsitems/streckensegmentitem.h"

class Visualisierung
{
public:
    virtual float offset() const { return 0.0f; }
    virtual void setzeDarstellung(StreckensegmentItem&) const { }
    virtual std::unique_ptr<Segmentierer> segmentierer() const { return std::make_unique<GleisfunktionSegmentierer>(); }
    virtual std::unique_ptr<QGraphicsScene> legende() const = 0;

    Visualisierung() {}
    Visualisierung(Visualisierung& other) = delete;
};

#endif // VISUALISIERUNG_H
