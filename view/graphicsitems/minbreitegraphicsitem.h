#ifndef MINBREITEGRAPHICSITEM_H
#define MINBREITEGRAPHICSITEM_H

#include <QGraphicsItem>
#include <QPainter>
#include <QStyleOptionGraphicsItem>
#include <QWidget>

/** Ein GraphicsItem, das auf dem Bildschirm immer mindestens 0.5 Pixel breit ist.
 *  T muss von QAbstractGraphicsShapeItem ableiten. */
template<typename T>
class MinBreiteGraphicsItem : public T
{
public:
    explicit MinBreiteGraphicsItem<T>(QGraphicsItem *parent = 0, qreal breite = 1.0f) : T(parent), m_breite(breite), m_stiftbreite(-1.0f)
    {
    }

    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget) override
    {
        qreal skalierung = option->levelOfDetailFromTransform(painter->worldTransform());

        // Der Stift wird mit einer Breite von mindestens 1 Bildschirmpixeln gezeichnet (also pen->width * skalierung >= 1)
        QPen pen = this->pen();
        if (this->m_breite * skalierung < 1)
        {
            pen.setWidth(0);
            pen.setCosmetic(true);
        }
        else
        {
            pen.setWidth(this->m_breite);
            pen.setCosmetic(false);
        }
        pen.setCapStyle(Qt::FlatCap);
        this->setPen(pen);

        T::paint(painter, option, widget);
    }

private:
    qreal m_breite;
    qreal m_stiftbreite;
};

#endif // MINBREITEGRAPHICSITEM_H
