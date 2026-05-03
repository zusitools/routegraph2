#ifndef MODEL_FAHRSTRASSE_H
#define MODEL_FAHRSTRASSE_H

#include "model/streckenelement.h"

#include "zusi_parser/utils.hpp"

#include <string>
#include <vector>

class Streckennetz;
struct Fahrstrasse;

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

    // Zeiger auf das ursprüngliche Fahrstraßen-Element im Streckennetz und
    // den Pfad des Moduls, in dem es definiert ist. Wird zum Auflösen weiterer
    // Referenzen (FahrstrSignal, FahrstrRegister, …) im Detailfenster benötigt.
    // Lebensdauer: gültig solange Streckennetz die Strecke hält.
    const Fahrstrasse* quelle = nullptr;
    zusixml::ZusiPfad fahrstrasseModul = zusixml::ZusiPfad::vonZusiPfad("");
};

/**
 * Liest alle Fahrstraßen aus dem geladenen Streckennetz, löst ihre Referenzen auf
 * und berechnet zu jeder Fahrstraße den vollständigen Pfad. Gibt eine sortierte
 * Liste zurück (nach Betriebsstelle, dann Name).
 */
std::vector<ResolvedFahrstrasse> ermittleFahrstrassen(const Streckennetz& netz);

#endif // MODEL_FAHRSTRASSE_H
