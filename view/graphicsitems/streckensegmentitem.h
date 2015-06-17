#ifndef STRECKENELEMENTITEM_H
#define STRECKENELEMENTITEM_H

#include "view/graphicsitems/minbreitegraphicsitem.h"
#include "zusi_file_lib/src/model/streckenelement.hpp"

#include <QGraphicsLineItem>

using namespace std;

/** Ein Pfad, der eine Folge von Streckenelementen beschreibt */
class StreckensegmentItem : public MinBreiteGraphicsItem<QGraphicsPathItem>
{
public:
    explicit StreckensegmentItem(StreckenelementUndRichtung start,
        bool (*istSegmentStart)(StreckenelementUndRichtung),
        void (*setzeDarstellung)(StreckensegmentItem &, const StreckenelementUndRichtung &),
        QGraphicsItem *parent = 0);

    StreckenelementUndRichtung ende;

private:
};



#endif // STRECKENELEMENTITEM_H
