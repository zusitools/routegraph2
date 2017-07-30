#include "visualisierung.h"

#include "view/graphicsitems/label.h"

void Visualisierung::neuesLegendeElement(QGraphicsScene& scene, Segmentierer& segmentierer, Streckenelement& streckenelement, QString legende)
{
    streckenelement.p1.x = scene.sceneRect().right() + 5.0;
    streckenelement.p2.x = streckenelement.p1.x + 25.0;
    // TODO: Referenz auf Stackvariable (streckenelement)!
    auto segmentItem = std::make_unique<StreckensegmentItem>(
                        streckenelement.richtung(Streckenelement::RICHTUNG_NORM),
                        segmentierer, 0, nullptr);
    this->setzeDarstellung(*segmentItem);
    segmentItem->setBreite(5);
    scene.addItem(segmentItem.release());

    auto label = std::make_unique<Label>(legende);
    label->setAlignment(Qt::AlignVCenter);
    label->setPos(streckenelement.p2.x + 3.0, 0);
    label->setPen(QPen(Qt::black));
    label->setBrush(QBrush(Qt::black));
    scene.addItem(label.release());
}
