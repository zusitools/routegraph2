#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include "streckennetz.h"
#include "model/fahrstrasse.h"

#include <memory>
#include <optional>
#include <vector>

#include <QMainWindow>
#include <QStringList>

namespace Ui {
class MainWindow;
}

class Strecke;
class StreckeScene;
class FahrstrassenDetailsWindow;

class QDir;
class QGraphicsScene;

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();

public slots:
    void actionOeffnenTriggered();
    void actionModulOeffnenTriggered();
    void actionOrdnerOeffnenTriggered();
    void actionOrdnerAnfuegenTriggered();
    void actionVisualisierungTriggered();
    void actionAnsichtBetriebsstellennamenToggled(bool checked);
    void actionAnsichtBahnsteigeToggled(bool checked);
    void actionAnsichtFahrstrassenToggled(bool checked);
    void actionAnsichtFahrstrassenDetailsTriggered();

private slots:
    void onFahrstrasseAusgewaehlt(int index);
    void onFahrstrasseDoppelklick(int index);
    void onStreckeViewKontextmenuAngefordert(QPoint viewPos);
    void onFahrstrassenDetailAusgewaehlt(int index);
    void onFahrstrassenDetailDoppelklick(int index);

protected:
    virtual void dragEnterEvent(QDragEnterEvent *e) override;
    virtual void dragMoveEvent(QDragMoveEvent *e) override;
    virtual void dropEvent(QDropEvent *e) override;protected:
    bool eventFilter(QObject *obj, QEvent *event);

private:
    Ui::MainWindow *ui;

    Streckennetz m_streckennetz;
    std::unique_ptr<StreckeScene> m_streckeScene;
    std::unique_ptr<QGraphicsScene> m_legendeScene;

    bool m_zeigeBetriebsstellen{true};
    bool m_zeigeBahnsteige{false};

    // Fahrstraßen-Liste muss nach Modul-Ladevorgängen ggf. neu aufgebaut werden;
    // bei unsichtbarer Seitenleiste wird das auf den nächsten Aktivierungszeitpunkt
    // verschoben.
    bool m_fahrstrassenListeAktuell{false};
    std::optional<int> m_aktiveFahrstrasse;

    // Nicht-modales Detail-Fenster zur aktuell ausgewählten Fahrstraße.
    // Lebt solange MainWindow lebt; wird über Ansicht->Fahrstraßen-Details ein-/ausgeblendet.
    FahrstrassenDetailsWindow* m_fahrstrassenDetailsWindow = nullptr;

    void setzeAnsichtZurueck();
    void aktualisiereFahrstrassenListe();
    void wendeFahrstrassenHervorhebungAn();
    /** Aktualisiert den Inhalt des Fahrstraßen-Detailfensters anhand m_aktiveFahrstrasse. */
    void aktualisiereFahrstrassenDetailsFenster();
    /** Markiert die Liste als veraltet und aktualisiert sie sofort, falls sichtbar. */
    void fahrstrassenListeUngueltig();

    /** Oeffnet Streckendateien und fuegt sie zur Liste der offenen Strecken hinzu. */
    void oeffneStrecken(const QStringList& dateinamen);

    /** Zeigt die aktuell geladenen Strecken im Strecken-Widget an. */
    void aktualisiereDarstellung();

    /** Zeigt einen Dialog zum Oeffnen einer Strecke mit passendem Startverzeichnis und liefert den Dateinamen zurueck. */
    QStringList zeigeStreckeOeffnenDialog();

    /** Zeigt einen Dialog zum Oeffnen eines Streckenordners und liefert eine Liste von ST3-Dateien in diesem Verzeichnis zurueck. */
    QStringList zeigeOrdnerOeffnenDialog();

    QStringList findeSt3Rekursiv(QDir& dir, const QStringList& filter);
};

#endif // MAINWINDOW_H
