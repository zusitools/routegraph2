#ifndef STRECKEVIEW_H
#define STRECKEVIEW_H

#include <QGraphicsView>
#include <QWheelEvent>

class StreckeView : public QGraphicsView
{
    Q_OBJECT
public:
    explicit StreckeView(QWidget *parent = 0);

    void skalieren(double faktor);
    void vergroessern();
    void verkleinern();

    void setDefaultTransform(const QTransform& matrix);

protected:
    void wheelEvent(QWheelEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void mouseDoubleClickEvent(QMouseEvent *event) override;
    void drawBackground(QPainter *painter, const QRectF &rect) override;

private:
    bool m_rechteMaustasteGedrueckt { false };
    bool m_linkeMaustasteGedrueckt { false };
    bool m_resetDragPos { false };
    bool m_rechteMausHatGedraggt { false };
    QPoint m_rechtePressViewPos { };
    QPointF m_dragStart { };
    QTransform m_defaultTransform { };

signals:
    /**
     * Wird beim Loslassen der rechten Maustaste emittiert, sofern der Cursor
     * sich zwischen Press und Release nicht weiter als die Drag-Schwelle
     * bewegt hat (also kein Rotations-Drag stattgefunden hat).
     * @p viewPos ist die Klickposition in Widget-Koordinaten.
     */
    void kontextmenuAngefordert(QPoint viewPos);

public slots:

};

#endif // STRECKEVIEW_H
