#include "view/fahrstrassendetailswindow.h"

#include "model/fahrstrasse.h"
#include "model/fahrstrassendetails.h"
#include "streckennetz.h"

#include "zusi_parser/utils.hpp"
#include "zusi_parser/zusi_types.hpp"

#include "ls3render/ls3render.h"

#include <QGroupBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QListWidget>
#include <QListWidgetItem>
#include <QMutex>
#include <QMutexLocker>
#include <QPixmap>
#include <QScrollArea>
#include <QSignalBlocker>
#include <QSizePolicy>
#include <QSplitter>
#include <QStringList>
#include <QThread>
#include <QVBoxLayout>
#include <QVector>

#include <atomic>
#include <cstdint>
#include <optional>
#include <string_view>
#include <unordered_map>
#include <utility>
#include <vector>

namespace {

// Globaler Mutex, der ALLE ls3render_*-Aufrufe serialisiert. ls3render hält global state
// (eine GLFW-/OpenGL-Szene); paralleles Rendering ist nicht zulässig.
QMutex& ls3renderMutex() {
    static QMutex mutex;
    return mutex;
}

// Wird einmalig (bei erster Nutzung) initialisiert. Danach ohne neues Init-/Cleanup.
bool& ls3renderInitialisiert() {
    static bool flag = false;
    return flag;
}

constexpr int Ls3PixelProMeter = 40;

/**
 * Liest die Signalbild-ID aus der Signalmatrix bzw. Ersatzsignal-Matrix anhand der
 * im FahrstrasseDetailEintrag gespeicherten Indizes.
 * Bei Fehler (Matrix leer, Index ausserhalb, MatrixEintrag fehlt) wird std::nullopt
 * zurueckgegeben und *fehler mit einem aussagekraeftigen Text gefuellt.
 */
std::optional<uint64_t> berechneSignalbild(const Signal& sig, int zeile, int spalte,
                                           bool ersatz, QString* fehler) {
    if (ersatz) {
        if (zeile < 0 || static_cast<size_t>(zeile) >= sig.children_Ersatzsignal.size()) {
            *fehler = QObject::tr("Ersatzsignal-Zeile %1 außerhalb (0-%2)")
                          .arg(zeile)
                          .arg(static_cast<int>(sig.children_Ersatzsignal.size()));
            return std::nullopt;
        }
        const auto& es = sig.children_Ersatzsignal[zeile];
        if (!es) {
            *fehler = QObject::tr("Ersatzsignal-Zeile %1 leer").arg(zeile);
            return std::nullopt;
        }
        // Ersatzsignal::MatrixEintrag ist ein Wert (InlineChildStrategy im Parsergen),
        // also kein unique_ptr und damit immer vorhanden.
        return static_cast<uint64_t>(es->MatrixEintrag.Signalbild);
    }

    const size_t anzSpalten = sig.children_VsigBegriff.size();
    if (anzSpalten == 0) {
        *fehler = QObject::tr("Signal hat keine Vorsignal-Spalten (VsigBegriff)");
        return std::nullopt;
    }
    if (zeile < 0 || spalte < 0 || static_cast<size_t>(spalte) >= anzSpalten) {
        *fehler = QObject::tr("Matrix-Index Zeile %1 / Spalte %2 außerhalb (0-%3)")
                      .arg(zeile).arg(spalte).arg(static_cast<int>(anzSpalten - 1));
        return std::nullopt;
    }
    const size_t idx = static_cast<size_t>(zeile) * anzSpalten + static_cast<size_t>(spalte);
    if (idx >= sig.children_MatrixEintrag.size()) {
        *fehler = QObject::tr("Matrix-Eintrag %1 außerhalb (Matrix hat %2 Einträge)")
                      .arg(static_cast<int>(idx))
                      .arg(static_cast<int>(sig.children_MatrixEintrag.size()));
        return std::nullopt;
    }
    // Signal::children_MatrixEintrag enthaelt MatrixEintrag-Werte (InlineChildStrategy).
    const auto& me = sig.children_MatrixEintrag[idx];
    return static_cast<uint64_t>(me.Signalbild);
}

/**
 * Löst eine FahrstrStart/FahrstrZiel/FahrstrSignal/FahrstrVSignal-artige Referenz
 * (Modul-Datei + Index in children_ReferenzElemente) auf das bezeichnete
 * StreckenelementUndRichtung auf. Liefert ein leeres (default-konstruiertes)
 * StreckenelementUndRichtung bei jeglichem Auflöse-Fehler – Fehler werden hier
 * nicht propagiert, weil die Funktion nur zur Identitätsbildung beim
 * Vorgänger-/Nachfolger-Matching verwendet wird.
 *
 * Sonderfall leerer `dateinameRef` -> dieselbe Datei wie das referenzierende
 * Modul (gleiche Semantik wie `loeseReferenzAuf` in fahrstrassendetails.cpp).
 */
StreckenelementUndRichtung loeseReferenzAuf(const Streckennetz& netz,
                                             const zusixml::ZusiPfad& fahrstrasseModul,
                                             std::string_view dateinameRef,
                                             int64_t refIndex) {
    const auto modulPfad = dateinameRef.empty()
        ? fahrstrasseModul
        : zusixml::ZusiPfad::vonZusiPfad(dateinameRef, fahrstrasseModul);
    const Strecke* strecke = netz.get(modulPfad);
    if (!strecke) return {};
    if (refIndex < 0 || static_cast<size_t>(refIndex) >= strecke->children_ReferenzElemente.size()) {
        return {};
    }
    const auto& refEl = strecke->children_ReferenzElemente[refIndex];
    if (!refEl) return {};
    const auto strElIndex = refEl->StrElement;
    const auto richtung = static_cast<StreckenelementRichtung>(refEl->StrNorm);
    if (strElIndex < 0 || static_cast<size_t>(strElIndex) >= strecke->children_StrElement.size()) {
        return {};
    }
    const auto* strEl = strecke->children_StrElement[strElIndex].get();
    if (!strEl) return {};
    return StreckenelementUndRichtung{ strEl, richtung };
}

/** Eintrag der Hauptsignal-Liste einer Fahrstraße fürs Vorgänger-Matching. */
struct VorgaengerSignalInfo {
    int matrixZeile;
    bool ersatz;
};

/**
 * Sammelt alle FahrstrSignal-Einträge der gegebenen Fahrstraße und liefert eine
 * Map vom Signal-Standort (StreckenelementUndRichtung-val) auf die zugehörigen
 * Matrix-Indizes (Zeile + Ersatzsignal-Flag). Nicht auflösbare Einträge werden
 * übersprungen.
 */
std::unordered_map<intptr_t, VorgaengerSignalInfo> sammleVorgaengerHauptsignale(
        const Streckennetz& netz, const ResolvedFahrstrasse& fs) {
    std::unordered_map<intptr_t, VorgaengerSignalInfo> result;
    if (!fs.quelle) return result;
    for (const auto& s : fs.quelle->children_FahrstrSignal) {
        if (!s) continue;
        const auto er = loeseReferenzAuf(netz, fs.fahrstrasseModul,
                                         s->Datei.Dateiname, s->Ref);
        if (!er) continue;
        result[er.val] = { s->FahrstrSignalZeile, s->FahrstrSignalErsatzsignal };
    }
    return result;
}

/** Eintrag der Vorsignal-Liste einer Fahrstraße fürs Nachfolger-Matching. */
struct NachfolgerSignalInfo {
    int matrixSpalte;
};

std::unordered_map<intptr_t, NachfolgerSignalInfo> sammleNachfolgerVorsignale(
        const Streckennetz& netz, const ResolvedFahrstrasse& fs) {
    std::unordered_map<intptr_t, NachfolgerSignalInfo> result;
    if (!fs.quelle) return result;
    for (const auto& s : fs.quelle->children_FahrstrVSignal) {
        if (!s) continue;
        const auto er = loeseReferenzAuf(netz, fs.fahrstrasseModul,
                                         s->Datei.Dateiname, s->Ref);
        if (!er) continue;
        result[er.val] = { s->FahrstrSignalSpalte };
    }
    return result;
}

}  // namespace

class FahrstrassenDetailsLs3Worker : public QObject
{
    Q_OBJECT
public:
    explicit FahrstrassenDetailsLs3Worker(std::atomic<int>* generationRef)
        : m_generationRef(generationRef) {}

public slots:
    /**
     * Rendert einen Signal-Eintrag, dessen Frames als parallele Listen übergeben
     * werden:
     *   - osPfade[i]: OS-Pfad der LS3-Datei des i-ten Frames
     *   - params[9*i + 0..2]: Position (p.x/y/z) aus <SignalFrame>
     *   - params[9*i + 3..5]: Rotation (phi.x/y/z) aus <SignalFrame>
     *   - params[9*i + 6..8]: Skalierung (sk.x/y/z) aus <SignalFrame>
     *
     * Die parallelen Listen werden verwendet, weil Qt-Standardtypen ohne weiteres
     * über Qt::QueuedConnection übergeben werden können (kein Q_DECLARE_METATYPE).
     */
    void renderEinzeln(int generation, int eintragIndex,
                       QStringList osPfade, QVector<float> params, quint64 signalbild) {
        if (m_generationRef->load() != generation) {
            return;
        }

        QImage image;
        QString fehler;

        QMutexLocker lock(&ls3renderMutex());
        if (!ls3renderInitialisiert()) {
            if (ls3render_Init() == 1) {
                ls3render_SetMultisampling(4);
                ls3render_SetPixelProMeter(Ls3PixelProMeter);
                ls3render_SetAxonometrieParameter(0.1f, 0.25f);
                ls3renderInitialisiert() = true;
            } else {
                fehler = QObject::tr("ls3render_Init fehlgeschlagen");
            }
        }

        if (fehler.isEmpty()) {
            ls3render_Reset();
            ls3render_SetSignalbild(static_cast<uint64_t>(signalbild));

            Q_ASSERT(params.size() == osPfade.size() * 9);

            int hinzugefuegt = 0;
            for (int i = 0; i < osPfade.size(); ++i) {
                const QByteArray utf8 = osPfade.at(i).toUtf8();
                const float* p = params.constData() + i * 9;
                if (ls3render_AddSignalFrame(utf8.constData(),
                                             p[0], p[1], p[2],
                                             p[3], p[4], p[5],
                                             p[6], p[7], p[8]) == 1) {
                    ++hinzugefuegt;
                }
            }

            if (hinzugefuegt == 0) {
                fehler = QObject::tr("ls3render_AddSignalFrame fehlgeschlagen für alle %1 Frame(s)")
                             .arg(osPfade.size());
            } else {
                const int bufsize = ls3render_GetAusgabepufferGroesse();
                if (bufsize <= 0) {
                    fehler = QObject::tr("Nichts zu rendern");
                } else {
                    QByteArray buf;
                    buf.resize(bufsize);
                    if (ls3render_Render(buf.data()) != 1) {
                        fehler = QObject::tr("ls3render_Render fehlgeschlagen");
                    } else {
                        const int w = ls3render_GetBildbreite();
                        const int h = ls3render_GetBildhoehe();
                        // ls3render liefert 32-Bit-Pixeldaten in BGRA-Byte-Reihenfolge
                        // (siehe ls3render.h: glReadPixels mit GL_BGRA), zeilenweise
                        // von unten nach oben. QImage::Format_ARGB32 ist als
                        // 0xAARRGGBB-Integer in nativer Byte-Reihenfolge definiert,
                        // entspricht auf Little-Endian-Systemen also genau B,G,R,A
                        // im Speicher. mirrored(false, true) dreht die Zeilen um und
                        // erzeugt eine eigene Kopie, sodass `buf` nach Rückkehr
                        // verworfen werden darf.
                        QImage tmp(reinterpret_cast<const uchar*>(buf.constData()),
                                   w, h, w * 4, QImage::Format_ARGB32);
                        image = tmp.mirrored(false, true);
                    }
                }
            }
            ls3render_Reset();
        }

        emit fertig(generation, eintragIndex, image, fehler);
    }

signals:
    void fertig(int generation, int eintragIndex, QImage image, QString fehler);

private:
    std::atomic<int>* m_generationRef;
};

FahrstrassenDetailsWindow::FahrstrassenDetailsWindow(QWidget* parent)
    : QWidget(parent, Qt::Window)
    , m_eintragsListe(new QListWidget(this))
    , m_vorgaengerListe(new QListWidget(this))
    , m_nachfolgerListe(new QListWidget(this))
    , m_signalScroll(new QScrollArea(this))
    , m_signalContainer(new QWidget(this))
    , m_signalLayout(new QHBoxLayout(m_signalContainer))
{
    setWindowTitle(tr("Fahrstraßen-Details"));
    resize(1100, 600);

    auto* zentralLayout = new QVBoxLayout(this);
    zentralLayout->setContentsMargins(4, 4, 4, 4);

    auto* hauptSplitter = new QSplitter(Qt::Horizontal, this);
    hauptSplitter->setChildrenCollapsible(false);
    zentralLayout->addWidget(hauptSplitter, 1);

    // Linker Bereich: Elementliste
    auto* linksBox = new QGroupBox(tr("Elemente"), this);
    auto* linksLayout = new QVBoxLayout(linksBox);
    linksLayout->setContentsMargins(4, 4, 4, 4);
    m_eintragsListe->setSelectionMode(QAbstractItemView::SingleSelection);
    linksLayout->addWidget(m_eintragsListe);
    hauptSplitter->addWidget(linksBox);

    // Rechter Bereich: oben Splitter mit Vorgänger/Nachfolger, unten Signal-Vorschau
    auto* rechtsSplitter = new QSplitter(Qt::Vertical, this);
    rechtsSplitter->setChildrenCollapsible(false);

    auto* obenSplitter = new QSplitter(Qt::Horizontal, this);
    obenSplitter->setChildrenCollapsible(false);
    auto* vorBox = new QGroupBox(tr("Vorgänger-Fahrstraßen"), this);
    auto* vorLayout = new QVBoxLayout(vorBox);
    vorLayout->setContentsMargins(4, 4, 4, 4);
    vorLayout->addWidget(m_vorgaengerListe);
    obenSplitter->addWidget(vorBox);
    auto* nachBox = new QGroupBox(tr("Nachfolger-Fahrstraßen"), this);
    auto* nachLayout = new QVBoxLayout(nachBox);
    nachLayout->setContentsMargins(4, 4, 4, 4);
    nachLayout->addWidget(m_nachfolgerListe);
    obenSplitter->addWidget(nachBox);
    rechtsSplitter->addWidget(obenSplitter);

    auto* signalBox = new QGroupBox(tr("Signalstellungen"), this);
    auto* signalBoxLayout = new QVBoxLayout(signalBox);
    signalBoxLayout->setContentsMargins(4, 4, 4, 4);
    m_signalScroll->setWidgetResizable(true);
    m_signalLayout->setContentsMargins(4, 4, 4, 4);
    m_signalLayout->setSpacing(8);
    m_signalLayout->addStretch(1);
    m_signalScroll->setWidget(m_signalContainer);
    signalBoxLayout->addWidget(m_signalScroll);
    rechtsSplitter->addWidget(signalBox);

    rechtsSplitter->setStretchFactor(0, 0);
    rechtsSplitter->setStretchFactor(1, 1);
    hauptSplitter->addWidget(rechtsSplitter);
    hauptSplitter->setStretchFactor(0, 0);
    hauptSplitter->setStretchFactor(1, 1);

    connect(m_eintragsListe, &QListWidget::itemSelectionChanged,
            this, &FahrstrassenDetailsWindow::onEintragAusgewaehlt);
    connect(m_eintragsListe, &QListWidget::itemDoubleClicked,
            this, &FahrstrassenDetailsWindow::onEintragDoppelklick);
    connect(m_vorgaengerListe, &QListWidget::itemSelectionChanged,
            this, &FahrstrassenDetailsWindow::onVorgaengerNachfolgerAusgewaehlt);
    connect(m_nachfolgerListe, &QListWidget::itemSelectionChanged,
            this, &FahrstrassenDetailsWindow::onVorgaengerNachfolgerAusgewaehlt);

    // QVector<float> ist kein Qt-Standard-Meta-Typ; einmalig registrieren, damit
    // er via Qt::QueuedConnection ge-Q_ARG()-fähig ist.
    qRegisterMetaType<QVector<float>>("QVector<float>");

    // Worker-Thread
    m_workerThread = new QThread(this);
    m_worker = new FahrstrassenDetailsLs3Worker(&m_generation);
    m_worker->moveToThread(m_workerThread);
    connect(m_workerThread, &QThread::finished, m_worker, &QObject::deleteLater);
    connect(m_worker, &FahrstrassenDetailsLs3Worker::fertig, this,
            [this](int generation, int eintragIndex, QImage image, QString fehler) {
                if (generation != m_generation.load()) {
                    return;
                }
                onLs3Fertig(eintragIndex, std::move(image), std::move(fehler));
            });
    m_workerThread->start();
}

FahrstrassenDetailsWindow::~FahrstrassenDetailsWindow()
{
    m_generation.fetch_add(1);
    if (m_workerThread) {
        m_workerThread->quit();
        m_workerThread->wait();
    }
}

const FahrstrasseDetailEintrag* FahrstrassenDetailsWindow::aktuellerEintrag() const
{
    const auto* item = m_eintragsListe->currentItem();
    if (!item) {
        return nullptr;
    }
    const int idx = item->data(Qt::UserRole).toInt();
    if (idx < 0 || static_cast<size_t>(idx) >= m_eintraege.size()) {
        return nullptr;
    }
    return &m_eintraege[idx];
}

void FahrstrassenDetailsWindow::zeigeFahrstrasse(const Streckennetz* netz,
                                                 const std::vector<ResolvedFahrstrasse>* alleFahrstrassen,
                                                 int aktiverIndex)
{
    m_generation.fetch_add(1);

    m_netz = netz;
    m_alleFahrstrassen = alleFahrstrassen;
    m_aktiverIndex = aktiverIndex;

    const ResolvedFahrstrasse* fs = nullptr;
    if (netz && alleFahrstrassen
            && aktiverIndex >= 0
            && static_cast<size_t>(aktiverIndex) < alleFahrstrassen->size()) {
        fs = &(*alleFahrstrassen)[aktiverIndex];
    }

    if (!fs) {
        m_eintraege.clear();
        setWindowTitle(tr("Fahrstraßen-Details"));
        aktualisiereEintraege();
        aktualisiereVorgaengerNachfolger();
        stoppeLs3Rendering();
        return;
    }

    if (!fs->fehler.empty() || !fs->quelle) {
        m_eintraege.clear();
        if (!fs->fehler.empty()) {
            setWindowTitle(tr("Fahrstraßen-Details — %1  (%2)")
                               .arg(QString::fromStdString(fs->name))
                               .arg(QString::fromStdString(fs->fehler)));
        } else {
            setWindowTitle(tr("Fahrstraßen-Details — %1")
                               .arg(QString::fromStdString(fs->name)));
        }
        aktualisiereEintraege();
        aktualisiereVorgaengerNachfolger();
        stoppeLs3Rendering();
        return;
    }

    setWindowTitle(tr("Fahrstraßen-Details — %1")
                       .arg(QString::fromStdString(fs->name)));

    m_eintraege = ermittleFahrstrasseDetails(*netz, *fs);
    aktualisiereEintraege();
    aktualisiereVorgaengerNachfolger();

    starteLs3Rendering();
}

void FahrstrassenDetailsWindow::aktualisiereVorgaengerNachfolger()
{
    // Liste-Aktualisierung soll keinen Re-Render auslösen – Signal-Block.
    QSignalBlocker blockVor(m_vorgaengerListe);
    QSignalBlocker blockNach(m_nachfolgerListe);

    m_vorgaengerListe->clear();
    m_nachfolgerListe->clear();

    if (!m_alleFahrstrassen
            || m_aktiverIndex < 0
            || static_cast<size_t>(m_aktiverIndex) >= m_alleFahrstrassen->size()) {
        return;
    }
    const auto& aktuell = (*m_alleFahrstrassen)[m_aktiverIndex];
    if (!aktuell.fehler.empty() || !aktuell.quelle) {
        return;
    }

    // Sentinel-Einträge: speichern -1 in Qt::UserRole, damit das
    // Standardverhalten leicht unterscheidbar ist.
    auto* alleItem = new QListWidgetItem(tr("(alle)"));
    alleItem->setData(Qt::UserRole, -1);
    m_vorgaengerListe->addItem(alleItem);

    auto* keineItem = new QListWidgetItem(tr("(keine)"));
    keineItem->setData(Qt::UserRole, -1);
    m_nachfolgerListe->addItem(keineItem);

    for (size_t i = 0; i < m_alleFahrstrassen->size(); ++i) {
        if (static_cast<int>(i) == m_aktiverIndex) continue;
        const auto& other = (*m_alleFahrstrassen)[i];
        if (!other.fehler.empty() || !other.quelle) continue;

        if (other.ziel && aktuell.start && other.ziel == aktuell.start) {
            auto* item = new QListWidgetItem(QString::fromStdString(other.name));
            item->setData(Qt::UserRole, static_cast<int>(i));
            m_vorgaengerListe->addItem(item);
        }
        if (other.start && aktuell.ziel && other.start == aktuell.ziel) {
            auto* item = new QListWidgetItem(QString::fromStdString(other.name));
            item->setData(Qt::UserRole, static_cast<int>(i));
            m_nachfolgerListe->addItem(item);
        }
    }

    // Default-Auswahl: Sentinel-Eintrag (entspricht „bisheriges Verhalten“).
    m_vorgaengerListe->setCurrentRow(0);
    m_nachfolgerListe->setCurrentRow(0);
}

int FahrstrassenDetailsWindow::aktuelleVorgaengerFsIndex() const
{
    const auto* item = m_vorgaengerListe->currentItem();
    if (!item) return -1;
    return item->data(Qt::UserRole).toInt();
}

int FahrstrassenDetailsWindow::aktuelleNachfolgerFsIndex() const
{
    const auto* item = m_nachfolgerListe->currentItem();
    if (!item) return -1;
    return item->data(Qt::UserRole).toInt();
}

void FahrstrassenDetailsWindow::aktualisiereEintraege()
{
    m_eintragsListe->blockSignals(true);
    m_eintragsListe->clear();
    for (size_t i = 0; i < m_eintraege.size(); ++i) {
        const auto& e = m_eintraege[i];
        // Koppelsignale werden entsprechend ihrer Kopplungs-Tiefe eingerückt
        // (1 Tiefe = 4 Leerzeichen).
        QString text = QString::fromStdString(e.label);
        if (e.typ == FahrstrasseDetailEintrag::Typ::Koppelsignal && e.kopplungsTiefe > 0) {
            text = QString(e.kopplungsTiefe * 4, QLatin1Char(' ')) + text;
        }
        auto* item = new QListWidgetItem(text);
        item->setData(Qt::UserRole, static_cast<int>(i));
        if (!e.fehler.empty()) {
            item->setToolTip(QString::fromStdString(e.fehler));
            item->setFlags(item->flags() & ~Qt::ItemIsSelectable);
            QFont font = item->font();
            font.setItalic(true);
            item->setFont(font);
        } else if (!e.beschreibung.empty()) {
            item->setToolTip(QString::fromStdString(e.beschreibung));
        }
        m_eintragsListe->addItem(item);
    }
    m_eintragsListe->blockSignals(false);
}

void FahrstrassenDetailsWindow::onEintragAusgewaehlt()
{
    const auto* item = m_eintragsListe->currentItem();
    if (!item) {
        emit detailAusgewaehlt(-1);
        return;
    }
    const int idx = item->data(Qt::UserRole).toInt();
    if (idx < 0 || static_cast<size_t>(idx) >= m_eintraege.size() || !m_eintraege[idx].fehler.empty()) {
        emit detailAusgewaehlt(-1);
        return;
    }
    emit detailAusgewaehlt(idx);
}

void FahrstrassenDetailsWindow::onEintragDoppelklick(QListWidgetItem* item)
{
    if (!item) {
        return;
    }
    const int idx = item->data(Qt::UserRole).toInt();
    if (idx < 0 || static_cast<size_t>(idx) >= m_eintraege.size() || !m_eintraege[idx].fehler.empty()) {
        return;
    }
    emit detailDoppelklick(idx);
}

void FahrstrassenDetailsWindow::onVorgaengerNachfolgerAusgewaehlt()
{
    // Eine neue Auswahl macht eventuell laufende LS3-Aufträge obsolet (sie haben
    // ggf. die alten Matrix-Indizes verwendet). Generation hochzählen, dann neu
    // aufbauen – bisherige Slots werden in starteLs3Rendering komplett ersetzt.
    m_generation.fetch_add(1);
    starteLs3Rendering();
}

void FahrstrassenDetailsWindow::stoppeLs3Rendering()
{
    // Layout leeren (alle Slots entfernen).
    while (m_signalLayout->count() > 0) {
        QLayoutItem* item = m_signalLayout->takeAt(0);
        if (auto* w = item->widget()) {
            w->deleteLater();
        }
        delete item;
    }
    m_signalLayout->addStretch(1);
    m_signalSlots.clear();
}

std::vector<FahrstrassenDetailsWindow::EintragAnzeige>
FahrstrassenDetailsWindow::berechneEintragAnzeige() const
{
    std::vector<EintragAnzeige> anzeige(m_eintraege.size());
    for (size_t i = 0; i < m_eintraege.size(); ++i) {
        const auto& e = m_eintraege[i];
        anzeige[i].matrixZeile = e.matrixZeile;
        anzeige[i].matrixSpalte = e.matrixSpalte;
        anzeige[i].ersatzsignal = e.ersatzsignal;
    }

    // Helfer: alle direkt folgenden Koppelsignal-Einträge des angegebenen Roots
    // (zuVsig grenzt die Vorsignal- von der Hauptsignal-Gruppe ab) iterieren.
    const auto fuerKoppelKette =
        [this](size_t rootIdx, bool zuVsig, auto&& fn) {
            for (size_t j = rootIdx + 1; j < m_eintraege.size(); ++j) {
                const auto& cand = m_eintraege[j];
                if (cand.typ != FahrstrasseDetailEintrag::Typ::Koppelsignal) break;
                if (cand.kopplungZuVsig != zuVsig) break;
                fn(j);
            }
        };

    // Vorgänger-Override: Vorsignale, die in der Vorgänger-Fahrstraße als
    // Hauptsignale verknüpft sind, werden mit deren Zeile/Ersatzsignal-Flag
    // gerendert. Vorsignale ohne Match werden ausgeblendet.
    const int vorgIdx = aktuelleVorgaengerFsIndex();
    if (m_netz && m_alleFahrstrassen
            && vorgIdx >= 0
            && static_cast<size_t>(vorgIdx) < m_alleFahrstrassen->size()) {
        const auto& vorg = (*m_alleFahrstrassen)[vorgIdx];
        const auto vorgMap = sammleVorgaengerHauptsignale(*m_netz, vorg);
        for (size_t i = 0; i < m_eintraege.size(); ++i) {
            const auto& e = m_eintraege[i];
            if (e.typ != FahrstrasseDetailEintrag::Typ::FahrstrVSignal) continue;
            const auto er = e.elementUndRichtung;
            const auto it = er ? vorgMap.find(er.val) : vorgMap.end();
            if (it == vorgMap.end()) {
                anzeige[i].ausgeblendet = true;
                fuerKoppelKette(i, /*zuVsig=*/true, [&](size_t j) {
                    anzeige[j].ausgeblendet = true;
                });
            } else {
                anzeige[i].matrixZeile = it->second.matrixZeile;
                anzeige[i].ersatzsignal = it->second.ersatz;
                fuerKoppelKette(i, /*zuVsig=*/true, [&](size_t j) {
                    anzeige[j].matrixZeile = it->second.matrixZeile;
                    anzeige[j].ersatzsignal = it->second.ersatz;
                });
            }
        }
    }

    // Nachfolger-Override: Hauptsignale, die in der Nachfolger-Fahrstraße als
    // Vorsignale verknüpft sind, bekommen die dort angegebene Spalte. Die Zeile
    // bleibt aus der aktuellen Fahrstraße. Bei gezogenem Ersatzsignal wird die
    // Spalte ohnehin ignoriert (siehe berechneSignalbild).
    const int nachIdx = aktuelleNachfolgerFsIndex();
    if (m_netz && m_alleFahrstrassen
            && nachIdx >= 0
            && static_cast<size_t>(nachIdx) < m_alleFahrstrassen->size()) {
        const auto& nach = (*m_alleFahrstrassen)[nachIdx];
        const auto nachMap = sammleNachfolgerVorsignale(*m_netz, nach);
        for (size_t i = 0; i < m_eintraege.size(); ++i) {
            const auto& e = m_eintraege[i];
            if (e.typ != FahrstrasseDetailEintrag::Typ::FahrstrSignal) continue;
            const auto er = e.elementUndRichtung;
            const auto it = er ? nachMap.find(er.val) : nachMap.end();
            if (it == nachMap.end()) continue;
            if (!anzeige[i].ersatzsignal) {
                anzeige[i].matrixSpalte = it->second.matrixSpalte;
            }
            fuerKoppelKette(i, /*zuVsig=*/false, [&](size_t j) {
                if (!anzeige[j].ersatzsignal) {
                    anzeige[j].matrixSpalte = it->second.matrixSpalte;
                }
            });
        }
    }

    return anzeige;
}

void FahrstrassenDetailsWindow::starteLs3Rendering()
{
    stoppeLs3Rendering();

    const auto anzeige = berechneEintragAnzeige();

    // Reihenfolge: erst Vorsignale, dann Hauptsignale (entspricht dem Plan
    // „links nach rechts“; pro Gruppe in Eintragsreihenfolge). Koppelsignale
    // werden jeweils direkt hinter ihrem Wurzelsignal in derselben Gruppe
    // mitgerendert (kopplungZuVsig markiert die Wurzel-Zugehörigkeit).
    // Ausgeblendete Einträge (z.B. wegen Vorgänger-Filter) werden übersprungen,
    // brechen die Koppel-Kette aber nicht ab.
    std::vector<int> reihenfolge;
    auto sammleGruppe = [&](FahrstrasseDetailEintrag::Typ wurzelTyp, bool zuVsig) {
        for (size_t i = 0; i < m_eintraege.size(); ++i) {
            if (m_eintraege[i].typ != wurzelTyp) {
                continue;
            }
            if (!anzeige[i].ausgeblendet) {
                reihenfolge.push_back(static_cast<int>(i));
            }
            for (size_t j = i + 1; j < m_eintraege.size(); ++j) {
                const auto& cand = m_eintraege[j];
                if (cand.typ != FahrstrasseDetailEintrag::Typ::Koppelsignal) {
                    break;
                }
                if (cand.kopplungZuVsig != zuVsig) {
                    break;
                }
                if (anzeige[j].ausgeblendet) {
                    continue;
                }
                reihenfolge.push_back(static_cast<int>(j));
            }
        }
    };
    sammleGruppe(FahrstrasseDetailEintrag::Typ::FahrstrVSignal, /*zuVsig=*/true);
    sammleGruppe(FahrstrasseDetailEintrag::Typ::FahrstrSignal, /*zuVsig=*/false);

    if (reihenfolge.empty()) {
        return;
    }

    // Layout aufbauen: Stretch hinten lassen, Slots links davor einfügen.
    // Wir entfernen den Stretch erst und hängen ihn am Ende wieder an.
    while (m_signalLayout->count() > 0) {
        QLayoutItem* item = m_signalLayout->takeAt(0);
        delete item;
    }

    const int gen = m_generation.load();

    for (int idx : reihenfolge) {
        const auto& e = m_eintraege[idx];

        auto* slotWidget = new QWidget(m_signalContainer);
        auto* slotLayout = new QVBoxLayout(slotWidget);
        slotLayout->setContentsMargins(0, 0, 0, 0);
        slotLayout->setSpacing(2);

        auto* bildLabel = new QLabel(slotWidget);
        bildLabel->setMinimumSize(120, 160);
        bildLabel->setAlignment(Qt::AlignCenter);
        bildLabel->setText(tr("Wird gerendert …"));
        bildLabel->setFrameShape(QFrame::StyledPanel);
        slotLayout->addWidget(bildLabel);

        QString caption = QString::fromStdString(e.label);
        // Label hat das Fallback-Präfix; für die Caption die reinen Signal-Daten verwenden.
        if (e.signal) {
            const QString bs = QString::fromStdString(e.signal->NameBetriebsstelle);
            const QString sn = QString::fromStdString(e.signal->Signalname);
            if (!bs.isEmpty() || !sn.isEmpty()) {
                caption = bs;
                if (!bs.isEmpty() && !sn.isEmpty()) caption += QStringLiteral(" ");
                caption += sn;
            }
        }
        // Koppelsignal-Einträge auch in der Visualisierung als solche kennzeichnen.
        if (e.typ == FahrstrasseDetailEintrag::Typ::Koppelsignal) {
            caption = tr("Koppelsignal: %1").arg(caption);
        }
        auto* textLabel = new QLabel(caption, slotWidget);
        textLabel->setAlignment(Qt::AlignCenter);
        textLabel->setWordWrap(true);
        slotLayout->addWidget(textLabel);

        m_signalLayout->addWidget(slotWidget);

        SignalSlot slot;
        slot.eintragIndex = idx;
        slot.bildLabel = bildLabel;
        slot.textLabel = textLabel;
        m_signalSlots.push_back(slot);
    }

    m_signalLayout->addStretch(1);

    // Aufträge dispatchen: pro Signal-Eintrag werden alle <SignalFrame>-Einträge
    // mit nicht-leerer LS3-Datei gerendert (Multi-Frame-Signale).
    for (size_t s = 0; s < m_signalSlots.size(); ++s) {
        const SignalSlot& slot = m_signalSlots[s];
        const auto& e = m_eintraege[slot.eintragIndex];
        if (!e.signal) {
            slot.bildLabel->setText(tr("(kein Signal)"));
            continue;
        }

        // Signalbild aus der Signalmatrix bzw. Ersatzsignal-Matrix bestimmen.
        // Indizes ggf. durch Vorgänger-/Nachfolger-Auswahl überschrieben.
        const auto& a = anzeige[slot.eintragIndex];
        QString signalbildFehler;
        const auto signalbild = berechneSignalbild(*e.signal,
                                                   a.matrixZeile, a.matrixSpalte,
                                                   a.ersatzsignal, &signalbildFehler);
        if (!signalbild.has_value()) {
            slot.bildLabel->setText(signalbildFehler);
            slot.bildLabel->setToolTip(signalbildFehler);
            continue;
        }

        QStringList osPfade;
        QVector<float> params;
        params.reserve(static_cast<int>(e.signal->children_SignalFrame.size()) * 9);
        for (const auto& frame : e.signal->children_SignalFrame) {
            if (!frame) continue;
            if (frame->Datei.Dateiname.empty()) continue;
            const auto absPfad = zusixml::ZusiPfad::vonZusiPfad(frame->Datei.Dateiname, e.signalModul);
            osPfade.append(QString::fromStdString(absPfad.alsOsPfad()));
            params.append(frame->p.x);    params.append(frame->p.y);    params.append(frame->p.z);
            params.append(frame->phi.x);  params.append(frame->phi.y);  params.append(frame->phi.z);
            params.append(frame->sk.x);   params.append(frame->sk.y);   params.append(frame->sk.z);
        }
        if (osPfade.isEmpty()) {
            slot.bildLabel->setText(tr("(keine LS3-Datei)"));
            continue;
        }

        // QueuedConnection: Worker liegt in eigenem Thread.
        QMetaObject::invokeMethod(m_worker, "renderEinzeln", Qt::QueuedConnection,
                                  Q_ARG(int, gen),
                                  Q_ARG(int, slot.eintragIndex),
                                  Q_ARG(QStringList, osPfade),
                                  Q_ARG(QVector<float>, params),
                                  Q_ARG(quint64, static_cast<quint64>(*signalbild)));
    }
}

void FahrstrassenDetailsWindow::onLs3Fertig(int eintragIndex, QImage image, QString fehler)
{
    for (const auto& slot : m_signalSlots) {
        if (slot.eintragIndex != eintragIndex) {
            continue;
        }
        if (!fehler.isEmpty()) {
            slot.bildLabel->setText(fehler);
            slot.bildLabel->setToolTip(fehler);
        } else if (image.isNull()) {
            slot.bildLabel->setText(tr("(leeres Bild)"));
        } else {
            QPixmap pm = QPixmap::fromImage(image);
            // Auf eine handhabbare Höhe skalieren.
            const int maxH = 220;
            if (pm.height() > maxH) {
                pm = pm.scaledToHeight(maxH, Qt::SmoothTransformation);
            }
            slot.bildLabel->setPixmap(pm);
            slot.bildLabel->setToolTip({});
            slot.bildLabel->setMinimumSize(pm.size().expandedTo(QSize(120, 160)));
        }
        return;
    }
}

#include "fahrstrassendetailswindow.moc"
