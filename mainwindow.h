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

private:
    Ui::MainWindow *ui;

    std::vector<std::unique_ptr<Strecke>> m_strecken;

    /** Oeffnet Streckendateien und fuegt sie zur Liste der offenen Strecken hinzu. */
    void oeffneStrecken(const QStringList& dateinamen);

    /** Zeigt die aktuell geladenen Strecken im Strecken-Widget an. */
    void aktualisiereDarstellung();

    /** Zeigt einen Dialog zum Oeffnen einer Strecke mit passendem Startverzeichnis und liefert den Dateinamen zurueck. */
    QStringList zeigeStreckeOeffnenDialog();
};

#endif // MAINWINDOW_H
