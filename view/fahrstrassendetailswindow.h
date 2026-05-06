#ifndef VIEW_FAHRSTRASSENDETAILSWINDOW_H
#define VIEW_FAHRSTRASSENDETAILSWINDOW_H

#include "model/fahrstrassendetails.h"

#include <QImage>
#include <QPointer>
#include <QString>
#include <QWidget>

#include <atomic>
#include <memory>
#include <vector>

class QListWidget;
class QListWidgetItem;
class QScrollArea;
class QHBoxLayout;
class QLabel;
class QThread;

class Streckennetz;
struct ResolvedFahrstrasse;
class FahrstrassenDetailsLs3Worker;

/**
 * Nicht-modales Fenster, das Details zur aktuell in der Fahrstraßen-Seitenleiste
 * ausgewählten Fahrstraße anzeigt.
 *
 * - Links: Liste der Fahrstraßen-Elemente (Start, Ziel, Signale, Weichen, …).
 *   Selektion -> Marker im Hauptfenster; Doppelklick -> View zentriert auf das Element.
 * - Rechts oben: zwei Listen mit Vorgänger-/Nachfolger-Fahrstraßen (jeweils mit
 *   einem Sentinel-Eintrag „(alle)“ bzw. „(keine)“ am Anfang). Auswahl ändert die
 *   Signalvisualisierung (gefilterte Vorsignale bzw. überschriebene Spalten).
 * - Rechts unten: asynchron gerenderte Vorschau der Signal-LS3-Dateien.
 */
class FahrstrassenDetailsWindow : public QWidget
{
    Q_OBJECT
public:
    explicit FahrstrassenDetailsWindow(QWidget* parent = nullptr);
    ~FahrstrassenDetailsWindow() override;

    /**
     * Setzt die anzuzeigende Fahrstraße.
     * `alleFahrstrassen == nullptr` oder `aktiverIndex < 0` leert den Inhalt.
     * Sonst muss `(*alleFahrstrassen)[aktiverIndex]` gültig sein. Der Pointer auf
     * `alleFahrstrassen` muss bis zum nächsten Aufruf gültig bleiben.
     */
    void zeigeFahrstrasse(const Streckennetz* netz,
                          const std::vector<ResolvedFahrstrasse>* alleFahrstrassen,
                          int aktiverIndex);

    /** Liefert die gerade ausgewählten Detail-Eintrag, oder nullptr. */
    const FahrstrasseDetailEintrag* aktuellerEintrag() const;

signals:
    /** Auswahl in der Detail-Liste hat sich geändert (Index in eintraege() oder -1). */
    void detailAusgewaehlt(int index);
    /** Doppelklick auf einen Detail-Eintrag (Index in eintraege() oder -1). */
    void detailDoppelklick(int index);

private slots:
    void onEintragAusgewaehlt();
    void onEintragDoppelklick(QListWidgetItem* item);
    void onVorgaengerNachfolgerAusgewaehlt();
    void onLs3Fertig(int signalIndex, QImage image, QString fehler);

private:
    void aktualisiereEintraege();
    void aktualisiereVorgaengerNachfolger();
    void starteLs3Rendering();
    void stoppeLs3Rendering();

    // Pro Detail-Eintrag berechnete Anzeige-Werte für die Signalvisualisierung.
    // Werden auf Basis der aktuellen Vorgänger-/Nachfolger-Auswahl bestimmt.
    struct EintragAnzeige {
        bool ausgeblendet = false;  // true -> in der Visualisierung nicht zeigen
        int matrixZeile = 0;
        int matrixSpalte = 0;
        bool ersatzsignal = false;
    };
    std::vector<EintragAnzeige> berechneEintragAnzeige() const;

    // Liefert den ausgewählten Index in m_alleFahrstrassen, oder -1 wenn der
    // Sentinel-Eintrag ("(alle)" / "(keine)") gewählt ist bzw. keine Auswahl
    // existiert.
    int aktuelleVorgaengerFsIndex() const;
    int aktuelleNachfolgerFsIndex() const;

    // Quelldaten der aktuell angezeigten Fahrstraße.
    const Streckennetz* m_netz = nullptr;
    const std::vector<ResolvedFahrstrasse>* m_alleFahrstrassen = nullptr;
    int m_aktiverIndex = -1;

    // Eintraege der aktuell angezeigten Fahrstraße.
    std::vector<FahrstrasseDetailEintrag> m_eintraege;

    // UI-Elemente
    QListWidget* m_eintragsListe;
    QListWidget* m_vorgaengerListe;
    QListWidget* m_nachfolgerListe;
    QScrollArea* m_signalScroll;
    QWidget* m_signalContainer;
    QHBoxLayout* m_signalLayout;

    // LS3-Rendering: pro Signal-Eintrag genau ein Pixmap-Slot. Die Signals werden
    // sequenziell in einem Worker-Thread gerendert, weil ls3render globale State hat.
    struct SignalSlot {
        int eintragIndex;     // Index in m_eintraege
        QLabel* bildLabel;
        QLabel* textLabel;
    };
    std::vector<SignalSlot> m_signalSlots;

    // Worker-Thread für das Rendern; lebt solange das Fenster offen ist.
    QPointer<QThread> m_workerThread;
    FahrstrassenDetailsLs3Worker* m_worker = nullptr;

    // Generation-Counter, damit verspätete Worker-Antworten alter Auswahlen
    // verworfen werden können.
    std::atomic<int> m_generation { 0 };
};

#endif // VIEW_FAHRSTRASSENDETAILSWINDOW_H
