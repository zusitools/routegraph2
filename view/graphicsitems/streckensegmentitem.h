#ifndef STRECKENSEGMENTITEM_H
#define STRECKENSEGMENTITEM_H

#include <QGraphicsLineItem>

#include "view/graphicsitems/minbreitegraphicsitem.h"
#include "view/segmentierer.h"

#include "zusi_parser/zusi_types.hpp"

using namespace std;

/** Ein Pfad, der eine Folge von Streckenelementen beschreibt */
class StreckensegmentItem : public MinBreiteGraphicsItem<QGraphicsPathItem>
{
public:
    /**
     * @brief Erzeugt ein neues Streckensegment-Item.
     * @param start Das Streckenelement und die Richtung, an der das Segment beginnt.
     * @param segmentierer Segmentierer, der angibt, wie weit das Segment reicht
     * @param offset Verschiebung der grafischen Darstellung nach rechts (in Laufrichtung gesehen)
     * @param parent Das übergeordnete GraphicsItem.
     */
    explicit StreckensegmentItem(const StreckenelementUndRichtung& start,
        const Segmentierer& segmentierer, float offset,
        QGraphicsItem *parent = 0);

    inline StreckenelementUndRichtung start() const { return start_; }
    inline StreckenelementUndRichtung ende() const { return ende_; }

private:
    StreckenelementUndRichtung start_;
    StreckenelementUndRichtung ende_;

private:
};



#endif // STRECKENSEGMENTITEM_H
