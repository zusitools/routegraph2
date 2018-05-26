#ifndef OBERBAUVISUALISIERUNG_H
#define OBERBAUVISUALISIERUNG_H

#include <unordered_map>

#include "view/visualisierung/visualisierung.h"
#include "view/segmentierer.h"
#include "view/graphicsitems/streckensegmentitem.h"

#include "zusi_parser/zusi_types.hpp"

class OberbauSegmentierer : public Segmentierer
{
protected:
    virtual bool istSegmentGrenze(const StreckenelementUndRichtung &vorgaenger, const StreckenelementUndRichtung &nachfolger) const override;
};

class OberbauVisualisierung : public Visualisierung
{
private:
    std::unordered_map<std::string, QColor> colors_;
public:
    virtual void setzeDarstellung(StreckensegmentItem& item) override;
    virtual std::unique_ptr<Segmentierer> segmentierer() const override { return std::make_unique<OberbauSegmentierer>(); }
    virtual std::unique_ptr<QGraphicsScene> legende() override;
};

#endif // OBERBAUVISUALISIERUNG_H
