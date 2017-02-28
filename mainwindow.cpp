#include "mainwindow.h"
#include "ui_mainwindow.h"

#include <QFileDialog>
#include <QTime>
#include <QDebug>

#include "view/streckescene.h"

#include "zusi_file_lib/src/common/pfade.hpp"
#include "zusi_file_lib/src/io/fpn_leser.hpp"
#include "zusi_file_lib/src/io/st3_leser.hpp"
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

    if (!dateiname.isNull())
    {
        this->m_strecken.clear();
        this->oeffneStrecke(dateiname);
    }
}

void MainWindow::actionModulOeffnenTriggered()
{
    QString dateiname = this->zeigeStreckeOeffnenDialog();

    if (!dateiname.isNull())
    {
        this->oeffneStrecke(dateiname);
    }
}

void MainWindow::oeffneStrecke(QString dateiname)
{
    QTime timer;
    timer.start();

    if (dateiname.endsWith("st3", Qt::CaseInsensitive))
    {
        St3Leser st3Leser;
        this->m_strecken.push_back(st3Leser.liesDateiMitDateiname(dateiname.toStdString()));
    }
    else if (dateiname.endsWith("fpn", Qt::CaseInsensitive))
    {
        St3Leser st3Leser;
        auto fahrplan = FpnLeser().liesDateiMitDateiname(dateiname.toStdString());
        for (auto& modul : fahrplan->streckenmodule) {
            this->m_strecken.push_back(st3Leser.liesDateiMitDateiname(zusi_file_lib::pfade::zusiPfadZuOsPfad(modul)));
        }
    }
    else
    {
        StrLeser strLeser;
        this->m_strecken.push_back(strLeser.liesStrDateiMitDateiname(dateiname.toStdString()));
    }

    qDebug() << timer.elapsed() << "ms zum Lesen der Strecken";
    timer.restart();

    vector<reference_wrapper<unique_ptr<Strecke> > > strecken;
    for (auto &strecke : this->m_strecken)
    {
        strecken.push_back(reference_wrapper<unique_ptr<Strecke> >(strecke));
    }
    ui->streckeView->setScene(new StreckeScene(strecken));
    qDebug() << timer.elapsed() << "ms zum Erstellen der Segmente";
    ui->streckeView->resetTransform();

    // Zusi 2&3
    ui->streckeView->rotate(-90);
    ui->streckeView->scale(1.0f, -1.0f);
    ui->streckeView->fitInView(ui->streckeView->sceneRect(), Qt::KeepAspectRatio);
    ui->streckeView->centerOn(ui->streckeView->sceneRect().center());

    // Zusi 3
    if (this->m_strecken[0]->formatVersion.size() == 0 || this->m_strecken[0]->formatVersion[0] != '2')
    {
        ui->streckeView->rotate(-90);
    }

    ui->streckeView->setDefaultTransform(ui->streckeView->transform());
}

QString MainWindow::zeigeStreckeOeffnenDialog()
{
    QString startverzeichnis = "";
    return QFileDialog::getOpenFileName(this, tr("Strecke öffnen"), startverzeichnis, QString(tr("Streckendateien (*.str *.STR *.st3 *.ST3 *.fpn *.FPN);;Alle Dateien(*.*)")));
}
