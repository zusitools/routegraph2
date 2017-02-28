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
    explicit StreckensegmentItem(const StreckenelementUndRichtung& start,
        Segmentierer istSegmentStart,
        void (*setzeDarstellung)(StreckensegmentItem &, const StreckenelementUndRichtung &),
        QGraphicsItem *parent = 0);

    StreckenelementUndRichtung ende;

private:
};



#endif // STRECKENSEGMENTITEM_H
