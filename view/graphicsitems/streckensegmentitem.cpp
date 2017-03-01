#include "streckensegmentitem.h"

#include <cmath>

#include "view/zwerte.h"

StreckensegmentItem::StreckensegmentItem(const StreckenelementUndRichtung &start,
                                         Segmentierer istSegmentStart, float offset,
                                         void (*setzeDarstellung)(StreckensegmentItem &, const StreckenelementUndRichtung &),
                                         QGraphicsItem *parent) :
    MinBreiteGraphicsItem<QGraphicsPathItem>(parent, 1.0f), ende(start)
{
    this->setZValue(ZWERT_GLEIS);
    QPainterPath path;
    auto p1 = start.gegenrichtung().endpunkt();
    auto p2 = start.endpunkt();

    Punkt3D vec;
    float phi;

    if (offset == 0.0f)
    {
        path.moveTo(p1.x, p1.y);
    }
    else
    {
        vec = p2 - p1;
        phi = std::atan2(-vec.y, vec.x);
        path.moveTo(p1.x + std::sin(phi) * offset, p1.y + std::cos(phi) * offset);
    }

    auto cur = start;
    do {
        if (offset == 0.0f)
        {
            path.lineTo(p2.x, p2.y);
        }
        else
        {
            path.lineTo(p2.x + std::sin(phi) * offset, p2.y + std::cos(phi) * offset);
        }
        if (istSegmentStart(cur.gegenrichtung()))
        {
            break;
        }
        cur = cur.nachfolger();
        p1 = p2;
        p2 = cur.endpunkt();
        if (offset != 0.0f)
        {
            vec = p2 - p1;
            phi = std::atan2(-vec.y, vec.x);
        }
    } while (!istSegmentStart(cur));

    this->setPath(path);
    this->setToolTip(QString::number(start->nr));
    this->ende = cur;
    setzeDarstellung(*this, start);
}
