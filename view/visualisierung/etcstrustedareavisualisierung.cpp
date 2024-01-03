#include "etcstrustedareavisualisierung.h"

#include <QPen>
#include <QColor>

bool EtcsTrustedAreaSegmentierer::istSegmentGrenze(const StreckenelementUndRichtung vorgaenger, const StreckenelementUndRichtung nachfolger) const
{
    const bool gleisfunktionGleich = (hatFktFlag(*vorgaenger, StreckenelementFlag::KeineGleisfunktion) ==
            hatFktFlag(*nachfolger, StreckenelementFlag::KeineGleisfunktion));
    const bool trustedAreaGleich = (hatFktFlag(*vorgaenger, StreckenelementFlag::EtcsTrustedArea) ==
            hatFktFlag(*nachfolger, StreckenelementFlag::EtcsTrustedArea));
    return !gleisfunktionGleich || !trustedAreaGleich;
}

void EtcsTrustedAreaVisualisierung::setzeDarstellung(StreckensegmentItem& item)
{
    QPen pen = item.pen();
    pen.setColor(hatFktFlag(*item.start(), StreckenelementFlag::EtcsTrustedArea) ? QColor::fromRgb(0, 200, 0) : Qt::black);
    item.setPen(pen);
}

std::unique_ptr<QGraphicsScene> EtcsTrustedAreaVisualisierung::legende()
{
    auto result = std::make_unique<QGraphicsScene>();
    auto segmentierer = this->segmentierer();
    StrElement streckenelement {};
    for (int i = 0; i <= 1; ++i) {
        streckenelement.Fkt = (i == 0 ? 0 : static_cast<int64_t>(StreckenelementFlag::EtcsTrustedArea));
        this->neuesLegendeElement(*result, *segmentierer, streckenelement,
            i == 0 ? QString::fromUtf8("Keine Trusted Area") : QString::fromUtf8("Trusted Area"));
    }
    return result;
}
