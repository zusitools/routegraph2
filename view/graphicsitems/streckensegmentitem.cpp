#include "streckensegmentitem.h"

#include "view/zwerte.h"

StreckensegmentItem::StreckensegmentItem(const StreckenelementUndRichtung &start,
                                         Segmentierer istSegmentStart,
                                         void (*setzeDarstellung)(StreckensegmentItem &, const StreckenelementUndRichtung &),
                                         QGraphicsItem *parent) :
    MinBreiteGraphicsItem<QGraphicsPathItem>(parent, 1.0f), ende(start)
{
    this->setZValue(ZWERT_GLEIS);
    QPainterPath path;

    if (start.richtung == Streckenelement::RICHTUNG_NORM)
    {
        path.moveTo(start->p1.x, start->p1.y);
    }
    else
    {
        path.moveTo(start->p2.x, start->p2.y);
    }

    auto cur = start;
    do {
        if (cur.richtung == Streckenelement::RICHTUNG_NORM) {
            path.lineTo(cur->p2.x, cur->p2.y);
        } else {
            path.lineTo(cur->p1.x, cur->p1.y);
        }

        if (istSegmentStart(cur.gegenrichtung()))
        {
            break;
        }
        cur = cur.nachfolger();
    } while (!istSegmentStart(cur));

    this->setPath(path);
    this->setToolTip(QString::number(start->nr));
    this->ende = cur;
    setzeDarstellung(*this, start);
}
