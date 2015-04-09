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

signals:

public slots:

};

#endif // STRECKEVIEW_H
