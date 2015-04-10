#include "streckescene.h"

#include "view/graphicsitems/streckensegmentitem.h"

bool istSegmentStart(StreckenelementUndRichtung elementUndRichtung)
{
    if (!elementUndRichtung.hatVorgaenger())
    {
        return true;
    }

    auto vorgaenger = elementUndRichtung.vorgaenger();
    if (!vorgaenger.hatNachfolger())
    {
        return true;
    }

    return vorgaenger.nachfolger().streckenelement.lock().get() != elementUndRichtung.streckenelement.lock().get();
}

StreckeScene::StreckeScene(unique_ptr<Strecke> &strecke, QObject *parent) :
    QGraphicsScene(parent)
{
    if (strecke == nullptr)
    {
        throw std::invalid_argument("Versuch, ein StreckenelementItem mit streckenelement == nullptr zu konstruieren");
    }

    this->setItemIndexMethod(QGraphicsScene::NoIndex);

    for (auto streckenelement : strecke->streckenelemente)
    {
        if (streckenelement != nullptr && istSegmentStart(StreckenelementUndRichtung(streckenelement, Streckenelement::RICHTUNG_NORM)))
        {
            this->addItem(new StreckensegmentItem(
                                                  StreckenelementUndRichtung(streckenelement, Streckenelement::RICHTUNG_NORM),
                                                  istSegmentStart, nullptr));
        }
    }
}
