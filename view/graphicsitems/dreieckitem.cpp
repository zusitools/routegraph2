#include "dreieckitem.h"

#include <QPainter>
#include <QStyleOptionGraphicsItem>

#include <cmath>

// Maße des Dreiecks (in Szeneneinheiten = Metern)
#define SEITENLAENGE 6.0
#define HOEHE (SEITENLAENGE * sqrt(3.0) / 2)

DreieckItem::DreieckItem(qreal phi, const QString text, const QColor farbe, QGraphicsItem *parent)
    : QGraphicsItem(parent), m_phi(phi / 180 * M_PI), m_text(text), m_farbe(farbe),
      m_points({
          // Y-Koordinate invertieren, da sie bei Qts Koordinatensystem nach unten statt nach oben zeigt
          QPointF(HOEHE * cos(this->m_phi), -(HOEHE * sin(this->m_phi))),
          QPointF(SEITENLAENGE/2 * cos(this->m_phi - M_PI_2), -(SEITENLAENGE/2 * sin(this->m_phi - M_PI_2))),
          QPointF(SEITENLAENGE/2 * cos(this->m_phi + M_PI_2), -(SEITENLAENGE/2 * sin(this->m_phi + M_PI_2))),
      })
{
    this->setToolTip(this->m_text);

    this->m_label.reset(new QGraphicsSimpleTextItem(this->m_text, this));
    this->m_label->setPen(Qt::NoPen);
    this->m_label->setBrush(QBrush(this->m_farbe));
    this->m_label->setFlag(QGraphicsItem::ItemIgnoresTransformations);

    this->m_label->setPos(this->m_points[0]); // Spitze
}

QRectF DreieckItem::boundingRect() const
{
    return QRectF(-SEITENLAENGE, -SEITENLAENGE, 2 * SEITENLAENGE, 2 * SEITENLAENGE);
}

void DreieckItem::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget)
{
    Q_UNUSED(widget);

    qreal lod = option->levelOfDetailFromTransform(painter->worldTransform());
    this->m_label->setVisible(lod > 0.5);
    if (lod <= 0.1) return;

    painter->setPen(Qt::NoPen);
    painter->setBrush(this->m_farbe);
    painter->drawConvexPolygon(this->m_points, 3);
}
