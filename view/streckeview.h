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
    bool m_rechteMaustasteGedrueckt;
    bool m_linkeMaustasteGedrueckt;
    QPoint m_dragStart;
    QTransform m_defaultTransform;

signals:

public slots:

};

#endif // STRECKEVIEW_H
