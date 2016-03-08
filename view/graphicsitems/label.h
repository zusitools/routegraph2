#ifndef LABEL_H
#define LABEL_H

#include <QGraphicsSimpleTextItem>

class Label : public QGraphicsSimpleTextItem
{
public:
    Label(const QString text, QGraphicsItem *parent = 0);

    QRectF boundingRect() const override;
    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget) override;

    Qt::Alignment alignment() const { return this->m_alignment; }
    void setAlignment(Qt::Alignment alignment) { this->m_alignment = alignment; }

private:
    Qt::Alignment m_alignment;
};

#endif // LABEL_H
