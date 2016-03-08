#ifndef STRECKENSEGMENTITEM_H
#define STRECKENSEGMENTITEM_H

#include "view/graphicsitems/minbreitegraphicsitem.h"
#include "zusi_file_lib/src/model/streckenelement.hpp"

#include <QGraphicsLineItem>

using namespace std;

/** Ein Pfad, der eine Folge von Streckenelementen beschreibt */
class StreckensegmentItem : public MinBreiteGraphicsItem<QGraphicsPathItem>
{
public:
    explicit StreckensegmentItem(const StreckenelementUndRichtung& start,
        bool (*istSegmentStart)(const StreckenelementUndRichtung&),
        void (*setzeDarstellung)(StreckensegmentItem &, const StreckenelementUndRichtung &),
        QGraphicsItem *parent = 0);

    StreckenelementUndRichtung ende;

private:
};



#endif // STRECKENSEGMENTITEM_H
