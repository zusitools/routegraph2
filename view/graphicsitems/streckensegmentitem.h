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
        std::function<bool(const StreckenelementUndRichtung&)> istSegmentStart,
        std::function<void(StreckensegmentItem &, const StreckenelementUndRichtung &)> setzeDarstellung,
        QGraphicsItem *parent = 0);

    StreckenelementUndRichtung ende;

private:
};



#endif // STRECKENSEGMENTITEM_H
