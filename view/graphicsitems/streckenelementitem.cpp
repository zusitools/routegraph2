#include "streckenelementitem.h"

StreckenelementItem::StreckenelementItem(QGraphicsItem *parent, std::shared_ptr<Streckenelement> streckenelement) :
    QGraphicsLineItem(parent)
{
    if (streckenelement == nullptr)
    {
        throw std::invalid_argument("Versuch, ein StreckenelementItem mit streckenelement == nullptr zu konstruieren");
    }

    this->m_streckenelement = streckenelement;
    this->setLine(
                this->m_streckenelement->p1.x, this->m_streckenelement->p1.y,
                this->m_streckenelement->p2.x, this->m_streckenelement->p2.y);
}
