#include "streckeview.h"

#include <cmath>
#include <memory>

#include <QTimer>
#include <QVarLengthArray>

StreckeView::StreckeView(QWidget *parent) : QGraphicsView(parent)
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
        // wenn der Cursor den Rand des Widgets erreicht hat. (Nutze 1-Pixel-Rand, da das Fenster eventuell
        // im Vollbildmodus ist und der Cursor nicht über die Grenzen des Widgets hinaus bewegt werden kann.)
        if (!this->m_resetDragPos && ((event->pos().x() <= 0) || (event->pos().y() <= 0) || (event->pos().x() >= this->width() - 1) || (event->pos().y() >= this->height() - 1)))
        {
            this->m_resetDragPos = true;

            // Sende Event ueber QTimer, damit es nach allen anderen Events, die noch in der Queue stecken, abgearbeitet wird.
            QTimer::singleShot(0, [this, button = event->button(), buttons = event->buttons(), modifiers = event->modifiers()] {
                this->m_resetDragPos = false;
                if (!this->m_linkeMaustasteGedrueckt) {
                    return;
                }

                const auto pos = this->mapFromGlobal(QCursor::pos());
                QMouseEvent releaseEvent(QEvent::MouseButtonRelease, pos, button, buttons, modifiers);
                this->mouseReleaseEvent(&releaseEvent);

                this->m_dragStart = QPoint(
                  pos.x() <= 0 ? this->width() - 2 :
                    pos.x() >= this->width() - 1 ? 1 : pos.x(),
                  pos.y() <= 0 ? this->height() - 2 :
                    pos.y() >= this->height() - 1 ? 1 : pos.y());
                QCursor::setPos(this->mapToGlobal(this->m_dragStart));

                // QCursor::pos() hier nochmals abfragen, falls das System
                // das Setzen der Cursorposition nicht unterstuetzt und der Cursor
                // deshalb nicht verschoben wurde.
                QMouseEvent pressEvent(QEvent::MouseButtonPress, this->mapFromGlobal(QCursor::pos()), button, buttons, modifiers);
                this->mousePressEvent(&pressEvent);
            });
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

    QVarLengthArray<QPointF> points;

    for (qreal x = left; x < rect.right(); x += gridSize)
    {
        points.append(QPointF(x, rect.top()));
        points.append(QPointF(x, rect.bottom()));
    }
    for (qreal y = top; y < rect.bottom(); y += gridSize)
    {
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
