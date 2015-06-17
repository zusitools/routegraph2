#include "streckensegmentitem.h"

StreckensegmentItem::StreckensegmentItem(StreckenelementUndRichtung start,
                                         bool (*istSegmentStart)(StreckenelementUndRichtung),
                                         void (*setzeDarstellung)(StreckensegmentItem&, const StreckenelementUndRichtung&),
                                         QGraphicsItem *parent) :
    MinBreiteGraphicsItem<QGraphicsPathItem>(parent, 1.0f)
{
    if (start.streckenelement.expired())
    {
        throw std::invalid_argument("Versuch, ein StreckensegmentItem mit ungueltigem Start zu konstruieren");
    }

    QPainterPath path;

    auto startelem = start.streckenelement.lock();
    if (start.richtung == Streckenelement::RICHTUNG_NORM)
    {
        path.moveTo(startelem->p1.x, startelem->p1.y);
    }
    else
    {
        path.moveTo(startelem->p2.x, startelem->p2.y);
    }

    auto cur = start;
    do {
        auto curelem = cur.streckenelement.lock();
        if (cur.richtung == Streckenelement::RICHTUNG_NORM) {
            path.lineTo(curelem->p2.x, curelem->p2.y);
        } else {
            path.lineTo(curelem->p1.x, curelem->p1.y);
        }

        if (istSegmentStart(cur.gegenrichtung()))
        {
            break;
        }
        cur= cur.nachfolger();
    } while (!istSegmentStart(cur));

    this->setPath(path);
    this->setToolTip(QString::number(start.streckenelement.lock()->nr));
    this->ende = cur;
    setzeDarstellung(*this, start);
}
