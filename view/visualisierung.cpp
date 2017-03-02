#include "visualisierung.h"

#include <cmath>

#include <QPen>
#include <QColor>

#include "zusi_file_lib/src/model/streckenelement.hpp"
#include "zusi_file_lib/src/common/types.hpp"

void GleisfunktionVisualisierung::setzeDarstellung(StreckensegmentItem& item) const
{
    QPen pen = item.pen();
    pen.setColor(item.start()->hatFktFlag(StreckenelementFlag::KeineGleisfunktion) ? Qt::lightGray : Qt::black);
    item.setPen(pen);
}

void GeschwindigkeitVisualisierung::setzeDarstellung(StreckensegmentItem& item) const
{
    QPen pen = item.pen();
    geschwindigkeit_t geschwindigkeit = item.start().richtungsInfo().vmax;
    if (item.start()->hatFktFlag(StreckenelementFlag::KeineGleisfunktion) || geschwindigkeit <= 0) {
        pen.setColor(Qt::lightGray);
    } else {
        int colormap[17] = { 300, 286, 270, 258, 247, 214, 197, 180, 160, 130, 100, 70, 58, 50, 40, 30, 0 };
        int idx = std::round(geschwindigkeit * 3.6) / 10 - 1; /* <= 10 km/h, <= 20 km/h, ..., >= 170 km/h */
        int hue = colormap[std::max(0, std::min(16, idx))];
        pen.setColor(QColor::fromHsv(hue, 255, hue == 130 ? 230 : 255));
    }
    item.setPen(pen);
}
