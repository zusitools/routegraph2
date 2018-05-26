#include "fahrleitungvisualisierung.h"

#include <cmath>

#include <QPen>
#include <QColor>

#include "model/streckenelement.h"

#include "view/graphicsitems/streckensegmentitem.h"
#include "view/graphicsitems/label.h"

bool FahrleitungSegmentierer::istSegmentGrenze(const StreckenelementUndRichtung &vorgaenger, const StreckenelementUndRichtung &nachfolger) const
{
    return vorgaenger->Volt != nachfolger->Volt || ((vorgaenger->Drahthoehe == 0) != (nachfolger->Drahthoehe == 0));
}

void FahrleitungVisualisierung::setzeDarstellung(StreckensegmentItem& item)
{
    QPen pen = item.pen();

    FahrleitungTyp fahrleitungTyp = static_cast<FahrleitungTyp>(item.start()->Volt);
    auto drahthoehe = item.start()->Drahthoehe;

    auto it = this->farben_.find(fahrleitungTyp);
    if (it == std::end(this->farben_)) {
        pen.setColor(QColor(0, 0, 0));
    } else {
        pen.setColor(it->second.second);
    }
    if (fahrleitungTyp != FahrleitungTyp::Ohne && drahthoehe == 0) {
        pen.setStyle(Qt::DotLine);
    }
    item.setPen(pen);
}

unique_ptr<QGraphicsScene> FahrleitungVisualisierung::legende()
{
    auto result = std::make_unique<QGraphicsScene>();
    auto segmentierer = this->segmentierer();
    StrElement streckenelement {};
    streckenelement.Drahthoehe = 1;
    for (const auto& it : this->farben_) {
        streckenelement.Volt = static_cast<int32_t>(it.first);
        this->neuesLegendeElement(*result, *segmentierer, streckenelement, QString::fromUtf8(it.second.first.data(), it.second.first.size()));
    }
    streckenelement.Volt = static_cast<int32_t>(FahrleitungTyp::Unbestimmt);
    streckenelement.Drahthoehe = 0;
    this->neuesLegendeElement(*result, *segmentierer, streckenelement, QString("gestrichelt = Fahrdrahthöhe 0"));
    return result;
}

FahrleitungVisualisierung::FahrleitungVisualisierung() {
    farben_ = {
        // Farben von http://www.bueker.net/trainspotting/maps_germany.php
        {FahrleitungTyp::Ohne, {"Ohne", QColor(40, 214, 40)}},
        {FahrleitungTyp::Unbestimmt, {"Unbestimmt", QColor(128, 128, 128)}},
        {FahrleitungTyp::AC_15000V_16_7_Hz, {"15 kV, 16,7 Hz", QColor(255, 0, 0)}},
        {FahrleitungTyp::AC_25000V_50_Hz, {"25 kV, 50 Hz", QColor(0, 121, 255)}},
        {FahrleitungTyp::DC_1500V, {"1500 V, Gleichstrom", QColor(206, 133, 89)}},
        {FahrleitungTyp::DC_1200V_Stromschiene, {"1200 V, Gleichstrom (Stromschiene)", QColor(255, 149, 202)}},
        {FahrleitungTyp::DC_3000V, {"3 kV, Gleichstrom", QColor(0, 202, 202)}},
    };
}
