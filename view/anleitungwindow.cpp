#include "view/anleitungwindow.h"

#include <QTextBrowser>
#include <QVBoxLayout>

namespace {

// HTML-Inhalt der Anleitung. Wird vom QTextBrowser in dessen Subset von HTML/CSS
// gerendert (vgl. Qt-Doku "Supported HTML Subset"). Inline-Styles werden nur
// sparsam verwendet, da QTextBrowser nicht alle CSS-Properties unterstützt.
const char* const kAnleitungHtml = R"HTML(
<html>
<head>
<style type="text/css">
    body  { font-family: sans-serif; }
    h1    { color: #1a3a6c; }
    h2    { color: #1a3a6c; margin-top: 18px; }
    h3    { color: #2a5a9c; margin-top: 12px; }
    code  { background-color: #f0f0f0; padding: 1px 3px; }
    table { border-collapse: collapse; margin-top: 4px; margin-bottom: 4px; }
    td, th { border: 1px solid #aaaaaa; padding: 3px 8px; vertical-align: top; }
    th    { background-color: #e8e8f0; text-align: left; }
    .hinweis { color: #555555; font-style: italic; }
</style>
</head>
<body>

<h1>Anleitung: 2D-Streckenplan</h1>

<p>Der 2D-Streckenplan stellt Zusi-3-Streckenmodule (<code>*.st3</code>) als 2D-Plan dar.
Es können einzelne Module, ganze Fahrpläne (<code>*.fpn</code>) oder rekursiv
ganze Streckenordner geladen werden.</p>

<p><b>Inhalt</b></p>
<ul>
  <li><a href="#bedienung">Bedienung des Programms</a></li>
  <li><a href="#symbole">Symbole im 2D-Plan</a></li>
  <li><a href="#fahrstrassen-panel">Fahrstraßen-Seitenleiste</a></li>
  <li><a href="#fahrstrassen-details">Fahrstraßen-Details-Fenster</a></li>
</ul>

<h2 id="bedienung">Bedienung des Programms</h2>

<h3>Menü „Datei“</h3>
<table>
<tr><th>Menüpunkt</th><th>Beschreibung</th></tr>
<tr><td><b>Strecke/Fahrplan öffnen …</b></td>
    <td>Lädt eine oder mehrere <code>.st3</code>- oder <code>.fpn</code>-Dateien.
        Bereits geladene Module werden verworfen. Beim Laden einer
        <code>.fpn</code>-Datei werden alle dort referenzierten Module geöffnet.</td></tr>
<tr><td><b>Strecke/Fahrplan anfügen …</b></td>
    <td>Wie oben, aber die bereits geladenen Module bleiben erhalten.</td></tr>
<tr><td><b>Streckenordner öffnen …</b></td>
    <td>Lädt alle <code>.st3</code>-Dateien aus dem ausgewählten
        Verzeichnis und allen Unterverzeichnissen. Bereits geladene Module werden verworfen.</td></tr>
<tr><td><b>Streckenordner anfügen …</b></td>
    <td>Wie oben, aber die bereits geladenen Module bleiben erhalten.</td></tr>
<tr><td><b>Aktualisieren</b> (<code>F5</code>)</td>
    <td>Liest alle aktuell geladenen Module erneut von der Festplatte ein.</td></tr>
</table>

<p><span class="hinweis">Hinweis:</span> Streckendateien können auch per
<b>Drag &amp; Drop</b> ins Hauptfenster gezogen werden. Wird dabei die
<b>Umschalt-Taste</b> gedrückt gehalten, werden die Dateien angefügt
(statt zu ersetzen).</p>

<h3>Menü „Visualisierung“</h3>
<p>Bestimmt, wie die Streckenelemente eingefärbt werden. Es ist immer genau
eine Visualisierung aktiv. Sofern eine Legende verfügbar ist, wird sie oben
über dem Plan eingeblendet.</p>
<table>
<tr><th>Visualisierung</th><th>Bedeutung</th></tr>
<tr><td><b>Gleisfunktion/Keine Gleisfunktion</b></td>
    <td>Schwarz: normale Gleise. Hellgrau: Elemente mit Flag „keine Gleisfunktion“
        (z.&nbsp;B. Hilfselemente für Straßen).</td></tr>
<tr><td><b>Krümmung</b></td>
    <td>Farbverlauf nach Gleiskrümmung.</td></tr>
<tr><td><b>Überhöhung</b></td>
    <td>Farbverlauf nach Gleisüberhöhung.</td></tr>
<tr><td><b>Neigung</b></td>
    <td>Farbverlauf nach Längsneigung des Gleises (Steigung/Gefälle).</td></tr>
<tr><td><b>Geschwindigkeiten</b></td>
    <td>Farbverlauf nach Streckenelement-Geschwindigkeit v<sub>max</sub> (in 10-km/h-Stufen).
    Pro Element werden zwei Linien gezeichnet, da die Geschwindigkeit richtungsabhängig angegeben wird (Schema "Rechtsverkehr").
    Hellgrau bedeutet „nicht definiert“.
    Elemente ohne Gleisfunktion werden ebenfalls hellgrau dargestellt.</td></tr>
<tr><td><b>Oberbau</b></td>
    <td>Eine eigene Farbe pro Oberbau-Bezeichnung. Die Legende zeigt, welche
        Farbe zu welchem Oberbau gehört.</td></tr>
<tr><td><b>Fahrleitung</b></td>
    <td>Farbverlauf nach dem im Streckenelement angegebenen Elektrifizierungs-Typ.
    Elemente mit Elektrifizierungs-Typ, aber Fahrdrahthöhe 0 werden gestrichelt angezeigt.</td></tr>
<tr><td><b>ETCS Trusted Areas</b></td>
    <td>Hebt Streckenelemente hervor, die das Flag "ETCS-Trusted-Area" gesetzt haben.</td></tr>
</table>

<h3>Menü „Ansicht“</h3>
<table>
<tr><th>Menüpunkt</th><th>Beschreibung</th></tr>
<tr><td><b>Vergrößern</b> (<code>Strg + +</code>)</td>
    <td>Zoomt in den Plan hinein.</td></tr>
<tr><td><b>Verkleinern</b> (<code>Strg + -</code>)</td>
    <td>Zoomt aus dem Plan hinaus.</td></tr>
<tr><td><b>Betriebsstellennamen</b></td>
    <td>Blendet die Beschriftungen der Betriebsstellen ein/aus.</td></tr>
<tr><td><b>Bahnsteige</b></td>
    <td>Blendet die Darstellung von Bahnsteigen ein/aus.
    Diese richtet sich nach den Ereignissen "Bahnsteiganfang"/"Bahnsteigende" und kann bei komplexeren Konstruktionen fehlerhaft sein.</td></tr>
<tr><td><b>Fahrstraßen</b></td>
    <td>Blendet die Fahrstraßen-Seitenleiste rechts ein/aus.</td></tr>
<tr><td><b>Fahrstraßen-Details</b></td>
    <td>Öffnet das Detail-Fenster zur aktuell ausgewählten Fahrstraße
        (siehe unten).</td></tr>
</table>

<h3>Maus- und Tastaturaktionen im 2D-Plan</h3>
<table>
<tr><th>Aktion</th><th>Beschreibung</th></tr>
<tr><td><b>Linke Maustaste ziehen</b></td>
    <td>Verschiebt den Plan. Beim Erreichen des Bildschirmrands „springt“
        der Cursor auf die gegenüberliegende Seite, sodass auch große
        Strecken endlos durchgescrollt werden können.</td></tr>
<tr><td><b>Mausrad</b></td>
    <td>Zoomt unter dem Cursor herein/heraus.</td></tr>
<tr><td><b>Rechte Maustaste vertikal ziehen</b></td>
    <td>Rotiert den Plan um den Cursor-Mittelpunkt.</td></tr>
<tr><td><b>Rechtsklick (ohne Ziehen)</b> in der Nähe einer Modulgrenze</td>
    <td>Öffnet ein Kontextmenü, mit dem das benachbarte Streckenmodul nachgeladen
        werden kann (sofern es nicht bereits geladen ist).</td></tr>
<tr><td><b>Doppelklick mit rechter Maustaste</b></td>
    <td>Setzt die Rotation auf den Anfangszustand zurück und passt die
        Ansicht so an, dass der gesamte Plan angezeigt wird.</td></tr>
<tr><td><b>Mauszeiger über einem Signal/Aufgleispunkt</b></td>
    <td>Zeigt einen Tooltip mit Betriebsstelle, Signalname und ggf. Stellwerk an.</td></tr>
</table>

<h2 id="symbole">Symbole im 2D-Plan</h2>

<p>Die Streckenelemente werden je nach gewählter Visualisierung eingefärbt
(siehe oben). Zusätzlich werden folgende Symbole eingezeichnet:</p>

<table>
<tr><th>Symbol</th><th>Bedeutung</th></tr>
<tr><td><b>Dreieck</b></td>
    <td>Signal. Die Spitze zeigt in Fahrtrichtung des Signals.
        Daneben steht der Signalname. Relevant für die Farbe ist der in den Signaleigenschaften angegebene Signaltyp.</td></tr>
<tr><td><b> – dunkelgrün</b></td>
    <td>Vorsignal.</td></tr>
<tr><td><b> – blau</b></td>
    <td>Rangiersignal oder Gleissperre.</td></tr>
<tr><td><b> – rot</b></td>
    <td>Alle anderen Signale, insbesondere alle Hauptsignale.</td></tr>
<tr><td><b>Dreieck, magenta</b></td>
    <td>Aufgleispunkt.</td></tr>
<tr><td><b>Dreieck, grau</b></td>
    <td>Bahnsteigmitte-Ereignis (nur, wenn Bahnsteigdarstellung aktiv ist).</td></tr>
<tr><td><b>Graue Fläche</b></td>
    <td>Bahnsteig (nur, wenn Bahnsteigdarstellung aktiv ist).</td></tr>
<tr><td><b>Schwarzer Text</b></td>
    <td>Name einer Betriebsstelle (zentriert über deren Signalen). Lässt sich
        über <i>Ansicht → Betriebsstellennamen</i> ein-/ausblenden.</td></tr>
<tr><td><b>Hellgraues Gitter</b></td>
    <td>Hintergrundgitter mit 1&nbsp;000&nbsp;km Gitterweite.</td></tr>
<tr><td><b>Gelber, halbtransparenter Pfad</b></td>
    <td>Hervorhebung der aktuell in der Fahrstraßen-Seitenleiste gewählten Fahrstraße.</td></tr>
<tr><td><b>Roter, halbtransparenter Kreis</b></td>
    <td>Marker für das aktuell im Fahrstraßen-Details-Fenster ausgewählte Element
        (Signal, Weiche, Register, …).</td></tr>
</table>

<h2 id="fahrstrassen-panel">Fahrstraßen-Seitenleiste</h2>

<p>Die Seitenleiste wird über <i>Ansicht → Fahrstraßen</i> ein- und ausgeblendet.
Sie zeigt sämtliche Fahrstraßen aller geladenen Module gruppiert nach der
Betriebsstelle des Startsignals (bei Aufgleisfahrstraßen: des Zielsignals) an.</p>

<table>
<tr><th>Element</th><th>Beschreibung</th></tr>
<tr><td><b>Filter-Textfeld</b> (oben)</td>
    <td>Filtert die Liste (Volltextsuche, ohne Groß-/Kleinschreibung).</td></tr>
<tr><td><b>Fettgedruckter Knoten</b></td>
    <td>Eine Betriebsstelle. Aufklappen zeigt alle Fahrstraßen, deren Startsignal
    (bei Aufgleisfahrstraßen: Zielsignal) zu dieser Betriebsstelle gehört.</td></tr>
<tr><td><b>Einfacher Eintrag</b></td>
    <td>Eine konkrete Fahrstraße. Tooltip = Fahrstraßen-Typ (z.&nbsp;B.
        <code>TypZug</code>, <code>TypRangier</code>, …).</td></tr>
<tr><td><b>Ausgegrauter Eintrag</b></td>
    <td>Fahrstraße, deren Start, Ziel oder Pfad sich nicht auflösen ließ. Der
        Tooltip beschreibt den Grund (z.&nbsp;B. fehlendes Nachbarmodul). Solche
        Einträge sind nicht auswählbar.</td></tr>
<tr><td><b>Auswahl</b> (Mausklick oder Pfeiltasten)</td>
    <td>Hebt den Fahrstraßen-Pfad im 2D-Plan orange hervor; falls geöffnet, wird
        das Fahrstraßen-Details-Fenster aktualisiert.</td></tr>
<tr><td><b>Doppelklick</b> bzw. <b>Eingabetaste</b></td>
    <td>Zentriert den 2D-Plan auf die ausgewählte Fahrstraße und passt den Zoom
        so an, dass der gesamte Pfad sichtbar ist.</td></tr>
</table>

<h2 id="fahrstrassen-details">Fahrstraßen-Details-Fenster</h2>

<p>Das Fenster wird über <i>Ansicht → Fahrstraßen-Details</i> geöffnet. Es zeigt
die Detailstruktur der aktuell ausgewählten Fahrstraße an. Es ist nicht-modal
und kann parallel zum Hauptfenster verwendet werden; bei einer neuen Auswahl in
der Seitenleiste wird sein Inhalt automatisch aktualisiert.</p>

<h3>Bereich „Elemente“ (links)</h3>
<p>Listet alle Elemente der Fahrstraße in der Reihenfolge auf, wie sie in der .st3-Datei stehen.</p>
<table>
<tr><th>Eintrag</th><th>Beschreibung</th></tr>
<tr><td><b>Start</b></td>
    <td>Startelement der Fahrstraße (<code>FahrstrStart</code>).</td></tr>
<tr><td><b>Ziel</b></td>
    <td>Zielelement (<code>FahrstrZiel</code>).</td></tr>
<tr><td><b>Signal: …</b></td>
    <td>Hauptsignal-Eintrag (<code>FahrstrSignal</code>) mit der gewählten
        Signalmatrix-Zeile bzw. Ersatzsignal-Zeile.</td></tr>
<tr><td><b>Vorsignal: …</b></td>
    <td>Vorsignal-Eintrag (<code>FahrstrVSignal</code>) mit der gewählten
        Signalmatrix-Spalte.</td></tr>
<tr><td><b>Weiche: …</b></td>
    <td>Lage einer (spitz oder stumpf befahrenen) Weiche (<code>FahrstrWeiche</code>).</td></tr>
<tr><td><b>Register: …</b></td>
    <td>Belegtes Fahrstraßen-Register (<code>FahrstrRegister</code>).</td></tr>
<tr><td><b>Auflösung: …</b></td>
    <td>Punkt, an dem die Fahrstraße aufgelöst wird
        (<code>FahrstrAufloesung</code>).</td></tr>
<tr><td><b>Teilauflösung: …</b></td>
    <td>Punkt, an dem die Fahrstraße teilaufgelöst wird
        (<code>FahrstrTeilaufloesung</code>).</td></tr>
<tr><td><b>Signalhaltfall: …</b></td>
    <td>Punkt, an dem die bisher freigefahrenen Signale der Fahrstraße auf Halt fallen
        (<code>FahrstrSigHaltfall</code>).</td></tr>
<tr><td><b>Eingerückter Eintrag</b></td>
    <td>Koppelsignal eines übergeordneten Haupt- oder Vorsignals
        (<code>KoppelSignal</code>).</td></tr>
<tr><td><b>Kursiver Eintrag</b></td>
    <td>Eintrag, dessen Streckenelement nicht aufgelöst werden konnte. Der
        Tooltip zeigt den Grund.</td></tr>
</table>

<p>Aktionen in der Liste:</p>
<ul>
<li><b>Auswahl</b> (Mausklick oder Pfeiltasten): markiert das zugehörige
    Streckenelement im 2D-Plan mit einem roten Kreis.</li>
<li><b>Doppelklick</b>: zentriert den 2D-Plan auf das gewählte Element.</li>
<li><b>Tooltip</b>: zeigt zusätzliche Informationen (Element-Nummer, Richtung,
    Signalname, …).</li>
</ul>

<h3>Bereich „Vorgänger-Fahrstraßen“ und „Nachfolger-Fahrstraßen“ (rechts oben)</h3>
<p>Beide Listen enthalten andere Fahrstraßen, die direkt an die aktuelle
anschließen:</p>
<ul>
<li><b>Vorgänger</b>: Fahrstraße, deren Ziel gleich dem Start der aktuellen Fahrstraße ist.
    Die Auswahl beeinflusst, welche Vorsignale der aktuellen Fahrstraße im Bereich
    „Signalstellungen“ angezeigt werden.<br />
    <b>(alle)</b>: keine Vorgänger-Filterung – alle Vorsignale werden angezeigt.</li>
<li><b>Nachfolger</b>: Fahrstraße, deren Start gleich dem Ziel der aktuellen Fahrstraße ist.
    Die Auswahl beeinflusst, welche Spalte in den Hauptsignalen  im Bereich
    „Signalstellungen“ angesteuert wird.<br />
    <b>(keine)</b>: die Hauptsignale zeigen die Standard-Spalte (normalerweise: Halt erwarten)</li>
</ul>
<p>Einträge, die <b>keinen Einfluss</b> auf die Signalstellungen der aktuellen
Fahrstraße haben, werden ausgegraut und sind nicht auswählbar.</p>

<h3>Bereich „Signalstellungen“ (rechts unten)</h3>
<p>Zeigt für jedes Haupt- und Vorsignal der Fahrstraße eine grafische Vorschau des Signalbilds.</p>

</body>
</html>
)HTML";

}  // namespace

AnleitungWindow::AnleitungWindow(QWidget* parent)
    : QWidget(parent, Qt::Window)
    , m_browser(new QTextBrowser(this))
{
    setWindowTitle(tr("Anleitung"));
    resize(820, 720);

    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->addWidget(m_browser);

    m_browser->setOpenExternalLinks(false);
    m_browser->setOpenLinks(true);
    m_browser->setHtml(QString::fromUtf8(kAnleitungHtml));
}
