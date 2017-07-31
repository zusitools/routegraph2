#ifndef FAHRLEITUNGVISUALISIERUNG_H
#define FAHRLEITUNGVISUALISIERUNG_H

#include <map>

#include "view/visualisierung/visualisierung.h"
#include "view/segmentierer.h"
#include "view/graphicsitems/streckensegmentitem.h"

#include "zusi_file_lib/src/model/streckenelement.hpp"

class FahrleitungSegmentierer : public Segmentierer
{
protected:
    virtual bool istSegmentGrenze(const StreckenelementUndRichtung &vorgaenger, const StreckenelementUndRichtung &nachfolger) const override;
};

class FahrleitungVisualisierung : public Visualisierung
{
private:
    std::map<FahrleitungTyp, std::pair<std::string, QColor>> farben_;
public:
    virtual void setzeDarstellung(StreckensegmentItem& item) override;
    virtual std::unique_ptr<Segmentierer> segmentierer() const override { return std::make_unique<FahrleitungSegmentierer>(); }
    virtual std::unique_ptr<QGraphicsScene> legende() override;

    FahrleitungVisualisierung();
};

#endif // FAHRLEITUNGVISUALISIERUNG_H
