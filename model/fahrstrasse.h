#ifndef MODEL_FAHRSTRASSE_H
#define MODEL_FAHRSTRASSE_H

#include "model/streckenelement.h"

#include <string>
#include <vector>

class Streckennetz;

/**
 * Eine aus den Streckendaten aufgelöste Fahrstraße samt vorausberechnetem Pfad.
 * Bei einem nicht-leeren `fehler` ist `pfad` leer; der Fehlertext kann als Tooltip
 * angezeigt werden, der Eintrag ist nicht klickbar.
 */
struct ResolvedFahrstrasse {
    std::string name;            // FahrstrName
    std::string typ;             // FahrstrTyp ("TypZug", ...)
    std::string betriebsstelle;  // Gruppierungsschlüssel (NameBetriebsstelle Start- oder Zielsignal)

    StreckenelementUndRichtung start;  // ggf. nullptr-StrElement bei Auflösefehler
    StreckenelementUndRichtung ziel;   // ggf. nullptr-StrElement bei Auflösefehler

    // Sequenz aller Streckenelemente (mit Richtung) zwischen Start und Ziel,
    // einschließlich Start- und Zielelement. Leer falls fehler nicht leer.
    std::vector<StreckenelementUndRichtung> pfad;

    // Leerer String = aufgelöst und Pfad berechnet; sonst Tooltip-Text mit Grund.
    std::string fehler;
};

/**
 * Liest alle Fahrstraßen aus dem geladenen Streckennetz, löst ihre Referenzen auf
 * und berechnet zu jeder Fahrstraße den vollständigen Pfad. Gibt eine sortierte
 * Liste zurück (nach Betriebsstelle, dann Name).
 */
std::vector<ResolvedFahrstrasse> ermittleFahrstrassen(const Streckennetz& netz);

#endif // MODEL_FAHRSTRASSE_H
