#include "visualisierung.h"

#include "view/graphicsitems/label.h"

void Visualisierung::neuesLegendeElement(QGraphicsScene& scene, Segmentierer& segmentierer, StrElement &streckenelement, QString legende)
{
    streckenelement.g.X = scene.sceneRect().right() + 5.0;
    streckenelement.b.X = streckenelement.g.X + Visualisierung::legendeElementLaenge;
    // TODO: Referenz auf Stackvariable (streckenelement)!
    auto segmentItem = std::make_unique<StreckensegmentItem>(
                        richtung(streckenelement, StreckenelementRichtung::Norm),
                        segmentierer, 0, nullptr);
    this->setzeDarstellung(*segmentItem);
    segmentItem->setBreite(5);
    scene.addItem(segmentItem.release());

    auto label = std::make_unique<Label>(legende);
    label->setAlignment(Qt::AlignVCenter);
    label->setPos(streckenelement.b.X + 3.0, 0);
    label->setPen(QPen(Qt::black));
    label->setBrush(QBrush(Qt::black));
    scene.addItem(label.release());
}
