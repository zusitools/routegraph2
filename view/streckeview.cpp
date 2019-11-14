#include "streckeview.h"

#include <cmath>
#include <memory>

#include <QApplication>
#include <QDesktopWidget>
#include <QVarLengthArray>
#include <QScrollBar>

StreckeView::StreckeView(QWidget *parent) : QGraphicsView(parent)
{
    this->setCursor(Qt::OpenHandCursor);
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
     if (event->modifiers() == Qt::NoModifier)
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
    this->setCursor(Qt::ClosedHandCursor);
    if (event->buttons().testFlag(Qt::RightButton))
    {
        this->m_rechteMaustasteGedrueckt = true;
        this->m_linkeMaustasteGedrueckt = false;
        this->m_dragStart = event->globalPos();
    }
    if (event->buttons().testFlag(Qt::LeftButton))
    {
        this->m_rechteMaustasteGedrueckt = false;
        this->m_linkeMaustasteGedrueckt = true;
        this->m_dragStart = event->globalPos();
    }

    QGraphicsView::mousePressEvent(event);
}

void StreckeView::mouseMoveEvent(QMouseEvent *event)
{
    if (this->m_rechteMaustasteGedrueckt)
    {
        QPoint dxy = event->pos() - this->m_dragStart;
        this->m_dragStart = event->pos();
        this->rotate(dxy.y());
    }

    QGraphicsView::mouseMoveEvent(event);

    if (this->m_linkeMaustasteGedrueckt)
    {
        // Unendliches Scrollen. Die Cursorposition wird an die gegenüberliegende Kante gesetzt,
        // wenn der Cursor den Rand des Bildschirms erreicht hat.
        // Adapted from Okular source code (ui/pageview.cpp).
        QPoint mousePos = event->globalPos();
        QPoint delta = this->m_dragStart - mousePos;

        const QRect mouseContainer = QApplication::desktop()->screenGeometry(this);
        // If the delta is huge it probably means we just wrapped in that direction
        const QPoint absDelta(abs(delta.x()), abs(delta.y()));
        if (absDelta.y() > mouseContainer.height() / 2)
        {
            delta.setY(mouseContainer.height() - absDelta.y());
        }
        if (absDelta.x() > mouseContainer.width() / 2)
        {
            delta.setX(mouseContainer.width() - absDelta.x());
        }

        // TODO: If we wrap both left/right and top/bottom, do not call QCursor::setPos() twice
        // wrap mouse from top to bottom
        if (mousePos.y() <= mouseContainer.top() + 4 &&
             verticalScrollBar()->value() < verticalScrollBar()->maximum() - 10)
        {
            mousePos.setY(mouseContainer.bottom() - 5);
            QCursor::setPos(mousePos);
        }
        // wrap mouse from bottom to top
        else if (mousePos.y() >= mouseContainer.bottom() - 4 &&
                  verticalScrollBar()->value() > 10)
        {
            mousePos.setY(mouseContainer.top() + 5);
            QCursor::setPos(mousePos);
        }
        // wrap mouse from left to right
        if (mousePos.x() <= mouseContainer.left() + 4 &&
             horizontalScrollBar()->value() < horizontalScrollBar()->maximum() - 10)
        {
            mousePos.setX(mouseContainer.right() - 5);
            QCursor::setPos(mousePos);
        }
        // wrap mouse from right to left
        else if (mousePos.x() >= mouseContainer.right() - 4 &&
                  horizontalScrollBar()->value() > 10)
        {
            mousePos.setX(mouseContainer.left() + 5);
            QCursor::setPos(mousePos);
        }

        // remember last position
        this->m_dragStart = mousePos;

        // scroll page by position increment
        // TODO: Does this trigger two draw events?
        horizontalScrollBar()->setValue(horizontalScrollBar()->value() + delta.x());
        verticalScrollBar()->setValue(verticalScrollBar()->value() + delta.y());
    }
}

void StreckeView::mouseReleaseEvent(QMouseEvent *event)
{
    this->m_rechteMaustasteGedrueckt = false;
    this->m_linkeMaustasteGedrueckt = false;
    this->setCursor(Qt::OpenHandCursor);
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
    constexpr int gridSize = 1000;

    qreal left = int(rect.left()) - (int(rect.left()) % gridSize);
    qreal top = int(rect.top()) - (int(rect.top()) % gridSize);

    QVarLengthArray<QPointF> points;

    for (int i = 0; left + i * gridSize < rect.right(); ++i)
    {
        const qreal x = left + i * gridSize;
        points.append(QPointF(x, rect.top()));
        points.append(QPointF(x, rect.bottom()));
    }
    for (int i = 0; top + i * gridSize < rect.bottom(); ++i)
    {
        const qreal y = top + i * gridSize;
        points.append(QPointF(rect.left(), y));
        points.append(QPointF(rect.right(), y));
    }

    auto antiAliasingAn = painter->testRenderHint(QPainter::Antialiasing);
    painter->setRenderHint(QPainter::Antialiasing, false);
    auto pen = QPen(QColor(220, 220, 220));
    pen.setWidth(0);
    painter->setPen(pen);
    painter->drawLines(points.data(), points.size() / 2);
    painter->setRenderHint(QPainter::Antialiasing, antiAliasingAn);
}
