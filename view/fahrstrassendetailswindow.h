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
 * - Rechts oben: zwei (vorerst leere) Listen für Vorgänger-/Nachfolger-Fahrstraßen.
 * - Rechts unten: asynchron gerenderte Vorschau der Signal-LS3-Dateien.
 */
class FahrstrassenDetailsWindow : public QWidget
{
    Q_OBJECT
public:
    explicit FahrstrassenDetailsWindow(QWidget* parent = nullptr);
    ~FahrstrassenDetailsWindow() override;

    /** Setzt die anzuzeigende Fahrstraße. nullptr -> leerer Inhalt. */
    void zeigeFahrstrasse(const Streckennetz* netz, const ResolvedFahrstrasse* fs);

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
    void onLs3Fertig(int signalIndex, QImage image, QString fehler);

private:
    void aktualisiereEintraege();
    void starteLs3Rendering();
    void stoppeLs3Rendering();

    // Eintraege der aktuell angezeigten Fahrstraße.
    std::vector<FahrstrasseDetailEintrag> m_eintraege;

    // UI-Elemente
    QListWidget* m_eintragsListe;
    QListWidget* m_vorgaengerListe;
    QListWidget* m_nachfolgerListe;
    QScrollArea* m_signalScroll;
    QWidget* m_signalContainer;
    QHBoxLayout* m_signalLayout;
    QLabel* m_titel;

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
