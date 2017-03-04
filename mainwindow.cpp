#include "mainwindow.h"
#include "ui_mainwindow.h"

#include <future>
#include <thread>

#include <QActionGroup>
#include <QDir>
#include <QFileDialog>
#include <QTime>
#include <QDebug>

#include "view/streckescene.h"
#include "view/visualisierung.h"

#include "zusi_file_lib/src/common/pfade.hpp"
#include "zusi_file_lib/src/io/fpn_leser.hpp"
#include "zusi_file_lib/src/io/st3_leser.hpp"
#include "zusi_file_lib/src/io/str_leser.hpp"

#ifdef HAS_CALLGRIND
#include <valgrind/callgrind.h>
#else
#define CALLGRIND_START_INSTRUMENTATION
#define CALLGRIND_STOP_INSTRUMENTATION
#endif

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    connect(ui->actiongroupVisualisierung, &QActionGroup::triggered, this, &MainWindow::aktualisiereDarstellung);
    connect(ui->actionVergroessern, &QAction::triggered, this->ui->streckeView, &StreckeView::vergroessern);
    connect(ui->actionVerkleinern, &QAction::triggered, this->ui->streckeView, &StreckeView::verkleinern);
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
        this->aktualisiereDarstellung();

        // Zusi 2&3
        ui->streckeView->resetTransform();
        ui->streckeView->rotate(-90);
        ui->streckeView->scale(1.0f, -1.0f);

        // Zusi 3
        if (this->m_strecken.size() > 0 &&
                (this->m_strecken[0]->formatVersion.size() == 0 || this->m_strecken[0]->formatVersion[0] != '2'))
        {
            ui->streckeView->rotate(-90);
        }

        ui->streckeView->fitInView(ui->streckeView->sceneRect(), Qt::KeepAspectRatio);
        ui->streckeView->setDefaultTransform(ui->streckeView->transform());
    }
}

void MainWindow::actionModulOeffnenTriggered()
{
    QString dateiname = this->zeigeStreckeOeffnenDialog();

    if (!dateiname.isNull())
    {
        this->oeffneStrecke(dateiname);
        this->aktualisiereDarstellung();
    }
}

void MainWindow::oeffneStrecke(QString dateiname)
{
    QTime timer;
    timer.start();

    St3Leser st3Leser;
    std::vector<std::future<std::unique_ptr<Strecke>>> futures;

#if 1
    if (dateiname.endsWith("st3", Qt::CaseInsensitive))
    {
        this->m_strecken.push_back(st3Leser.liesDateiMitDateiname(dateiname.toStdString()));
    }
    else if (dateiname.endsWith("fpn", Qt::CaseInsensitive))
    {
        auto fahrplan = FpnLeser().liesDateiMitDateiname(dateiname.toStdString());
        for (auto& modul : fahrplan->streckenmodule) {
            futures.push_back(std::async([&st3Leser, modul]{
                return st3Leser.liesDateiMitDateiname(zusi_file_lib::pfade::zusiPfadZuOsPfad(modul));
            }));
        }
    }
    else
    {
        StrLeser strLeser;
        this->m_strecken.push_back(strLeser.liesStrDateiMitDateiname(dateiname.toStdString()));
    }
#else
    for (auto& dateiname : {
         "Routes\\Deutschland\\32U_0004_0057\\000442_005692_Freienohl\\Freienohl_1985.st3",
         "Routes\\Deutschland\\32U_0004_0057\\000444_005689_Wennemen\\Wennemen_1973.st3",
         "Routes\\Deutschland\\32U_0004_0057\\000449_005689_Meschede\\Meschede_1980.st3",
         "Routes\\Deutschland\\32U_0004_0057\\000392_005690_Abzw_Rehsiepen\\Abzw_Rehsiepen_1982.st3",
         "Routes\\Deutschland\\32U_0004_0057\\000392_005691_Hagen_Hbf\\Hagen_Hbf_1985.st3",
         "Routes\\Deutschland\\32U_0004_0057\\000392_005694_Hagen_Gbf\\Hagen_Gbf_1985.st3",
         "Routes\\Deutschland\\32U_0004_0057\\000394_005696_Hengstey\\Hengstey_1975.st3",
         "Routes\\Deutschland\\32U_0004_0057\\000394_005696_Hengstey\\Hengstey_Bettungserstellung.st3",
         "Routes\\Deutschland\\32U_0004_0057\\000395_005695_Kabel\\Kabel_1985.st3",
         "Routes\\Deutschland\\32U_0004_0057\\000400_005700_Schwerte\\Schwerte_1985.st3",
         "Routes\\Deutschland\\32U_0004_0057\\000402_005702_Schwerte_Ost\\Schwerte_Ost_1982.st3",

         "Routes\\Deutschland\\32U_0005_0057\\000455_005689_Eversberg\\Eversberg_1985.st3",
         "Routes\\Deutschland\\32U_0005_0057\\000460_005690_Bestwig\\Bestwig_1985.st3",
         "Routes\\Deutschland\\32U_0005_0057\\000461_005691_Nuttlar\\Nuttlar_1980.st3",
         "Routes\\Deutschland\\32U_0005_0057\\000464_005690_Olsberg\\Olsberg_1973.st3",
         "Routes\\Deutschland\\32U_0005_0057\\000466_005689_Elleringhausen\\Elleringhausen_1980.st3",
         "Routes\\Deutschland\\32U_0005_0057\\000471_005689_Brilon_Wald\\Brilon_Wald_1985.st3",
         "Routes\\Deutschland\\32U_0005_0057\\000475_005692_Hoppecke\\Hoppecke_1980.st3",
         "Routes\\Deutschland\\32U_0005_0057\\000480_005694_Messinghausen\\Messinghausen_1980.st3",
         "Routes\\Deutschland\\32U_0005_0057\\000483_005730_Paderborn\\Paderborn_1985.st3",
         "Routes\\Deutschland\\32U_0005_0057\\000486_005697_Bredelar\\Bredelar_1978.st3",
         "Routes\\Deutschland\\32U_0005_0057\\000487_005731_Benhausen\\Benhausen_1985.st3",
         "Routes\\Deutschland\\32U_0005_0057\\000490_005701_Marsberg\\Marsberg_1978.st3",
         "Routes\\Deutschland\\32U_0005_0057\\000495_005705_Westheim\\Westheim_1985.st3",
         "Routes\\Deutschland\\32U_0005_0057\\000496_005734_Altenbeken\\Altenbeken_1985.st3",
         "Routes\\Deutschland\\32U_0005_0057\\000497_005731_Buke\\Buke_1985.st3",
         "Routes\\Deutschland\\32U_0005_0057\\000498_005723_Neuenheerse\\Neuenheerse_1985.st3",
         "Routes\\Deutschland\\32U_0005_0057\\000499_005737_Langeland\\Langeland_1985.st3",
         "Routes\\Deutschland\\32U_0005_0057\\000500_005707_Wrexen\\Wrexen_1985.st3",
         "Routes\\Deutschland\\32U_0005_0057\\000500_005719_Willebadessen\\Willebadessen_1985.st3",
         "Routes\\Deutschland\\32U_0005_0057\\000500_005739_Himmighausen\\Himmighausen_1985.st3",
         "Routes\\Deutschland\\32U_0005_0057\\000503_005732_Driburg\\Driburg_1985.st3",
         "Routes\\Deutschland\\32U_0005_0057\\000504_005709_Scherfede\\Scherfede_1974.st3",
         "Routes\\Deutschland\\32U_0005_0057\\000505_005712_Bonenburg\\Bonenburg_1985.st3",
         "Routes\\Deutschland\\32U_0005_0057\\000509_005707_Noerde\\Noerde_1985_Dummy.st3",
         "Routes\\Deutschland\\32U_0005_0057\\000509_005707_Warburg\\Warburg_1985.st3",
         "Routes\\Deutschland\\32U_0005_0057\\000517_005705_Liebenau\\Liebenau_1980.st3",
         "Routes\\Deutschland\\32U_0005_0057\\000523_005708_Lamerden\\Lamerden_1985.st3",
         "Routes\\Deutschland\\32U_0005_0057\\000527_005706_Hofgeismar\\Hofgeismar_1985.st3",
         "Routes\\Deutschland\\32U_0005_0057\\000527_005709_Huemme\\Huemme_1980.st3",
         "Routes\\Deutschland\\32U_0005_0057\\000530_005701_Grebenstein\\Grebenstein_1985.st3",
         "Routes\\Deutschland\\32U_0005_0057\\000531_005684_Kassel_Wilhelmshoehe\\Kassel_Wilhelmshoehe_1988.st3",
         "Routes\\Deutschland\\32U_0005_0057\\000531_005690_Obervellmar\\Obervellmar_1985.st3",
         "Routes\\Deutschland\\32U_0005_0057\\000531_005696_Immenhausen\\Immenhausen_1985.st3",
         "Routes\\Deutschland\\32U_0005_0057\\000532_005686_Kassel_Bahndreieck\\Kassel_Bahndreieck_1990.st3",
         "Routes\\Deutschland\\32U_0005_0057\\000532_005693_Moenchehof\\Moenchehof_1980.st3",
         "Routes\\Deutschland\\32U_0005_0057\\000533_005686_Kassel_Hbf\\Kassel_Hbf_1988.st3",
         "Routes\\Deutschland\\32U_0005_0057\\000533_005687_Harleshausen\\Harleshausen_1980.st3",
         "Routes\\Deutschland\\32U_0005_0057\\000533_005687_Kassel_Rbf\\Kassel_Rbf_1988.st3",
         "Routes\\Deutschland\\32U_0005_0057\\000534_005686_Kassel_Unterstadt\\Kassel_Unterstadt_GF_Dummy.st3",
         "Routes\\Deutschland\\32U_0005_0057\\000534_005689_Niedervellmar\\Niedervellmar_2000.st3",
         "Routes\\Deutschland\\32U_0005_0057\\000537_005690_Ihringshausen\\Ihringshausen_2000.st3",
         "Routes\\Deutschland\\32U_0005_0057\\000541_005691_Lutterberg\\Lutterberg_2000.st3",
         "Routes\\Deutschland\\32U_0005_0057\\000548_005695_Lippoldshausen\\Lippoldshausen_2000.st3",

         "Routes\\Deutschland\\32U_0005_0057\\000474_005726_Salzkotten\\Salzkotten_1994.st3",
         "Routes\\Deutschland\\32U_0006_0057\\000558_005703_Juehnde\\Juehnde_2000.st3",
         "Routes\\Deutschland\\32U_0006_0057\\000562_005708_Siekweg\\Siekweg_2000.st3",
         "Routes\\Deutschland\\32U_0006_0057\\000564_005711_Goettingen_Pbf\\Goettingen_Pbf_2000.st3",
         "Routes\\Deutschland\\32U_0006_0057\\000563_005706_Rosdorf\\Rosdorf_1998.st3",
         "Routes\\Deutschland\\32U_0006_0057\\000564_005713_Goettingen_Gbf\\Goettingen_Gbf_2000.st3",
    }) {
        futures.push_back(std::async([&st3Leser, dateiname]{ return st3Leser.liesDateiMitDateiname(zusi_file_lib::pfade::zusiPfadZuOsPfad(dateiname)); }));
    }
#endif

    for (auto& fut : futures) {
        this->m_strecken.push_back(fut.get());
    }

    qDebug() << timer.elapsed() << "ms zum Lesen der Strecken";
}

void MainWindow::aktualisiereDarstellung()
{
    unique_ptr<Visualisierung> visualisierung;

    if (ui->actionVisualisierungGeschwindigkeiten->isChecked()) {
        visualisierung = std::make_unique<GeschwindigkeitVisualisierung>();
    } else {
        visualisierung = std::make_unique<GleisfunktionVisualisierung>();
    }


    if (visualisierung) {
        ui->legendeView->setScene(visualisierung->legende().release());
        ui->legendeView->setFixedHeight(ui->legendeView->sceneRect().height());
    } else {
        ui->legendeView->scene()->clear();
        ui->legendeView->setFixedHeight(0);
    }
    ui->centralWidget->layout()->update();

    QTime timer;
    timer.start();
    ui->streckeView->setScene(new StreckeScene(this->m_strecken, *visualisierung));
    qDebug() << timer.elapsed() << "ms zum Erstellen der Segmente";
}

QString MainWindow::zeigeStreckeOeffnenDialog()
{
    QDir startverzeichnis(QString::fromStdString(zusi_file_lib::pfade::getZusi3Datenpfad()));
    return QFileDialog::getOpenFileName(this, tr("Strecke öffnen"),
                                        this->m_strecken.size() == 0? startverzeichnis.absolutePath() : QString(""),
                                        QString(tr("Strecken- und Fahrplandateien (*.str *.STR *.st3 *.ST3 *.fpn *.FPN);;Alle Dateien(*.*)")));
}
