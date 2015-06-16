#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>

#include "zusi_file_lib/src/model/strecke.hpp"

using namespace std;

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

    vector<unique_ptr<Strecke> > m_strecken;

    /** Oeffnet eine Streckendatei und fuegt sie zur Liste der offenen Strecken hinzu. */
    void oeffneStrecke(QString dateiname);

    /** Zeigt einen Dialog zum Oeffnen einer Strecke mit passendem Startverzeichnis und liefert den Dateinamen zurueck. */
    QString zeigeStreckeOeffnenDialog();
};

#endif // MAINWINDOW_H
