#ifndef BAHNSTEIGE_H
#define BAHNSTEIGE_H

#include <unordered_map>
#include <utility>

#include <QtGlobal>

class QGraphicsScene;
struct Strecke;
struct StrElement;

/**
 * Zeichnet die Bahnsteige aller Streckenelemente einer Strecke in die Szene:
 * graue Polygone entlang der Streckenelemente von Bahnsteiganfang- bis
 * Bahnsteigende-Ereignis sowie dunkelgraue Dreiecke für Bahnsteigmitte-Ereignisse,
 * deren Spitze jeweils in Richtung Bahnsteigende zeigt.
 *
 * @p utmOffsetByElement liefert für jedes Streckenelement (auch in fremden Modulen)
 * den (utm_dx, utm_dy)-Versatz in Szenenkoordinaten, sodass Bahnsteige, die sich
 * über Modulgrenzen erstrecken, korrekt platziert werden.
 */
void zeichneBahnsteige(
    QGraphicsScene& scene,
    const Strecke& strecke,
    const std::unordered_map<const StrElement*, std::pair<qreal, qreal>>& utmOffsetByElement);

#endif // BAHNSTEIGE_H
