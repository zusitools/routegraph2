#ifndef FAHRLEITUNGVISUALISIERUNG_H
#define FAHRLEITUNGVISUALISIERUNG_H

#include <map>

#include "view/visualisierung/visualisierung.h"
#include "view/segmentierer.h"
#include "view/graphicsitems/streckensegmentitem.h"

#include "zusi_parser/zusi_types.hpp"

enum class FahrleitungTyp : decltype(StrElement::Volt) {
    Ohne = 0,
    Unbestimmt = 1,
    AC_15000V_16_7_Hz = 2,
    AC_25000V_50_Hz = 3,
    DC_1500V = 4,
    DC_1200V_Stromschiene = 5,
    DC_3000V = 6,
};

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
