#ifndef MODEL_FAHRSTRASSENDETAILS_H
#define MODEL_FAHRSTRASSENDETAILS_H

#include "model/fahrstrasse.h"
#include "model/streckenelement.h"

#include "zusi_parser/utils.hpp"

#include <string>
#include <vector>

class Streckennetz;
struct Signal;

/**
 * Ein einzelnes Element einer Fahrstraße, wie es im Detailfenster angezeigt wird.
 * Liste umfasst Start, Ziel, sowie alle Fahrstr<*>-Kindelemente.
 */
struct FahrstrasseDetailEintrag {
    enum class Typ {
        Start,
        Ziel,
        FahrstrSignal,
        FahrstrVSignal,
        FahrstrWeiche,
        FahrstrRegister,
        FahrstrAufloesung,
        FahrstrTeilaufloesung,
        FahrstrSigHaltfall,
        // An ein Haupt-/Vorsignal gekoppeltes Signal (<KoppelSignal>-Eintrag).
        // Wird auf dieselbe Matrix-Zeile/Spalte wie das gekoppelte Signal gestellt.
        Koppelsignal,
    };

    Typ typ;
    std::string label;                       // Anzeigetext (z. B. "Signal: BS A")
    std::string beschreibung;                // Tooltip / zusätzliche Infos (Element-Nr, Richtung, …)
    StreckenelementUndRichtung elementUndRichtung;  // ggf. ungültig bei Auflösefehler
    std::string fehler;                      // leer = OK; sonst Tooltip mit Grund

    // Nur für FahrstrSignal/FahrstrVSignal/Koppelsignal gefüllt: das Signal-Objekt
    // selbst und der Modulpfad, in dem es liegt (für die LS3-Pfad-Auflösung gegen
    // das Strecken-Modul).
    const Signal* signal = nullptr;
    zusixml::ZusiPfad signalModul = zusixml::ZusiPfad::vonZusiPfad("");

    // Indizes in die Signalmatrix (Zeile=Hauptsignal-Begriff, Spalte=Vorsignal-Begriff).
    // Für FahrstrSignal: Zeile=FahrstrSignalZeile, Spalte=0.
    // Für FahrstrVSignal: Zeile=0, Spalte=FahrstrSignalSpalte.
    // Für Koppelsignal: vom verkoppelten Wurzelsignal geerbt.
    int matrixZeile = 0;
    int matrixSpalte = 0;
    // Nur für FahrstrSignal/Koppelsignal eines Hauptsignals: bei true wird stattdessen
    // der Eintrag der Ersatzsignal-Matrix mit Index `matrixZeile` (ohne Spalte) verwendet.
    bool ersatzsignal = false;

    // Nur für Typ::Koppelsignal:
    //   kopplungsTiefe: 1 für direkt gekoppelte Signale, 2 für deren Koppelsignale, ...
    //   kopplungZuVsig: true, wenn die Kopplungs-Wurzel ein FahrstrVSignal ist, sonst
    //                   FahrstrSignal. Bestimmt die Gruppierung in der Visualisierung.
    int kopplungsTiefe = 0;
    bool kopplungZuVsig = false;
};

/**
 * Erzeugt aus einer ResolvedFahrstrasse eine flache Liste aller Detail-Einträge
 * (Start, Ziel, Signale, Vorsignale, Weichen, Register, Auflöse-, Teilauflöse-,
 * Signalhaltfall-Punkte). Reihenfolge: gemäß der Plan-Vorgabe pro Gruppe.
 */
std::vector<FahrstrasseDetailEintrag> ermittleFahrstrasseDetails(
    const Streckennetz& netz,
    const ResolvedFahrstrasse& fs);

#endif // MODEL_FAHRSTRASSENDETAILS_H
