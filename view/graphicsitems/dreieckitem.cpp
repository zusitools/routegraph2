#include "dreieckitem.h"

#include <QPainter>
#include <QStyleOptionGraphicsItem>

#define _USE_MATH_DEFINES
#define _BSD_SOURCE
#include <math.h>

#include "view/zwerte.h"

// Maße des Dreiecks (in Szeneneinheiten = Metern)
#define SEITENLAENGE 6.0
#define HOEHE (SEITENLAENGE * sqrt(3.0) / 2)

DreieckItem::DreieckItem(qreal phi, const QString text, const QColor farbe, QGraphicsItem *parent)
    : QGraphicsItem(parent), m_phi(phi / 180 * M_PI), m_text(text), m_farbe(farbe)
{
    // Y-Koordinate invertieren, da sie bei Qts Koordinatensystem nach unten statt nach oben zeigt
    this->m_points[0] = QPointF(HOEHE * cos(this->m_phi), -(HOEHE * sin(this->m_phi)));
    this->m_points[1] = QPointF(SEITENLAENGE/2 * cos(this->m_phi - M_PI_2), -(SEITENLAENGE/2 * sin(this->m_phi - M_PI_2)));
    this->m_points[2] = QPointF(SEITENLAENGE/2 * cos(this->m_phi + M_PI_2), -(SEITENLAENGE/2 * sin(this->m_phi + M_PI_2)));

    this->setToolTip(this->m_text);
    this->setZValue(ZWERT_MARKIERUNG);

    this->m_label.reset(new Label(this->m_text, this));
    this->m_label->setPen(QPen(this->m_farbe));
    this->m_label->setBrush(QBrush(this->m_farbe));
    this->m_label->setFlag(QGraphicsItem::ItemIgnoresTransformations);

    this->m_label->setPos(this->m_points[0]); // Spitze

    // TODO: optimiert auf Zusi-3-Strecken
    auto phi_mod = fmod(phi, 360);
    if (phi_mod >= 90 && phi_mod <= 270)
    {
        this->m_label->setAlignment(Qt::AlignRight);
    }
    if (phi_mod >= 180)
    {
        this->m_label->setAlignment(this->m_label->alignment() | Qt::AlignTop);
    }
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
