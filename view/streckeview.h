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
    void wheelEvent(QWheelEvent *event);
    void mousePressEvent(QMouseEvent *event);
    void mouseMoveEvent(QMouseEvent *event);
    void mouseReleaseEvent(QMouseEvent *event);
    void mouseDoubleClickEvent(QMouseEvent *event);
    void drawBackground(QPainter *painter, const QRectF &rect);

private:
    bool m_rechteMaustasteGedrueckt;
    bool m_linkeMaustasteGedrueckt;
    QPoint m_dragStart;
    QTransform m_defaultTransform;

signals:

public slots:

};

#endif // STRECKEVIEW_H
