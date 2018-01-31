#include "kruemmungvisualisierung.h"

#include "zusi_file_lib/src/model/streckenelement.hpp"
#include "zusi_file_lib/src/common/types.hpp"

#include <QPen>
#include <QColor>

#include <cmath>

namespace {
    constexpr static qreal maxRadius = 10000;  // alles ueber diesem Radius ist schwarz
    constexpr static qreal midRadius = 1000;  // bis zu diesem Radius geht die Farbe von schwarz nach gruen ueber
    constexpr static qreal minRadius = 100;  // bis zu diesem Radius geht die Farbe von gruen (h=83) ueber gelb nach rot (h=0) ueber

    constexpr static qreal maxRadiusKruemmung = 1.0/maxRadius;
    constexpr static qreal midRadiusKruemmung = 1.0/midRadius;
    constexpr static qreal minRadiusKruemmung = 1.0/minRadius;
}

bool KruemmungSegmentierer::istSegmentGrenze(const StreckenelementUndRichtung &vorgaenger, const StreckenelementUndRichtung &nachfolger) const
{
    const bool gleisfunktionGleich = (vorgaenger->hatFktFlag(StreckenelementFlag::KeineGleisfunktion) ==
            nachfolger->hatFktFlag(StreckenelementFlag::KeineGleisfunktion));
    const qreal vorgaengerKruemmung = std::fabs(vorgaenger->kruemmung);
    const qreal nachfolgerKruemmung = std::fabs(nachfolger->kruemmung);
    const bool kruemmungGleich =
            (vorgaengerKruemmung < maxRadiusKruemmung && nachfolgerKruemmung < maxRadiusKruemmung) ||
            (vorgaengerKruemmung > minRadiusKruemmung && nachfolgerKruemmung > minRadiusKruemmung) ||
            (vorgaengerKruemmung == nachfolgerKruemmung);  // absichtlicher bitweiser Float-Vergleich! Sonst fehlerhafte Segmentierung bei mehreren aufeinanderfolgenden Segmenten
    return !gleisfunktionGleich || !kruemmungGleich;
}

void KruemmungVisualisierung::setzeDarstellung(StreckensegmentItem& item)
{
    QPen pen = item.pen();

    const qreal kruemmung = std::fabs(item.start()->kruemmung);
    qreal farbton = 83;
    qreal saettigung = 255;
    qreal wert = 0;
    if (kruemmung >= midRadiusKruemmung) {
        // Kleine Radien (Weichen): Lineare Interpolation ueber den Radius
        wert = 255;
        farbton = std::min(qreal(83), std::max(qreal(0), 83 - (1/kruemmung - midRadius)/(minRadius - midRadius) * 83));
    } else if (kruemmung >= maxRadiusKruemmung) {
        // Grosse Radien (Gleisboegen): Lineare Interpolation ueber die Kruemmung (bei einer Klothoide nimmt die Kruemmung linear zu)
        wert = std::min(qreal(255), std::max(qreal(0), 255 - (kruemmung - midRadiusKruemmung)/(maxRadiusKruemmung - midRadiusKruemmung) * 255));
    }

    if (item.start()->hatFktFlag(StreckenelementFlag::KeineGleisfunktion)) {
        saettigung /= 2;
        wert = 192 + saettigung / 2;
    }

    pen.setColor(QColor::fromHsv(farbton, saettigung, wert));
    item.setPen(pen);
}

std::unique_ptr<QGraphicsScene> KruemmungVisualisierung::legende()
{
    auto result = std::make_unique<QGraphicsScene>();
    auto segmentierer = this->segmentierer();
    Streckenelement streckenelement;
    for (qreal r  : { minRadius, (minRadius+midRadius)/2, midRadius, 1/((midRadiusKruemmung+maxRadiusKruemmung)/2), maxRadius }) {
        streckenelement.kruemmung = 1/r;
        this->neuesLegendeElement(*result, *segmentierer, streckenelement,
            (r == maxRadius ? QString::fromUtf8("r ⩾ ") :
                (r == minRadius ? QString::fromUtf8("r ⩽ ") : (QString::fromUtf8("r = "))))
            + QString::number(r));
    }
    return result;
}
