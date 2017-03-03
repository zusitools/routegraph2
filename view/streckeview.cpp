#include "streckeview.h"

#include <cmath>

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
        QPoint oldDragStart = this->m_dragStart;

        // Unendliches Scrollen. Die Cursorposition wird an die gegenüberliegende Kante gesetzt,
        // wenn der Cursor den Rand des Widgets erreicht hat. (Nutze 1-Pixel-Rand, da das Fenster eventuell
        // im Vollbildmodus ist und der Cursor nicht über die Grenzen des Widgets hinaus bewegt werden kann.)
        this->m_dragStart = QPoint(
          event->pos().x() <= 0 ? this->width() - 2 :
            event->pos().x() >= this->width() - 1 ? 1 : this->m_dragStart.x(),
          event->pos().y() <= 0 ? this->height() - 2 :
            event->pos().y() >= this->height() - 1 ? 1 : this->m_dragStart.y());

        if (this->m_dragStart != oldDragStart)
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
    auto pen = QPen(QColor(220, 220, 220));
    pen.setWidth(0);
    painter->setPen(pen);
    painter->drawLines(lines.data(), lines.size());
    painter->setRenderHint(QPainter::Antialiasing, antiAliasingAn);
}
