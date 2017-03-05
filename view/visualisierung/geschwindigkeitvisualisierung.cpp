#include "geschwindigkeitvisualisierung.h"

#include <cmath>

#include <QPen>
#include <QColor>

#include "zusi_file_lib/src/model/streckenelement.hpp"
#include "zusi_file_lib/src/common/types.hpp"

#include "view/graphicsitems/streckensegmentitem.h"
#include "view/graphicsitems/label.h"

static QColor farbe(int geschwindigkeit)
{
    if (geschwindigkeit < 0) {
        return Qt::lightGray;
    } else {
        static constexpr int colormap[17] = { 300, 286, 270, 258, 247, 214, 197, 180, 160, 130, 100, 70, 58, 50, 40, 30, 0 };
        int hue = colormap[std::min(16, (geschwindigkeit - 1) / 10)];  /* <= 10 km/h, <= 20 km/h, ..., >= 170 km/h */
        return QColor::fromHsv(hue, 255, hue == 130 ? 230 : 255);
    }
}

void GeschwindigkeitVisualisierung::setzeDarstellung(StreckensegmentItem& item) const
{
    QPen pen = item.pen();
    geschwindigkeit_t geschwindigkeit = item.start().richtungsInfo().vmax;
    if (item.start()->hatFktFlag(StreckenelementFlag::KeineGleisfunktion)) {
        pen.setColor(QColor::fromRgb(240, 240, 240));
    } else {
        pen.setColor(farbe(std::round(geschwindigkeit * 3.6)));
    }
    item.setPen(pen);
}

unique_ptr<QGraphicsScene> GeschwindigkeitVisualisierung::legende() const
{
    auto result = std::make_unique<QGraphicsScene>();
    auto segmentierer = this->segmentierer();

    Streckenelement streckenelement;
    for (int v = 0; v <= 170; v += 10) {
        streckenelement.p1.x = result->sceneRect().right() + 5.0;
        streckenelement.p2.x = streckenelement.p1.x + 25.0;
        streckenelement.richtung(Streckenelement::RICHTUNG_NORM).richtungsInfo().vmax = v / 3.6;
        // TODO: Referenz auf Stackvariable (streckenelement)!
        auto segmentItem = std::make_unique<StreckensegmentItem>(
                            streckenelement.richtung(Streckenelement::RICHTUNG_NORM),
                            *segmentierer, 0, nullptr);
        this->setzeDarstellung(*segmentItem);
        segmentItem->setBreite(5);
        result->addItem(segmentItem.release());

        auto label = std::make_unique<Label>(v == 0 ? QString("undefiniert") :
            (v == 170 ? QString::fromUtf8("> 160") : (QString::fromUtf8("⩽ ") + QString::number(v))));
        label->setAlignment(Qt::AlignVCenter);
        label->setPos(streckenelement.p2.x + 3.0, 0);
        label->setPen(QPen(Qt::black));
        label->setBrush(QBrush(Qt::black));
        result->addItem(label.release());
    }
    return result;
}