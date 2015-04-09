#include "mainwindow.h"
#include "ui_mainwindow.h"

#include <QFileDialog>

#include "view/streckescene.h"

#include "zusi_file_lib/src/io/str_leser.hpp"

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::actionOeffnenTriggered()
{
    QString dateiname = this->zeigeStreckeOeffnenDialog();

    if (dateiname.isNull())
    {
        return;
    }

    StrLeser strLeser;
    this->m_strecke = strLeser.liesStrDateiMitDateiname(dateiname.toStdString());
    ui->streckeView->setScene(new StreckeScene(this->m_strecke));
    ui->streckeView->fitInView(ui->streckeView->sceneRect(), Qt::KeepAspectRatio);
}

QString MainWindow::zeigeStreckeOeffnenDialog()
{
    QString startverzeichnis = "";
    return QFileDialog::getOpenFileName(this, tr("Strecke öffnen"), startverzeichnis, QString(tr("Streckendateien (*.str *.STR *.st3 *.ST3 *.fpn *.FPN);;Alle Dateien(*.*)")));
}
