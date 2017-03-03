#include "label.h"

#include <QPainter>
#include <QStyleOptionGraphicsItem>

#include <limits>

#include "view/zwerte.h"

Label::Label(const QString text, QGraphicsItem *parent) : QGraphicsSimpleTextItem(text, parent), m_alignment(0)
{
    this->setZValue(ZWERT_BESCHRIFTUNG);
}

QRectF Label::boundingRect() const
{
    QRectF result = QGraphicsSimpleTextItem::boundingRect();

    if (m_alignment & Qt::AlignRight) {
        result.translate(-result.width(), 0);
    } else if (m_alignment & Qt::AlignCenter) {
        result.translate(-result.width() / 2.0, 0);
    }

    if (m_alignment & Qt::AlignTop) {
        result.translate(0, -result.height());
    } else if (m_alignment & Qt::AlignHCenter) {
        result.translate(0, -result.height() / 2.0);
    }

    return result;
}

void Label::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget)
{
    Q_UNUSED(option);
    Q_UNUSED(widget);

    painter->setPen(Qt::NoPen);
    painter->setBrush(QBrush(Qt::white));
    painter->setOpacity(0.75);
    painter->drawRect(this->boundingRect());

    painter->setPen(this->pen());
    painter->setBrush(this->brush());
    painter->setOpacity(1);
    painter->drawText(this->boundingRect(), this->text());
}
