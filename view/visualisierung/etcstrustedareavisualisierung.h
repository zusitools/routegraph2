#ifndef ETCSTRUSTEDAREAVISUALISIERUNG_H
#define ETCSTRUSTEDAREAVISUALISIERUNG_H

#include <QGraphicsScene>

#include "view/graphicsitems/streckensegmentitem.h"
#include "view/visualisierung/visualisierung.h"
#include "view/segmentierer.h"

#include <memory>

class EtcsTrustedAreaSegmentierer final : public Segmentierer
{
protected:
    virtual bool istSegmentGrenze(const StreckenelementUndRichtung vorgaenger, const StreckenelementUndRichtung nachfolger) const override;
};

class EtcsTrustedAreaVisualisierung final : public Visualisierung
{
public:
    virtual void setzeDarstellung(StreckensegmentItem& item) override;
    virtual std::unique_ptr<QGraphicsScene> legende() override;
    std::unique_ptr<Segmentierer> segmentierer() const override { return std::make_unique<EtcsTrustedAreaSegmentierer>(); }
};

#endif // ETCSTRUSTEDAREAVISUALISIERUNG_H
