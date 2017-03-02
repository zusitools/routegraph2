#ifndef STRECKESCENE_H
#define STRECKESCENE_H

#include <QGraphicsScene>

#include "view/visualisierung.h"

#include "zusi_file_lib/src/model/strecke.hpp"
#include "zusi_file_lib/src/model/utmpunkt.hpp"

using namespace std;

class StreckeScene : public QGraphicsScene
{
    Q_OBJECT
public:
    explicit StreckeScene(const vector<unique_ptr<Strecke>>& strecken,
                          const Visualisierung& visualisierung, QObject *parent = 0);

signals:

public slots:

private:
    // UTM-Referenzpunkt
    UTMPunkt m_utmRefPunkt;

};

#endif // STRECKESCENE_H
