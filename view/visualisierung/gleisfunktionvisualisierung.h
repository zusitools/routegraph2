#ifndef GLEISFUNKTIONVISUALISIERUNG_H
#define GLEISFUNKTIONVISUALISIERUNG_H

#include <QGraphicsScene>

#include "view/graphicsitems/streckensegmentitem.h"
#include "view/visualisierung/visualisierung.h"

class GleisfunktionVisualisierung : public Visualisierung
{
public:
    virtual void setzeDarstellung(StreckensegmentItem& item) override;
    virtual std::unique_ptr<QGraphicsScene> legende() override { return nullptr; }
};

#endif // GLEISFUNKTIONVISUALISIERUNG_H
