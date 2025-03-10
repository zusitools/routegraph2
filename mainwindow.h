#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include "streckennetz.h"

#include <memory>
#include <vector>

#include <QMainWindow>
#include <QStringList>

namespace Ui {
class MainWindow;
}

class Strecke;
class StreckeScene;

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

    void setzeAnsichtZurueck();

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
