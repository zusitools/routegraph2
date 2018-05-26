#include "ueberhoehungvisualisierung.h"

#include "model/streckenelement.h"

#include <QPen>
#include <QColor>

#include <cmath>

namespace {
    constexpr static qreal maxUeberhoehung = 0.124;  // im Bogenmass, entspricht 7.1 Grad (max. Ueberhoehung Regelspur gem. EBO)
}

bool UeberhoehungSegmentierer::istSegmentGrenze(const StreckenelementUndRichtung &vorgaenger, const StreckenelementUndRichtung &nachfolger) const
{
    const bool gleisfunktionGleich = (hatFktFlag(*vorgaenger, StreckenelementFlag::KeineGleisfunktion) ==
            hatFktFlag(*nachfolger, StreckenelementFlag::KeineGleisfunktion));
    const qreal vorgaengerUeberhoehung = std::fabs(vorgaenger->Ueberh);
    const qreal nachfolgerUeberhoehung = std::fabs(nachfolger->Ueberh);
    const bool ueberhoehungGleich = vorgaengerUeberhoehung == nachfolgerUeberhoehung;
    return !gleisfunktionGleich || !ueberhoehungGleich;
}

void UeberhoehungVisualisierung::setzeDarstellung(StreckensegmentItem& item)
{
    QPen pen = item.pen();

    const qreal ueberhoehung = std::fabs(item.start()->Ueberh);
    if (hatFktFlag(*item.start(), StreckenelementFlag::KeineGleisfunktion)) {
        pen.setColor(QColor::fromRgb(192, 192 + std::min(qreal(1), ueberhoehung / maxUeberhoehung) * (255-192), 192));
    } else {
        pen.setColor(QColor::fromRgb(0, std::min(qreal(1), ueberhoehung / maxUeberhoehung) * 255, 0));
    }
    item.setPen(pen);
}
