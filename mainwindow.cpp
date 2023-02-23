#include "mainwindow.h"
#include "ui_mainwindow.h"

#include <future>
#include <thread>

#include <QActionGroup>
#include <QDir>
#include <QFileDialog>
#include <QMessageBox>
#include <QMimeData>
#include <QTime>
#include <QDebug>

#include "view/streckescene.h"
#include "view/visualisierung/visualisierung.h"
#include "view/visualisierung/gleisfunktionvisualisierung.h"
#include "view/visualisierung/kruemmungvisualisierung.h"
#include "view/visualisierung/ueberhoehungvisualisierung.h"
#include "view/visualisierung/neigungvisualisierung.h"
#include "view/visualisierung/geschwindigkeitvisualisierung.h"
#include "view/visualisierung/oberbauvisualisierung.h"
#include "view/visualisierung/fahrleitungvisualisierung.h"

#include "zusi_parser/utils.hpp"
#include "zusi_parser/zusi_types.hpp"

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

    connect(ui->actiongroupVisualisierung, &QActionGroup::triggered, this, &MainWindow::actionVisualisierungTriggered);
    connect(ui->actionVergroessern, &QAction::triggered, this->ui->streckeView, &StreckeView::vergroessern);
    connect(ui->actionVerkleinern, &QAction::triggered, this->ui->streckeView, &StreckeView::verkleinern);

    ui->streckeView->installEventFilter(this);
    ui->streckeView->viewport()->installEventFilter(this); // http://stackoverflow.com/a/2501489
    ui->legendeView->installEventFilter(this);
    ui->legendeView->viewport()->installEventFilter(this);

    setAcceptDrops(true);
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::setzeAnsichtZurueck()
{
    // Zusi 2&3
    ui->streckeView->resetTransform();
    ui->streckeView->rotate(-90);
    ui->streckeView->scale(1.0, -1.0);

    // Zusi 3
    ui->streckeView->rotate(-90);

    ui->streckeView->fitInView(ui->streckeView->sceneRect(), Qt::KeepAspectRatio);
    ui->streckeView->setDefaultTransform(ui->streckeView->transform());
}

void MainWindow::actionOeffnenTriggered()
{
    QStringList dateinamen = this->zeigeStreckeOeffnenDialog();

    if (dateinamen.size() > 0)
    {
        this->m_streckennetz.clear();
        this->oeffneStrecken(dateinamen);
        this->aktualisiereDarstellung();
        this->setzeAnsichtZurueck();
    }
}

void MainWindow::actionModulOeffnenTriggered()
{
    QStringList dateinamen = this->zeigeStreckeOeffnenDialog();

    if (dateinamen.size() > 0)
    {
        this->oeffneStrecken(dateinamen);
        this->aktualisiereDarstellung();
    }
}

static void findeSt3Rekursiv(QDir& dir, QStringList& filter, QStringList& result) {
    for (const QString& dateiname : dir.entryList(filter, QDir::Files | QDir::NoSymLinks)) {
        result.append(dir.path() + QLatin1Char('/') + dateiname);
    }
    for (const QString& subdir : dir.entryList(QDir::Dirs | QDir::NoSymLinks | QDir::NoDotAndDotDot)) {
        dir.cd(subdir);
        findeSt3Rekursiv(dir, filter, result);
        dir.cdUp();
    }
}

void MainWindow::actionOrdnerOeffnenTriggered()
{
    QStringList dateinamen = this->zeigeOrdnerOeffnenDialog();

    if (dateinamen.size() > 0)
    {
        this->m_streckennetz.clear();
        this->oeffneStrecken(dateinamen);
        this->aktualisiereDarstellung();
        this->setzeAnsichtZurueck();
    }
}

void MainWindow::actionOrdnerAnfuegenTriggered()
{
    QStringList dateinamen = this->zeigeOrdnerOeffnenDialog();

    if (dateinamen.size() > 0)
    {
        this->oeffneStrecken(dateinamen);
        this->aktualisiereDarstellung();
    }
}

void MainWindow::actionVisualisierungTriggered()
{
    // Transformation und Scroll-Position speichern und wiederherstellen
    const auto& transform = this->ui->streckeView->transform();
    const auto& viewport = this->ui->streckeView->viewport();
    const auto& centerPoint = this->ui->streckeView->mapToScene(viewport->width() / 2, viewport->height() / 2);
    aktualisiereDarstellung();
    this->ui->streckeView->setTransform(transform);
    this->ui->streckeView->centerOn(centerPoint);
}

void MainWindow::dragEnterEvent(QDragEnterEvent *e)
{
    if (e->mimeData()->hasUrls()) {
        e->acceptProposedAction();
    }
}

void MainWindow::dragMoveEvent(QDragMoveEvent *e)
{
    if (e->mimeData()->hasUrls()) {
        e->acceptProposedAction();
    }
}

void MainWindow::dropEvent(QDropEvent *e)
{
    QStringList dateien;
    foreach (const QUrl &url, e->mimeData()->urls()) {
        dateien.append(url.toLocalFile());
    }
    if (dateien.size() > 0) {
        if (!(e->keyboardModifiers() & Qt::ShiftModifier)) {
            this->m_streckennetz.clear();
        }
        this->oeffneStrecken(dateien);
        this->aktualisiereDarstellung();
        this->setzeAnsichtZurueck();
    }
}

bool MainWindow::eventFilter(QObject *obj, QEvent *event)
{
    Q_UNUSED(obj);
    if (event->type() == QEvent::DragEnter) {
        this->dragEnterEvent(static_cast<QDragEnterEvent*>(event));
        return true;
    } else if (event->type() == QEvent::DragMove) {
        this->dragMoveEvent(static_cast<QDragMoveEvent*>(event));
        return true;
    } else if (event->type() == QEvent::Drop) {
        this->dropEvent(static_cast<QDropEvent*>(event));
        return true;
    }
    return false;
}

void MainWindow::oeffneStrecken(const QStringList& dateinamen)
{
    QTime timer;
    timer.start();
#ifdef _GLIBCXX_HAS_GTHREADS
    std::vector<std::pair<zusixml::ZusiPfad, std::future<std::unique_ptr<Strecke>>>> futures;
#else
    std::vector<std::pair<zusixml::ZusiPfad, std::unique_ptr<Strecke>>> strecken;
#endif

#if 0
    Q_UNUSED(dateinamen);
    for (auto zusiPfad : {
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
        QString dateiname = QString::fromStdString(zusixml::ZusiPfad::vonZusiPfad(zusiPfad).alsOsPfad());
#else
    for (auto dateiname : dateinamen) {
#endif
        if (dateiname.endsWith("st3", Qt::CaseInsensitive))
        {
#ifdef _GLIBCXX_HAS_GTHREADS
            auto dateinameStdString = dateiname.toStdString();
            futures.emplace_back(
                zusixml::ZusiPfad::vonOsPfad(dateinameStdString),
                std::async([dateinameStdString]() -> std::unique_ptr<Strecke> {
                    const auto& st3 = zusixml::parseFile(dateinameStdString);
                    if (st3 && st3->Strecke) {
                        return std::move(st3->Strecke);
                    } else {
                        return nullptr;
                    }
                }));
#else
            try {
                strecken.emplace_back(zusixml::ZusiPfad::vonOsPfad(dateiname.toStdString()), std::move(zusixml::parseFile(dateiname.toStdString())->Strecke));
            } catch (const std::exception& e) {
                QMessageBox::warning(this, "Fehler beim Laden der Strecke", e.what());
            }
#endif
        }
        else if (dateiname.endsWith("fpn", Qt::CaseInsensitive))
        {
            auto fahrplan = std::move(zusixml::parseFile(dateiname.toStdString())->Fahrplan);
            if (fahrplan)
            {
                const auto& fpnZusiPfad = zusixml::ZusiPfad::vonOsPfad(dateiname.toStdString());
                for (const auto& modul : fahrplan->children_StrModul) {
                    if (!modul) { continue; }
                    auto modulDateiname = zusixml::ZusiPfad::vonZusiPfad(modul->Datei.Dateiname, fpnZusiPfad);
#ifdef _GLIBCXX_HAS_GTHREADS
                    futures.emplace_back(
                        modulDateiname,
                        std::async([modulDateiname]() -> std::unique_ptr<Strecke> {
                            const auto& st3 = zusixml::parseFile(modulDateiname.alsOsPfad());
                            if (st3 && st3->Strecke) {
                                return std::move(st3->Strecke);
                            } else {
                                return nullptr;
                            }
                        }));
#else
                    try {
                        const auto& st3 = zusixml::parseFile(modulDateiname.alsOsPfad());
                        if (st3 && st3->Strecke) {
                            strecken.emplace_back(std::move(modulDateiname), std::move(st3->Strecke));
                        }
                    } catch (const std::exception& e) {
                        QMessageBox::warning(this, "Fehler beim Laden der Strecke", e.what());
                    }
#endif
                }
            }
        }
        else
        {
            QMessageBox::warning(this, "Datei nicht geladen", QString("Unbekannte Dateiendung: ") + dateiname);
        }
    }

#ifdef _GLIBCXX_HAS_GTHREADS
    for (auto& fut : futures) {
        try {
            auto dateiname = fut.first;
            auto strecke = fut.second.get();
#else
    for (auto& [dateiname, strecke] : strecken) {
#endif
            if (!strecke) {
                continue;
            }

            // Strecke verknuepfen (innerhalb desselben Moduls)
            const auto anzahlStreckenelemente = strecke->children_StrElement.size();
            for (auto& streckenelement : strecke->children_StrElement) {
                if (!streckenelement) {
                    continue;
                }
                for (const auto& nachfolger : streckenelement->children_NachNorm) {
                    const auto nr = nachfolger.Nr;
                    streckenelement->nachfolgerElementeNorm.push_back((nr < 0) || (static_cast<size_t>(nr) >= anzahlStreckenelemente) ? nullptr : strecke->children_StrElement[nr].get());
                }
                for (const auto& nachfolger : streckenelement->children_NachGegen) {
                    const auto nr = nachfolger.Nr;
                    streckenelement->nachfolgerElementeGegen.push_back((nr < 0) || (static_cast<size_t>(nr) >= anzahlStreckenelemente) ? nullptr : strecke->children_StrElement[nr].get());
                }
            }

            this->m_streckennetz.add(dateiname, std::move(strecke));
#ifdef _GLIBCXX_HAS_GTHREADS
        } catch (const std::exception& e) {
            QMessageBox::warning(this, "Fehler beim Laden der Strecke", e.what());
        }
#endif
    }

    qDebug() << timer.elapsed() << "ms zum Lesen der Strecken";

    ui->actionModulOeffnen->setEnabled(!this->m_streckennetz.empty());
    ui->actionOrdnerAnfuegen->setEnabled(!this->m_streckennetz.empty());
}

void MainWindow::aktualisiereDarstellung()
{
    unique_ptr<Visualisierung> visualisierung;

    if (ui->actionVisualisierungGeschwindigkeiten->isChecked()) {
        visualisierung = std::make_unique<GeschwindigkeitVisualisierung>();
    } else if (ui->actionVisualisierungOberbau->isChecked()) {
        visualisierung = std::make_unique<OberbauVisualisierung>();
    } else if (ui->actionVisualisierungFahrleitung->isChecked()) {
        visualisierung = std::make_unique<FahrleitungVisualisierung>();
    } else if (ui->actionVisualisierungKruemmung->isChecked()) {
        visualisierung = std::make_unique<KruemmungVisualisierung>();
    } else if (ui->actionVisualisierungUeberhoehung->isChecked()) {
        visualisierung = std::make_unique<UeberhoehungVisualisierung>();
    } else if (ui->actionVisualisierungNeigung->isChecked()) {
        visualisierung = std::make_unique<NeigungVisualisierung>();
    } else {
        visualisierung = std::make_unique<GleisfunktionVisualisierung>();
    }

    ui->streckeView->setScene(nullptr);
    this->m_streckeScene.reset(nullptr);

    QTime timer;
    timer.start();
    this->m_streckeScene.reset(new StreckeScene(this->m_streckennetz, *visualisierung));
    qDebug() << timer.elapsed() << "ms zum Erstellen der Segmente";
    ui->streckeView->setScene(this->m_streckeScene.get());

    ui->legendeView->setScene(nullptr);
    this->m_legendeScene.reset(nullptr);

    this->m_legendeScene = visualisierung->legende();
    if (this->m_legendeScene) {
        ui->legendeView->setScene(this->m_legendeScene.get());
        ui->legendeView->setFixedHeight(ui->legendeView->sceneRect().height());
    } else {
        ui->legendeView->setFixedHeight(0);
    }
}

QStringList MainWindow::zeigeStreckeOeffnenDialog()
{
    QDir startverzeichnis(QString::fromStdString(zusixml::getZusiDatenpfad()));
    return QFileDialog::getOpenFileNames(this, tr("Strecke öffnen"),
                                        this->m_streckennetz.empty() ? startverzeichnis.absolutePath() + "/Routes" : QString(""),
                                        QString(tr("Strecken- und Fahrplandateien (*.st3 *.ST3 *.fpn *.FPN);;Alle Dateien(*.*)")));
}

QStringList MainWindow::zeigeOrdnerOeffnenDialog()
{
    QDir startverzeichnis { QString::fromStdString(zusixml::getZusiDatenpfad()) };
    QDir dir { QFileDialog::getExistingDirectory(this, "",
                                                 this->m_streckennetz.empty() ? startverzeichnis.absolutePath() + "/Routes" : QString(""),
                                                 QFileDialog::ShowDirsOnly) };
    QStringList dateinamen;
    QStringList filter { "*.ST3" };

    QTime timer;
    timer.start();
    findeSt3Rekursiv(dir, filter, dateinamen);
    qDebug() << timer.elapsed() << "ms zum Auflisten aller Streckendateien";
    return dateinamen;
}
