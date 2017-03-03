#include "streckensegmentitem.h"

#include <cmath>

#include "view/zwerte.h"

StreckensegmentItem::StreckensegmentItem(const StreckenelementUndRichtung &start,
                                         const Segmentierer& istSegmentStart, float offset,
                                         QGraphicsItem *parent) :
    MinBreiteGraphicsItem<QGraphicsPathItem>(parent, 1.0f), start_(start)
{
    this->setZValue(ZWERT_GLEIS);
    QPainterPath path;
    auto p1 = start.gegenrichtung().endpunkt();
    auto p2 = start.endpunkt();

    Punkt3D vec {};
    float veclen = 0;

    if (offset == 0.0f)
    {
        path.moveTo(p1.x, p1.y);
    }
    else
    {
        vec = p2 - p1;
        // Der transformierte Punkt ist (p1.x + sin(phi) * offset, p1.y + cos(phi) * offset)
        // Dabei ist phi der Winkel, den das Streckenelement bildet, also atan2(-vec.y, vec.x).
        // Es gilt: sin(atan2(y, x)) = x / sqrt(x^2+y^2), analog fuer cos.
        veclen = std::sqrt(vec.x * vec.x + vec.y * vec.y);
        path.moveTo(p1.x + (-vec.y * offset / veclen), p1.y + (vec.x * offset / veclen));
    }

    auto cur = start;
    do {
        if (offset == 0.0f)
        {
            path.lineTo(p2.x, p2.y);
        }
        else
        {
            path.lineTo(p2.x + (-vec.y * offset / veclen), p2.y + (vec.x * offset / veclen));
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
            veclen = std::sqrt(vec.x * vec.x + vec.y * vec.y);
        }
    } while (!istSegmentStart(cur));

    this->setPath(path);
    this->setToolTip(QString::number(start->nr));
    this->ende_ = cur;
}
