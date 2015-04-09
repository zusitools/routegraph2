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

private:
    Ui::MainWindow *ui;

    unique_ptr<Strecke> m_strecke;

    /** Zeigt einen Dialog zum Oeffnen einer Strecke mit passendem Startverzeichnis und liefert den Dateinamen zurueck. */
    QString zeigeStreckeOeffnenDialog();
};

#endif // MAINWINDOW_H
