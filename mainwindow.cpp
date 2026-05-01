#include "mainwindow.h"
#include "ui_mainwindow.h"

#include <algorithm>
#include <future>
#include <thread>

#include <QActionGroup>
#include <QDebug>
#include <QDir>
#include <QLabel>
#include <QElapsedTimer>
#include <QFileDialog>
#include <QMessageBox>
#include <QMimeData>
#include <QProgressDialog>

#include "view/fahrstrassenpanel.h"
#include "view/streckescene.h"
#include "view/visualisierung/visualisierung.h"
#include "view/visualisierung/etcstrustedareavisualisierung.h"
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

    connect(ui->fahrstrassenPanel, &FahrstrassenPanel::fahrstrasseAusgewaehlt,
            this, &MainWindow::onFahrstrasseAusgewaehlt);
    connect(ui->fahrstrassenPanel, &FahrstrassenPanel::fahrstrasseDoppelklick,
            this, &MainWindow::onFahrstrasseDoppelklick);

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
        this->m_streckennetz = {};
        this->m_aktiveFahrstrasse.reset();
        this->ui->fahrstrassenPanel->setzeFahrstrassen({});
        this->oeffneStrecken(dateinamen);
        this->fahrstrassenListeUngueltig();
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
        this->fahrstrassenListeUngueltig();
        this->aktualisiereDarstellung();
    }
}

void MainWindow::actionOrdnerOeffnenTriggered()
{
    QStringList dateinamen = this->zeigeOrdnerOeffnenDialog();

    if (dateinamen.size() > 0)
    {
        this->m_streckennetz = {};
        this->m_aktiveFahrstrasse.reset();
        this->ui->fahrstrassenPanel->setzeFahrstrassen({});
        this->oeffneStrecken(dateinamen);
        this->fahrstrassenListeUngueltig();
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
        this->fahrstrassenListeUngueltig();
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

void MainWindow::actionAnsichtBetriebsstellennamenToggled(bool checked)
{
    m_zeigeBetriebsstellen = checked;

    // Transformation und Scroll-Position speichern und wiederherstellen
    const auto& transform = this->ui->streckeView->transform();
    const auto& viewport = this->ui->streckeView->viewport();
    const auto& centerPoint = this->ui->streckeView->mapToScene(viewport->width() / 2, viewport->height() / 2);
    aktualisiereDarstellung();
    this->ui->streckeView->setTransform(transform);
    this->ui->streckeView->centerOn(centerPoint);
}

void MainWindow::actionAnsichtBahnsteigeToggled(bool checked)
{
    m_zeigeBahnsteige = checked;

    const auto& transform = this->ui->streckeView->transform();
    const auto& viewport = this->ui->streckeView->viewport();
    const auto& centerPoint = this->ui->streckeView->mapToScene(viewport->width() / 2, viewport->height() / 2);
    aktualisiereDarstellung();
    this->ui->streckeView->setTransform(transform);
    this->ui->streckeView->centerOn(centerPoint);
}

void MainWindow::actionAnsichtFahrstrassenToggled(bool checked)
{
    ui->fahrstrassenPanel->setVisible(checked);
    if (checked && !m_fahrstrassenListeAktuell) {
        aktualisiereFahrstrassenListe();
    }
}

void MainWindow::onFahrstrasseAusgewaehlt(int index)
{
    if (index < 0) {
        m_aktiveFahrstrasse.reset();
        if (m_streckeScene) {
            m_streckeScene->verbirgFahrstrasse();
        }
        return;
    }
    if (static_cast<size_t>(index) >= ui->fahrstrassenPanel->fahrstrassen().size()) {
        return;
    }
    m_aktiveFahrstrasse = index;
    wendeFahrstrassenHervorhebungAn();
}

void MainWindow::onFahrstrasseDoppelklick(int index)
{
    if (index < 0 || static_cast<size_t>(index) >= ui->fahrstrassenPanel->fahrstrassen().size()) {
        return;
    }
    m_aktiveFahrstrasse = index;
    wendeFahrstrassenHervorhebungAn();

    if (!m_streckeScene) {
        return;
    }
    const auto& fs = ui->fahrstrassenPanel->fahrstrassen()[index];
    QRectF bbox = m_streckeScene->boundingRectFuerPfad(fs.pfad);
    if (bbox.isValid() && !bbox.isEmpty()) {
        bbox = bbox.adjusted(-50, -50, 50, 50);
        ui->streckeView->fitInView(bbox, Qt::KeepAspectRatio);
    }
}

void MainWindow::aktualisiereFahrstrassenListe()
{
    auto fahrstrassen = ermittleFahrstrassen(m_streckennetz);
    ui->fahrstrassenPanel->setzeFahrstrassen(std::move(fahrstrassen));
    m_fahrstrassenListeAktuell = true;

    // Aktiver Index könnte nicht mehr passen (Reihenfolge geändert oder Modul entfernt).
    if (m_aktiveFahrstrasse) {
        const auto& list = ui->fahrstrassenPanel->fahrstrassen();
        if (static_cast<size_t>(*m_aktiveFahrstrasse) >= list.size() ||
                !list[*m_aktiveFahrstrasse].fehler.empty()) {
            m_aktiveFahrstrasse.reset();
            if (m_streckeScene) {
                m_streckeScene->verbirgFahrstrasse();
            }
        }
    }
}

void MainWindow::fahrstrassenListeUngueltig()
{
    m_fahrstrassenListeAktuell = false;
    if (ui->fahrstrassenPanel->isVisible()) {
        aktualisiereFahrstrassenListe();
    }
}

void MainWindow::wendeFahrstrassenHervorhebungAn()
{
    if (!m_streckeScene) {
        return;
    }
    if (!m_aktiveFahrstrasse) {
        m_streckeScene->verbirgFahrstrasse();
        return;
    }
    const auto& list = ui->fahrstrassenPanel->fahrstrassen();
    if (static_cast<size_t>(*m_aktiveFahrstrasse) >= list.size()) {
        m_streckeScene->verbirgFahrstrasse();
        return;
    }
    const auto& fs = list[*m_aktiveFahrstrasse];
    if (!fs.fehler.empty() || fs.pfad.empty()) {
        m_streckeScene->verbirgFahrstrasse();
        return;
    }
    m_streckeScene->zeigeFahrstrasse(fs.pfad);
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
            this->m_streckennetz = {};
            this->m_aktiveFahrstrasse.reset();
            this->ui->fahrstrassenPanel->setzeFahrstrassen({});
        }
        this->oeffneStrecken(dateien);
        this->fahrstrassenListeUngueltig();
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
    QElapsedTimer timer;
    timer.start();
    std::vector<std::pair<zusixml::ZusiPfad, std::future<std::unique_ptr<Strecke>>>> futures;
    std::vector<std::pair<zusixml::ZusiPfad, std::unique_ptr<Strecke>>> streckenOhneUtmPunkt;
    bool hatStreckeMitUtmPunkt = false;

    for (auto dateiname : dateinamen) {
        if (dateiname.endsWith("st3", Qt::CaseInsensitive))
        {
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
                }
            }
        }
        else
        {
            QMessageBox::warning(this, "Datei nicht geladen", QString("Unbekannte Dateiendung: ") + dateiname);
        }
    }

    for (auto& fut : futures) {
        try {
            auto dateiname = fut.first;
            auto strecke = fut.second.get();
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

            if (!strecke->UTM || (strecke->UTM->UTM_NS == 0 && strecke->UTM->UTM_WE == 0)) {
                streckenOhneUtmPunkt.emplace_back(dateiname, std::move(strecke));
            } else {
                hatStreckeMitUtmPunkt = true;
                this->m_streckennetz.add(dateiname, std::move(strecke));
            }
        } catch (const std::exception& e) {
            QMessageBox::warning(this, "Fehler beim Laden der Strecke", e.what());
        }
    }

    if (hatStreckeMitUtmPunkt) {
        qDebug() << "Ignoriere" << streckenOhneUtmPunkt.size() << "Module ohne UTM-Punkt";
    } else {
        for (auto& [dateiname, strecke] : streckenOhneUtmPunkt) {
            this->m_streckennetz.add(dateiname, std::move(strecke));
        }
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
    } else if (ui->actionVisualisierungEtcsTrustedAreas->isChecked()) {
        visualisierung = std::make_unique<EtcsTrustedAreaVisualisierung>();
    } else {
        visualisierung = std::make_unique<GleisfunktionVisualisierung>();
    }

    ui->streckeView->setScene(nullptr);
    this->m_streckeScene.reset(nullptr);

    QElapsedTimer timer;
    timer.start();
    this->m_streckeScene.reset(new StreckeScene(this->m_streckennetz, *visualisierung, m_zeigeBetriebsstellen, m_zeigeBahnsteige));
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

    // Hervorhebung der aktiven Fahrstraße über Visualisierungs-/Anzeigewechsel
    // hinweg erhalten (die Szene wurde gerade neu gebaut).
    wendeFahrstrassenHervorhebungAn();
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
    const QStringList filter { "*.ST3" };
    return this->findeSt3Rekursiv(dir, filter);
}

namespace {

void findeSt3RekursivIntern(QDir& dir, const QStringList& filter, QStringList& result, QProgressDialog& progressDialog, QElapsedTimer& progressTimer) {
    if (progressDialog.wasCanceled()) {
        return;
    }
    if (progressTimer.hasExpired(100)) {
        progressDialog.setLabelText(dir.path());
        QApplication::processEvents();
        progressTimer.restart();
    }
    for (const QString& dateiname : dir.entryList(filter, QDir::Files)) {
        result.append(dir.path() + QLatin1Char('/') + dateiname);
    }
    for (const QString& subdir : dir.entryList(QDir::Dirs | QDir::NoDotAndDotDot)) {
        dir.cd(subdir);
        findeSt3RekursivIntern(dir, filter, result, progressDialog, progressTimer);
        dir.cdUp();
    }
}

}

QStringList MainWindow::findeSt3Rekursiv(QDir& dir, const QStringList& filter) {
    QElapsedTimer timer;
    timer.start();

    QProgressDialog progressDialog("", "Abbrechen", 0, 0, this);
    auto label = std::make_unique<QLabel>("", this);
    label->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    progressDialog.setLabel(label.release());
    progressDialog.setWindowTitle("Suche ST3-Dateien");
    progressDialog.setWindowModality(Qt::WindowModal);
    progressDialog.setAutoClose(false);
    progressDialog.setValue(0);
    QElapsedTimer progressTimer;
    progressTimer.start();

    QStringList result;
    findeSt3RekursivIntern(dir, filter, result, progressDialog, progressTimer);
    if (progressDialog.wasCanceled()) {
        return {};
    }
    qDebug() << timer.elapsed() << "ms zum Auflisten aller" << result.size() << "Streckendateien";
    return result;
}
