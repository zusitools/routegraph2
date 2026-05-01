#ifndef STRECKESCENE_H
#define STRECKESCENE_H

#include <QGraphicsScene>

#include "model/streckenelement.h"
#include "view/visualisierung/visualisierung.h"

#include "zusi_parser/zusi_types.hpp"

#include <vector>
#include <memory>
#include <unordered_map>

class Streckennetz;
class QGraphicsPathItem;

class StreckeScene : public QGraphicsScene
{
    Q_OBJECT
public:
    explicit StreckeScene(const Streckennetz& streckennetz,
                          Visualisierung& visualisierung, bool zeigeBetriebsstellen,
                          bool zeigeBahnsteige, QObject *parent = nullptr);

    /**
     * Zeichnet einen Pfad (Folge von Streckenelementen mit Richtung) als
     * farbliche Hervorhebung über die normale Gleisdarstellung. Eine ggf.
     * vorhandene vorherige Hervorhebung wird vorher entfernt.
     */
    void zeigeFahrstrasse(const std::vector<StreckenelementUndRichtung>& pfad);

    /** Entfernt eine ggf. vorhandene Fahrstraßen-Hervorhebung. */
    void verbirgFahrstrasse();

    /**
     * Liefert die Bounding-Box eines Pfades in Szenen-Koordinaten,
     * also unter Berücksichtigung des UTM-Versatzes der jeweiligen Module.
     * Liefert ein leeres QRectF, wenn der Pfad leer ist oder keine Elemente
     * der Szene zugeordnet werden können.
     */
    QRectF boundingRectFuerPfad(const std::vector<StreckenelementUndRichtung>& pfad) const;

signals:

public slots:

private:
    UTM m_utmRefPunkt = {};

    // Pro Streckenelement: x/y-Versatz seiner Strecke gegenüber dem Szenen-Referenzpunkt.
    std::unordered_map<const StrElement*, std::pair<qreal, qreal>> m_elementToOffset;

    // Lebenszeit gehört der Szene (über addItem); der Zeiger wird in zeigeFahrstrasse
    // gesetzt und beim verbirgFahrstrasse / Aufräumen wieder zu nullptr.
    QGraphicsPathItem* m_fahrstrasseHighlight = nullptr;
};

#endif // STRECKESCENE_H
