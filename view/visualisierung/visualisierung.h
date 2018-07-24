#ifndef VISUALISIERUNG_H
#define VISUALISIERUNG_H

#include <QGraphicsScene>

#include "view/segmentierer.h"
#include "view/graphicsitems/streckensegmentitem.h"

class Visualisierung
{
protected:
    void neuesLegendeElement(QGraphicsScene& scene, Segmentierer& segmentierer, StrElement &streckenelement, QString legende);
    static constexpr qreal legendeElementLaenge = 25.0;
public:
    virtual void setzeDarstellung(StreckensegmentItem&) { }
    virtual std::unique_ptr<Segmentierer> segmentierer() const { return std::make_unique<GleisfunktionSegmentierer>(); }
    virtual std::unique_ptr<QGraphicsScene> legende() = 0;

    Visualisierung() = default;
    Visualisierung(Visualisierung& other) = delete;
    virtual ~Visualisierung() { }
};

#endif // VISUALISIERUNG_H
