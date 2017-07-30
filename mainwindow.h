#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <memory>
#include <vector>

#include <QMainWindow>

#include "zusi_file_lib/src/model/strecke.hpp"

namespace Ui {
class MainWindow;
}

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

protected:
    virtual void dragEnterEvent(QDragEnterEvent *e) override;
    virtual void dragMoveEvent(QDragMoveEvent *e) override;
    virtual void dropEvent(QDropEvent *e) override;protected:
    bool eventFilter(QObject *obj, QEvent *event);

private:
    Ui::MainWindow *ui;

    std::vector<std::unique_ptr<Strecke>> m_strecken;

    void setzeAnsichtZurueck();

    /** Oeffnet Streckendateien und fuegt sie zur Liste der offenen Strecken hinzu. */
    void oeffneStrecken(const QStringList& dateinamen);

    /** Zeigt die aktuell geladenen Strecken im Strecken-Widget an. */
    void aktualisiereDarstellung();

    /** Zeigt einen Dialog zum Oeffnen einer Strecke mit passendem Startverzeichnis und liefert den Dateinamen zurueck. */
    QStringList zeigeStreckeOeffnenDialog();

    /** Zeigt einen Dialog zum Oeffnen eines Streckenordners und liefert eine Liste von ST3-Dateien in diesem Verzeichnis zurueck. */
    QStringList zeigeOrdnerOeffnenDialog();
};

#endif // MAINWINDOW_H
