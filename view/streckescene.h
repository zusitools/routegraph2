#ifndef STRECKESCENE_H
#define STRECKESCENE_H

#include <QGraphicsScene>

#include "view/visualisierung/visualisierung.h"

#include "zusi_parser/zusi_types.hpp"

#include <vector>
#include <memory>

class Streckennetz;

class StreckeScene : public QGraphicsScene
{
    Q_OBJECT
public:
    explicit StreckeScene(const Streckennetz& streckennetz,
                          Visualisierung& visualisierung, bool zeigeBetriebsstellen, QObject *parent = nullptr);

signals:

public slots:

private:
    // UTM-Referenzpunkt
    UTM m_utmRefPunkt = {};

};

#endif // STRECKESCENE_H
