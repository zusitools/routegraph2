#include "streckensegmentitem.h"

#include <cmath>

#include "view/zwerte.h"

StreckensegmentItem::StreckensegmentItem(const StreckenelementUndRichtung &start,
                                         const Segmentierer& segmentierer, float offset,
                                         QGraphicsItem *parent) :
    MinBreiteGraphicsItem<QGraphicsPathItem>(parent, 1.0f), start_(start)
{
    this->setZValue(ZWERT_GLEIS);
    QPainterPath path;
    auto p1 = start.gegenrichtung().endpunkt();
    auto p2 = start.endpunkt();

    Vec3 vec {};
    float veclen = 0;

    if (offset == 0.0f)
    {
        path.moveTo(p1.X, p1.Y);
        path.lineTo(p2.X, p2.Y);
    }
    else
    {
        vec = p2 - p1;
        // Der transformierte Punkt ist (p1.X - sin(phi) * offset, p1.Y - cos(phi) * offset)
        // Dabei ist phi der Winkel, den das Streckenelement bildet, also atan2(-vec.Y, vec.X).
        // Es gilt: sin(atan2(y, x)) = x / sqrt(x^2+y^2), analog fuer cos.
        veclen = std::sqrt(vec.X * vec.X + vec.Y * vec.Y);
        path.moveTo(p1.X + (vec.Y * offset / veclen), p1.Y - (vec.X * offset / veclen));
        path.lineTo(p2.X + (vec.Y * offset / veclen), p2.Y - (vec.X * offset / veclen));
    }

    auto cur = start;
    while (!segmentierer.istSegmentEnde(cur))
    {
        cur = cur.nachfolger();
        p1 = p2;
        p2 = cur.endpunkt();

        if (offset == 0.0f)
        {
            path.lineTo(p2.X, p2.Y);
        }
        else
        {
            vec = p2 - p1;
            veclen = std::sqrt(vec.X * vec.X + vec.Y * vec.Y);
            path.lineTo(p2.X + (vec.Y * offset / veclen), p2.Y - (vec.X * offset / veclen));
        }
    }

    this->setPath(path);
    this->setToolTip(QString::number(start->Nr));
    this->ende_ = cur;
}
