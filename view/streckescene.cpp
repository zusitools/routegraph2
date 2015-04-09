#include "streckescene.h"

#include "view/graphicsitems/streckenelementitem.h"

StreckeScene::StreckeScene(unique_ptr<Strecke> &strecke, QObject *parent) :
    QGraphicsScene(parent)
{
    if (strecke == nullptr)
    {
        throw std::invalid_argument("Versuch, ein StreckenelementItem mit streckenelement == nullptr zu konstruieren");
    }

    for (auto streckenelement : strecke->streckenelemente)
    {
        if (streckenelement != nullptr)
        {
            this->addItem(new StreckenelementItem(nullptr, streckenelement));
        }
    }
}
