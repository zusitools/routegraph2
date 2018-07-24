#ifndef NEIGUNGVISUALISIERUNG_H
#define NEIGUNGVISUALISIERUNG_H

#include "view/visualisierung/visualisierung.h"

#include "view/segmentierer.h"
#include "view/graphicsitems/streckensegmentitem.h"

#include <QGraphicsScene>

#include <memory>

class NeigungSegmentierer : public Segmentierer
{
protected:
    virtual bool istSegmentGrenze(const StreckenelementUndRichtung vorgaenger, const StreckenelementUndRichtung nachfolger) const override;
};

class NeigungVisualisierung : public Visualisierung
{
public:
    void setzeDarstellung(StreckensegmentItem& item) override;
    std::unique_ptr<QGraphicsScene> legende() override;
    std::unique_ptr<Segmentierer> segmentierer() const override { return std::make_unique<NeigungSegmentierer>(); }
};

#endif // NEIGUNGVISUALISIERUNG_H
