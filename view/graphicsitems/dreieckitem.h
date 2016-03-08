#ifndef DREIECKITEM_H
#define DREIECKITEM_H

#include <memory>

#include <QGraphicsItem>
#include <QGraphicsSimpleTextItem>
#include <QColor>

#include "label.h"

using namespace std;

/**
 * Eine Markierung (mit Text) in Form eines gleichseitigen Dreiecks.
 * Der Ursprung liegt in einer der Seiten; das Dreieck zeigt in die Richtung,
 * die durch den Winkel phi (in Grad, gegen den Uhrzeigersinn) vorgegeben ist.
 */
class DreieckItem: public QGraphicsItem
{
public:
    DreieckItem(qreal phi, const QString text, const QColor farbe, QGraphicsItem *parent = 0);
    ~DreieckItem() = default;

    QRectF boundingRect() const override;
    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget) override;
private:
    double m_phi; // im Bogenmass
    QString m_text;
    QColor m_farbe;
    QPointF m_points[3];

    unique_ptr<Label> m_label;
};

#endif // DREIECKITEM_H
