#include "streckeview.h"

StreckeView::StreckeView(QWidget *parent) :
    QGraphicsView(parent)
{
    this->setDragMode(QGraphicsView::ScrollHandDrag);
    this->setRenderHint(QPainter::Antialiasing);
}

void StreckeView::skalieren(double faktor)
{
    this->scale(faktor, faktor);
}

void StreckeView::vergroessern()
{
    this->skalieren(1.1);
}

void StreckeView::verkleinern()
{
    this->skalieren(1.0/1.1);
}

void StreckeView::wheelEvent(QWheelEvent *event)
{
    if (event->modifiers() == Qt::ControlModifier)
     {
        this->setTransformationAnchor(QGraphicsView::AnchorUnderMouse);
        this->skalieren(std::pow(4.0 / 3.0, (event->delta() / 240.0)));
        this->setTransformationAnchor(QGraphicsView::AnchorViewCenter);
     }
     else
     {
         QGraphicsView::wheelEvent(event);
     }
}
