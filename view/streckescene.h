#ifndef STRECKESCENE_H
#define STRECKESCENE_H

#include <QGraphicsScene>

#include "zusi_file_lib/src/model/strecke.hpp"

using namespace std;

class StreckeScene : public QGraphicsScene
{
    Q_OBJECT
public:
    explicit StreckeScene(unique_ptr<Strecke> &strecke, QObject *parent = 0);

signals:

public slots:

};

#endif // STRECKESCENE_H
