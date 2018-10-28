#ifndef STRECKESCENE_H
#define STRECKESCENE_H

#include <QGraphicsScene>

#include "view/visualisierung/visualisierung.h"

#include "zusi_parser/zusi_types.hpp"

#include <vector>
#include <memory>

class StreckeScene : public QGraphicsScene
{
    Q_OBJECT
public:
    explicit StreckeScene(const std::vector<std::unique_ptr<Strecke>>& strecken,
                          Visualisierung& visualisierung, QObject *parent = nullptr);

signals:

public slots:

private:
    // UTM-Referenzpunkt
    UTM m_utmRefPunkt = {};

};

#endif // STRECKESCENE_H
