#ifndef UEBERHOEHUNGVISUALISIERUNG_H
#define UEBERHOEHUNGVISUALISIERUNG_H

#include "view/visualisierung/visualisierung.h"

#include "view/segmentierer.h"
#include "view/graphicsitems/streckensegmentitem.h"

#include <QGraphicsScene>

#include <memory>

class UeberhoehungSegmentierer : public Segmentierer
{
protected:
    virtual bool istSegmentGrenze(const StreckenelementUndRichtung &vorgaenger, const StreckenelementUndRichtung &nachfolger) const override;
};

class UeberhoehungVisualisierung : public Visualisierung
{
public:
    void setzeDarstellung(StreckensegmentItem& item) override;
    std::unique_ptr<QGraphicsScene> legende() override { return nullptr; }
    std::unique_ptr<Segmentierer> segmentierer() const override { return std::make_unique<UeberhoehungSegmentierer>(); }
};

#endif // UEBERHOEHUNGVISUALISIERUNG_H
