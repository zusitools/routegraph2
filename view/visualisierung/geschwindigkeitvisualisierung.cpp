#include "geschwindigkeitvisualisierung.h"

#include <cmath>

#include <QPen>
#include <QColor>

#include "model/streckenelement.h"

#include "view/graphicsitems/streckensegmentitem.h"
#include "view/graphicsitems/label.h"

static QColor farbe(int geschwindigkeit)
{
    if (geschwindigkeit <= 0) {
        return Qt::lightGray;
    } else {
        static constexpr int colormap[17] = { 300, 286, 270, 258, 247, 214, 197, 180, 160, 130, 100, 70, 58, 50, 40, 30, 0 };
        int hue = colormap[std::min(16, (geschwindigkeit - 1) / 10)];  /* <= 10 km/h, <= 20 km/h, ..., >= 170 km/h */
        return QColor::fromHsv(hue, 255, hue == 130 ? 230 : 255);
    }
}

void GeschwindigkeitVisualisierung::setzeDarstellung(StreckensegmentItem& item)
{
    QPen pen = item.pen();
    const auto& start = item.start();
    const auto& richtungsInfo = start.richtungsInfo();
    const auto geschwindigkeit = richtungsInfo.has_value() ? richtungsInfo->vMax : 0;
    if (hatFktFlag(*start, StreckenelementFlag::KeineGleisfunktion)) {
        pen.setColor(QColor::fromRgb(240, 240, 240));
    } else {
        pen.setColor(farbe(std::round(geschwindigkeit * 3.6)));
    }
    item.setPen(pen);
}

unique_ptr<QGraphicsScene> GeschwindigkeitVisualisierung::legende()
{
    auto result = std::make_unique<QGraphicsScene>();
    auto segmentierer = this->segmentierer();
    StrElement streckenelement {};
    for (int v = 0; v <= 170; v += 10) {
        streckenelement.InfoNormRichtung.emplace();
        streckenelement.InfoNormRichtung->vMax = v / 3.6;
        this->neuesLegendeElement(*result, *segmentierer, streckenelement, v == 0 ? QString("undefiniert") :
            (v == 170 ? QString::fromUtf8("> 160") : (QString::fromUtf8("⩽ ") + QString::number(v))));
    }
    return result;
}
