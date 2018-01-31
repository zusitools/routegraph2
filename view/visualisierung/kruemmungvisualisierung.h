#ifndef KRUEMMUNGVISUALISIERUNG_H
#define KRUEMMUNGVISUALISIERUNG_H

#include "view/visualisierung/visualisierung.h"

#include "view/segmentierer.h"
#include "view/graphicsitems/streckensegmentitem.h"

#include <QGraphicsScene>

#include <memory>

class KruemmungSegmentierer : public Segmentierer
{
protected:
    virtual bool istSegmentGrenze(const StreckenelementUndRichtung &vorgaenger, const StreckenelementUndRichtung &nachfolger) const override;
};

class KruemmungVisualisierung : public Visualisierung
{
public:
    void setzeDarstellung(StreckensegmentItem& item) override;
    std::unique_ptr<QGraphicsScene> legende() override;
    std::unique_ptr<Segmentierer> segmentierer() const override { return std::make_unique<KruemmungSegmentierer>(); }
};

#endif // KRUEMMUNGVISUALISIERUNG_H
