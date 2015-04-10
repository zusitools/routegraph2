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

protected:
    void wheelEvent(QWheelEvent *event);
    void mousePressEvent(QMouseEvent *event);
    void mouseMoveEvent(QMouseEvent *event);
    void mouseReleaseEvent(QMouseEvent *event);
    void mouseDoubleClickEvent(QMouseEvent *event);

private:
    bool m_rechteMaustasteGedrueckt;
    bool m_linkeMaustasteGedrueckt;
    QPoint m_dragStart;

signals:

public slots:

};

#endif // STRECKEVIEW_H
