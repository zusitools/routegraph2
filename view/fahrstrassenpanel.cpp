#include "view/fahrstrassenpanel.h"

#include <QApplication>
#include <QBrush>
#include <QEvent>
#include <QHeaderView>
#include <QItemSelectionModel>
#include <QKeyEvent>
#include <QLineEdit>
#include <QModelIndex>
#include <QPalette>
#include <QSortFilterProxyModel>
#include <QStandardItemModel>
#include <QTreeView>
#include <QVBoxLayout>

namespace {
constexpr int FahrstrasseIndexRole = Qt::UserRole + 1;
}

FahrstrassenPanel::FahrstrassenPanel(QWidget* parent)
    : QWidget(parent)
    , m_filterEdit(new QLineEdit(this))
    , m_treeView(new QTreeView(this))
    , m_model(new QStandardItemModel(this))
    , m_proxy(new QSortFilterProxyModel(this))
{
    m_filterEdit->setPlaceholderText(tr("Filter ..."));
    m_filterEdit->setClearButtonEnabled(true);

    m_model->setHorizontalHeaderLabels({ tr("Fahrstraßen") });

    m_proxy->setSourceModel(m_model);
    m_proxy->setFilterCaseSensitivity(Qt::CaseInsensitive);
    m_proxy->setRecursiveFilteringEnabled(true);

    m_treeView->setModel(m_proxy);
    m_treeView->setHeaderHidden(true);
    m_treeView->setUniformRowHeights(true);
    m_treeView->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_treeView->setSelectionMode(QAbstractItemView::SingleSelection);
    m_treeView->setExpandsOnDoubleClick(false);

    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(4, 4, 4, 4);
    layout->setSpacing(4);
    layout->addWidget(m_filterEdit);
    layout->addWidget(m_treeView);

    connect(m_filterEdit, &QLineEdit::textChanged, this, &FahrstrassenPanel::onFilterChanged);
    // currentChanged feuert sowohl bei Mausklick als auch bei Tastaturnavigation
    // (Pfeiltasten) und sorgt so für sofortige Hervorhebung beim Durchblättern.
    connect(m_treeView->selectionModel(), &QItemSelectionModel::currentChanged,
            this, &FahrstrassenPanel::onCurrentChanged);
    connect(m_treeView, &QTreeView::doubleClicked, this, &FahrstrassenPanel::onDoubleClicked);

    // Eingabe-/Return-Taste auf dem Tree wie Doppelklick behandeln.
    m_treeView->installEventFilter(this);
}

bool FahrstrassenPanel::eventFilter(QObject* watched, QEvent* event)
{
    if (watched == m_treeView && event->type() == QEvent::KeyPress) {
        const auto* keyEvent = static_cast<QKeyEvent*>(event);
        if (keyEvent->key() == Qt::Key_Return || keyEvent->key() == Qt::Key_Enter) {
            onDoubleClicked(m_treeView->currentIndex());
            return true;
        }
    }
    return QWidget::eventFilter(watched, event);
}

void FahrstrassenPanel::setzeFahrstrassen(std::vector<ResolvedFahrstrasse> fahrstrassen)
{
    m_fahrstrassen = std::move(fahrstrassen);

    m_model->removeRows(0, m_model->rowCount());

    const QBrush graueFarbe(QApplication::palette().color(QPalette::Disabled, QPalette::Text));

    QStandardItem* aktuellerKopf = nullptr;
    QString aktuelleBetriebsstelle;
    bool ersterEintrag = true;

    for (size_t i = 0; i < m_fahrstrassen.size(); ++i) {
        const auto& fs = m_fahrstrassen[i];
        const QString betriebsstelle = QString::fromStdString(fs.betriebsstelle.empty() ? std::string("(ohne Betriebsstelle)") : fs.betriebsstelle);

        if (ersterEintrag || betriebsstelle != aktuelleBetriebsstelle) {
            aktuellerKopf = new QStandardItem(betriebsstelle);
            aktuellerKopf->setEditable(false);
            aktuellerKopf->setSelectable(false);
            QFont font = aktuellerKopf->font();
            font.setBold(true);
            aktuellerKopf->setFont(font);
            m_model->appendRow(aktuellerKopf);
            aktuelleBetriebsstelle = betriebsstelle;
            ersterEintrag = false;
        }

        QString itemText = QString::fromStdString(fs.name);
        auto* item = new QStandardItem(itemText);
        item->setEditable(false);
        item->setData(static_cast<int>(i), FahrstrasseIndexRole);

        if (!fs.fehler.empty()) {
            Qt::ItemFlags flags = item->flags();
            flags &= ~Qt::ItemIsEnabled;
            flags &= ~Qt::ItemIsSelectable;
            item->setFlags(flags);
            item->setForeground(graueFarbe);
            item->setToolTip(QString::fromStdString(fs.fehler));
        } else if (!fs.typ.empty()) {
            item->setToolTip(QString::fromStdString(fs.typ));
        }

        aktuellerKopf->appendRow(item);
    }
}

int FahrstrassenPanel::fahrstrassenIndex(const QModelIndex& proxyIndex) const
{
    if (!proxyIndex.isValid()) {
        return -1;
    }
    const QModelIndex sourceIndex = m_proxy->mapToSource(proxyIndex);
    const QStandardItem* item = m_model->itemFromIndex(sourceIndex);
    if (!item) {
        return -1;
    }
    const QVariant data = item->data(FahrstrasseIndexRole);
    if (!data.isValid()) {
        return -1;
    }
    bool ok = false;
    const int idx = data.toInt(&ok);
    if (!ok || idx < 0 || static_cast<size_t>(idx) >= m_fahrstrassen.size()) {
        return -1;
    }
    if (!m_fahrstrassen[idx].fehler.empty()) {
        return -1;
    }
    return idx;
}

void FahrstrassenPanel::onFilterChanged(const QString& text)
{
    m_proxy->setFilterFixedString(text);
    if (!text.isEmpty()) {
        m_treeView->expandAll();
    }
}

void FahrstrassenPanel::onCurrentChanged(const QModelIndex& current, const QModelIndex& /*previous*/)
{
    // -1 bedeutet "keine Fahrstraße ausgewählt" (Betriebsstellen-Knoten, ausgegrauter
    // Eintrag oder leere Auswahl) – die Hervorhebung wird dann ausgeblendet.
    emit fahrstrasseAusgewaehlt(fahrstrassenIndex(current));
}

void FahrstrassenPanel::onDoubleClicked(const QModelIndex& proxyIndex)
{
    const int idx = fahrstrassenIndex(proxyIndex);
    if (idx >= 0) {
        emit fahrstrasseDoppelklick(idx);
    }
}
