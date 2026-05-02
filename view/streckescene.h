#ifndef STRECKESCENE_H
#define STRECKESCENE_H

#include <QGraphicsScene>

#include "model/streckenelement.h"
#include "view/visualisierung/visualisierung.h"

#include "zusi_parser/utils.hpp"
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

    /**
     * Eine Modulgrenze in der Szene: ein Streckenelement-Endpunkt, an dem ein
     * Nachfolger in einem anderen Modul referenziert wird.
     */
    struct Modulgrenze {
        /** Position der Modulgrenze in Szenen-Koordinaten. */
        QPointF szenePunkt;
        /** Pfad des Nachbarmoduls, aufgelöst gegen das aktuelle Modul. */
        zusixml::ZusiPfad nachbarModul;
    };

    /**
     * Liefert alle Modulgrenzen, deren Szenen-Position innerhalb eines
     * Quadrats mit der angegebenen Halbkantenlänge um @p szenePunkt liegt.
     */
    std::vector<Modulgrenze> modulgrenzenInUmgebung(const QPointF& szenePunkt, qreal radius) const;

signals:

public slots:

private:
    UTM m_utmRefPunkt = {};

    // Pro Streckenelement: x/y-Versatz seiner Strecke gegenüber dem Szenen-Referenzpunkt.
    std::unordered_map<const StrElement*, std::pair<qreal, qreal>> m_elementToOffset;

    // Lebenszeit gehört der Szene (über addItem); der Zeiger wird in zeigeFahrstrasse
    // gesetzt und beim verbirgFahrstrasse / Aufräumen wieder zu nullptr.
    QGraphicsPathItem* m_fahrstrasseHighlight = nullptr;

    // Alle Modulgrenzen, die beim Aufbau der Szene aus den geladenen Strecken
    // ermittelt wurden. Wird für das Kontextmenü zum Nachladen benötigt.
    std::vector<Modulgrenze> m_modulgrenzen;
};

#endif // STRECKESCENE_H
