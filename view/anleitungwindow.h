#ifndef VIEW_ANLEITUNGWINDOW_H
#define VIEW_ANLEITUNGWINDOW_H

#include <QWidget>

class QTextBrowser;

/**
 * Nicht-modales Fenster, das eine ausführliche Anleitung zur Bedienung
 * von routegraph2 anzeigt (Menü, 2D-Plan, Symbole, Fahrstraßen-Seitenleiste,
 * Fahrstraßen-Details-Fenster).
 *
 * Die Anleitung ist als statisches HTML im Konstruktor hinterlegt und wird
 * über ein QTextBrowser dargestellt (Scrollbar, Anker-Sprünge im Text, ...).
 */
class AnleitungWindow : public QWidget
{
    Q_OBJECT
public:
    explicit AnleitungWindow(QWidget* parent = nullptr);

private:
    QTextBrowser* m_browser;
};

#endif // VIEW_ANLEITUNGWINDOW_H
