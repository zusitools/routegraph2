#include "streckensegmentitem.h"

StreckensegmentItem::StreckensegmentItem(StreckenelementUndRichtung start,
                                         bool (*istSegmentStart)(StreckenelementUndRichtung),
                                         QGraphicsItem *parent) :
    MinBreiteGraphicsItem<QGraphicsPathItem>(parent, float(qrand()) / RAND_MAX * 3.0f)
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
        path.lineTo(startelem->p2.x, startelem->p2.y);
    }
    else
    {
        path.moveTo(startelem->p2.x, startelem->p2.y);
        path.lineTo(startelem->p1.x, startelem->p1.y);
    }

    if (start.hatNachfolger()) {
        auto cur = start.nachfolger();
        while (!istSegmentStart(cur)) {
            if (cur.richtung == Streckenelement::RICHTUNG_NORM) {
                path.lineTo(cur.streckenelement.lock()->p2.x, cur.streckenelement.lock()->p2.y);
            } else {
                path.lineTo(cur.streckenelement.lock()->p1.x, cur.streckenelement.lock()->p1.y);
            }

            if (!cur.hatNachfolger())
            {
                break;
            }
            auto next = cur.nachfolger();
            if (next.hatVorgaenger() && next.vorgaenger().streckenelement.lock() != cur.streckenelement.lock())
            {
                break;
            }
            cur = next;
        }
    }

    this->setPath(path);

    QColor penColor = QColor(qrand() % 256, qrand() % 256, qrand() % 256);
    QPen pen = this->pen();
    pen.setColor(penColor);
    this->setPen(pen);
}
