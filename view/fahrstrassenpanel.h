#ifndef VIEW_FAHRSTRASSENPANEL_H
#define VIEW_FAHRSTRASSENPANEL_H

#include "model/fahrstrasse.h"

#include <QWidget>

#include <vector>

class QLineEdit;
class QTreeView;
class QStandardItemModel;
class QSortFilterProxyModel;
class QModelIndex;

/**
 * Seitenleiste, die alle Fahrstraßen des aktuellen Streckennetzes in Baumform
 * anzeigt (gruppiert nach Betriebsstelle des Start- bzw. Zielsignals) und sich
 * über ein Textfeld filtern lässt.
 *
 * Nicht aufgelöste Fahrstraßen werden ausgegraut und tragen den Fehlertext als
 * Tooltip.
 */
class FahrstrassenPanel : public QWidget
{
    Q_OBJECT
public:
    explicit FahrstrassenPanel(QWidget* parent = nullptr);

    /** Setzt die anzuzeigenden Fahrstraßen und baut den Baum neu auf. */
    void setzeFahrstrassen(std::vector<ResolvedFahrstrasse> fahrstrassen);

    /** Liefert die aktuell hinterlegten Fahrstraßen (z.B. für die Pfad-Hervorhebung). */
    const std::vector<ResolvedFahrstrasse>& fahrstrassen() const { return m_fahrstrassen; }

protected:
    bool eventFilter(QObject* watched, QEvent* event) override;

signals:
    /**
     * Wird bei jedem Wechsel der aktuellen Auswahl ausgelöst. `index` ist der Index
     * der aufgelösten Fahrstraße (>= 0) oder -1, wenn die aktuelle Auswahl keine
     * Fahrstraße ist (z.B. ein Betriebsstellen-Knoten oder eine ausgegraute Fahrstraße).
     */
    void fahrstrasseAusgewaehlt(int index);

    /** Wird ausgelöst, wenn der Nutzer auf eine aufgelöste Fahrstraße doppelklickt. */
    void fahrstrasseDoppelklick(int index);

private slots:
    void onFilterChanged(const QString& text);
    void onCurrentChanged(const QModelIndex& current, const QModelIndex& previous);
    void onDoubleClicked(const QModelIndex& proxyIndex);

private:
    std::vector<ResolvedFahrstrasse> m_fahrstrassen;

    QLineEdit* m_filterEdit;
    QTreeView* m_treeView;
    QStandardItemModel* m_model;
    QSortFilterProxyModel* m_proxy;

    /** Liefert -1 falls der Index keine Fahrstraße bezeichnet (z.B. Betriebsstellen-Knoten). */
    int fahrstrassenIndex(const QModelIndex& proxyIndex) const;
};

#endif // VIEW_FAHRSTRASSENPANEL_H
