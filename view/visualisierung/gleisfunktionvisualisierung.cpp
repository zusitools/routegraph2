#include "gleisfunktionvisualisierung.h"

#include <QPen>
#include <QColor>

void GleisfunktionVisualisierung::setzeDarstellung(StreckensegmentItem& item)
{
    QPen pen = item.pen();
    pen.setColor(item.start()->hatFktFlag(StreckenelementFlag::KeineGleisfunktion) ? Qt::lightGray : Qt::black);
    item.setPen(pen);
}
