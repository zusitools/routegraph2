#ifndef GESCHWINDIGKEITVISUALISIERUNG_H
#define GESCHWINDIGKEITVISUALISIERUNG_H

#include "view/visualisierung/visualisierung.h"

#include "view/segmentierer.h"
#include "view/graphicsitems/streckensegmentitem.h"

class GeschwindigkeitVisualisierung : public Visualisierung
{
public:
    virtual void setzeDarstellung(StreckensegmentItem& item) override;
    virtual std::unique_ptr<Segmentierer> segmentierer() const override { return std::make_unique<GeschwindigkeitSegmentierer>(); }
    virtual std::unique_ptr<QGraphicsScene> legende() override;
};

#endif // GESCHWINDIGKEITVISUALISIERUNG_H
