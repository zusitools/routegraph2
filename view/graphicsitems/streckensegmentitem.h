#ifndef STRECKENSEGMENTITEM_H
#define STRECKENSEGMENTITEM_H

#include <QGraphicsLineItem>

#include "view/graphicsitems/minbreitegraphicsitem.h"
#include "view/segmentierer.h"

#include "zusi_file_lib/src/model/streckenelement.hpp"

using namespace std;

/** Ein Pfad, der eine Folge von Streckenelementen beschreibt */
class StreckensegmentItem : public MinBreiteGraphicsItem<QGraphicsPathItem>
{
public:
    /**
     * @brief Erzeugt ein neues Streckensegment-Item.
     * @param start Das Streckenelement und die Richtung, an der das Segment beginnt.
     * @param istSegmentStart Callback, der angibt, ob das gegebene Streckenelement und die gegebene Richtung ein neues Segment anfängt.
     * @param offset Verschiebung der grafischen Darstellung nach rechts (in Laufrichtung gesehen)
     * @param parent Das übergeordnete GraphicsItem.
     */
    explicit StreckensegmentItem(const StreckenelementUndRichtung& start,
        Segmentierer istSegmentStart, float offset,
        QGraphicsItem *parent = 0);

    inline StreckenelementUndRichtung start() const { return start_; }
    inline StreckenelementUndRichtung ende() const { return ende_; }

private:
    StreckenelementUndRichtung start_;
    StreckenelementUndRichtung ende_;

private:
};



#endif // STRECKENSEGMENTITEM_H
