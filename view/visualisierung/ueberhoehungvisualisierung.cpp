#include "ueberhoehungvisualisierung.h"

#include "zusi_file_lib/src/model/streckenelement.hpp"
#include "zusi_file_lib/src/common/types.hpp"

#include <QPen>
#include <QColor>

#include <cmath>

namespace {
    constexpr static qreal maxUeberhoehung = 0.124;  // im Bogenmass, entspricht 7.1 Grad (max. Ueberhoehung Regelspur gem. EBO)
}

bool UeberhoehungSegmentierer::istSegmentGrenze(const StreckenelementUndRichtung &vorgaenger, const StreckenelementUndRichtung &nachfolger) const
{
    const bool gleisfunktionGleich = (vorgaenger->hatFktFlag(StreckenelementFlag::KeineGleisfunktion) ==
            nachfolger->hatFktFlag(StreckenelementFlag::KeineGleisfunktion));
    const qreal vorgaengerUeberhoehung = std::fabs(vorgaenger->ueberhoehung);
    const qreal nachfolgerUeberhoehung = std::fabs(nachfolger->ueberhoehung);
    const bool ueberhoehungGleich = vorgaengerUeberhoehung == nachfolgerUeberhoehung;
    return !gleisfunktionGleich || !ueberhoehungGleich;
}

void UeberhoehungVisualisierung::setzeDarstellung(StreckensegmentItem& item)
{
    QPen pen = item.pen();

    const qreal ueberhoehung = std::fabs(item.start()->ueberhoehung);
    if (item.start()->hatFktFlag(StreckenelementFlag::KeineGleisfunktion)) {
        pen.setColor(QColor::fromRgb(192, 192 + ueberhoehung / maxUeberhoehung * (255-192), 192));
    } else {
        pen.setColor(QColor::fromRgb(0, ueberhoehung / maxUeberhoehung * 255, 0));
    }
    item.setPen(pen);
}
