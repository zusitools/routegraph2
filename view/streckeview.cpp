#include "streckeview.h"

#include <QVarLengthArray>

StreckeView::StreckeView(QWidget *parent) :
    QGraphicsView(parent), m_rechteMaustasteGedrueckt(false), m_linkeMaustasteGedrueckt(false), m_dragStart(QPoint())
{
    this->setDragMode(QGraphicsView::ScrollHandDrag);
    this->setTransformationAnchor(QGraphicsView::AnchorUnderMouse);
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

void StreckeView::setDefaultTransform(const QTransform &matrix)
{
    this->m_defaultTransform = matrix;
}

void StreckeView::wheelEvent(QWheelEvent *event)
{
     if (event->modifiers() == Qt::ControlModifier)
     {
        this->skalieren(std::pow(4.0 / 3.0, (event->delta() / 240.0)));
     }
     else
     {
         QGraphicsView::wheelEvent(event);
     }
}

void StreckeView::mousePressEvent(QMouseEvent *event)
{
    if (event->buttons().testFlag(Qt::RightButton))
    {
        this->m_rechteMaustasteGedrueckt = true;
        this->m_linkeMaustasteGedrueckt = false;
        this->m_dragStart = event->pos();
    }
    if (event->buttons().testFlag(Qt::LeftButton))
    {
        this->m_rechteMaustasteGedrueckt = false;
        this->m_linkeMaustasteGedrueckt = true;
        this->m_dragStart = event->pos();
    }

    QGraphicsView::mousePressEvent(event);
}

void StreckeView::mouseMoveEvent(QMouseEvent *event)
{
    QPoint dxy = event->pos() - this->m_dragStart;
    this->m_dragStart = event->pos();

    if (this->m_rechteMaustasteGedrueckt)
    {
        this->rotate(dxy.y());
    }

    QGraphicsView::mouseMoveEvent(event);

    if (this->m_linkeMaustasteGedrueckt)
    {
        bool cursorNeuSetzen = true;

        // Unendliches Scrollen
        if (event->pos().x() < 0)
        {
            this->m_dragStart = QPoint(this->width(), event->pos().y());
        }
        else if (event->pos().y() < 0)
        {
            this->m_dragStart = QPoint(event->pos().x(), this->height());
        }
        else if (event->pos().x() >= this->width())
        {
            this->m_dragStart = QPoint(1, event->pos().y());
        }
        else if (event->pos().y() >= this->height())
        {
            this->m_dragStart = QPoint(event->pos().x(), 1);
        }
        else
        {
            cursorNeuSetzen = false;
        }

        if (cursorNeuSetzen)
        {
            this->mouseReleaseEvent(event);

            QCursor::setPos(mapToGlobal(this->m_dragStart));
            QMouseEvent newEvent(event->type(), this->m_dragStart, event->button(), event->buttons(), event->modifiers());
            this->mousePressEvent(&newEvent);
        }
    }
}

void StreckeView::mouseReleaseEvent(QMouseEvent *event)
{
    this->m_rechteMaustasteGedrueckt = false;
    this->m_linkeMaustasteGedrueckt = false;
    QGraphicsView::mouseReleaseEvent(event);
}

void StreckeView::mouseDoubleClickEvent(QMouseEvent *event)
{
    if (event->button() == Qt::RightButton) {
        this->setTransform(this->m_defaultTransform);
        this->fitInView(this->sceneRect(), Qt::KeepAspectRatio);
    }
}

void StreckeView::drawBackground(QPainter *painter, const QRectF &rect)
{
    const int gridSize = 1000;

    qreal left = int(rect.left()) - (int(rect.left()) % gridSize);
    qreal top = int(rect.top()) - (int(rect.top()) % gridSize);

    QVarLengthArray<QLineF, 100> lines;

    for (qreal x = left; x < rect.right(); x += gridSize)
    {
        lines.append(QLineF(x, rect.top(), x, rect.bottom()));
    }
    for (qreal y = top; y < rect.bottom(); y += gridSize)
    {
        lines.append(QLineF(rect.left(), y, rect.right(), y));
    }

    auto antiAliasingAn = painter->testRenderHint(QPainter::Antialiasing);
    painter->setRenderHint(QPainter::Antialiasing, false);
    painter->setPen(QColor(220, 220, 220));
    painter->drawLines(lines.data(), lines.size());
    painter->setRenderHint(QPainter::Antialiasing, antiAliasingAn);
    }
}
