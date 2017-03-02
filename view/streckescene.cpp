#include "streckescene.h"

#include "model/streckenelement.hpp"

#include "view/graphicsitems/streckensegmentitem.h"
#include "view/graphicsitems/dreieckitem.h"
#include "view/segmentierer.h"

#include <QDebug>

#include <cassert>
#include <cmath>

void setzeDarstellung_Zufallsfarbe(StreckensegmentItem &item, const StreckenelementUndRichtung &start)
{
    (void)start;
    QPen pen = item.pen();
    pen.setColor(QColor::fromRgb(rand() % 256, rand() % 256, rand() % 256));
    item.setPen(pen);
}

void setzeDarstellung_Gleisfunktion(StreckensegmentItem &item, const StreckenelementUndRichtung &start)
{
    QPen pen = item.pen();
    pen.setColor(start->hatFktFlag(StreckenelementFlag::KeineGleisfunktion) ? Qt::lightGray : Qt::black);
    item.setPen(pen);
}

void setzeDarstellung_Geschwindigkeit(StreckensegmentItem &item, const StreckenelementUndRichtung &start)
{
    QPen pen = item.pen();
    geschwindigkeit_t geschwindigkeit = start.richtungsInfo().vmax;
    if (start->hatFktFlag(StreckenelementFlag::KeineGleisfunktion) || geschwindigkeit <= 0) {
        pen.setColor(Qt::lightGray);
    } else {
        int colormap[17] = { 300, 286, 270, 258, 247, 214, 197, 180, 160, 130, 100, 70, 58, 50, 40, 30, 0 };
        int idx = std::round(geschwindigkeit * 3.6) / 10 - 1; /* <= 10 km/h, <= 20 km/h, ..., >= 170 km/h */
        int hue = colormap[std::max(0, std::min(16, idx))];
        pen.setColor(QColor::fromHsv(hue, 255, hue == 130 ? 230 : 255));
    }
    item.setPen(pen);
}

StreckeScene::StreckeScene(vector<reference_wrapper<unique_ptr<Strecke> > > strecken, QObject *parent) :
    QGraphicsScene(parent)
{
    this->setItemIndexMethod(QGraphicsScene::NoIndex);
    size_t anzahlSegmente = 0, anzahlStreckenelemente = 0;

    // Berechne UTM-Referenzpunkt als Mittelwert der Strecken-Referenzpunkte
    double utmRefWe = 0.0;
    double utmRefNs = 0.0;
    for (unique_ptr<Strecke> &strecke : strecken)
    {
        utmRefWe += strecke->utmPunkt.we / float(strecken.size());
        utmRefNs += strecke->utmPunkt.ns / float(strecken.size());
    }
    this->m_utmRefPunkt.we = int(utmRefWe);
    this->m_utmRefPunkt.ns = int(utmRefNs);

    Segmentierer istSegmentStart = GeschwindigkeitSegmentierer();
    const auto richtungen_zusi2 = { Streckenelement::RICHTUNG_NORM };
    const auto richtungen_zusi3 = { Streckenelement::RICHTUNG_NORM, Streckenelement::RICHTUNG_GEGEN };

    for (unique_ptr<Strecke> &strecke : strecken)
    {
        // Die Betriebsstellen werden pro Streckenmodul beschriftet, da manche Betriebsstellennamen
        // (z.B. Sbk-Bezeichnungen) in mehreren Modulen vorkommen und dann falsch platziert wuerden.
        std::unordered_map<std::string, QRectF> betriebsstellenKoordinaten;

        bool istZusi2 = strecke->formatVersion.size() > 0 && strecke->formatVersion[0] == '2';
        auto richtungen = istZusi2 ? richtungen_zusi2 : richtungen_zusi3;

        for (auto& streckenelement : strecke->streckenelemente)
        {
            if (streckenelement)
            {
                anzahlStreckenelemente++;
                for (streckenelement_richtung_t richtung : richtungen)
                {
                    StreckenelementUndRichtung elementRichtung = streckenelement->richtung(richtung);
                    // Streckenelement-Segmente
                    if (istSegmentStart(elementRichtung))
                    {
                        auto item = std::make_unique<StreckensegmentItem>(elementRichtung, istSegmentStart, 0.0f, setzeDarstellung_Geschwindigkeit, nullptr);
                        // Fuer Zusi-3-Strecken wird jedes Segment doppelt gefunden (einmal von jedem Ende).
                        // Behalte nur die Segmente, deren Endelement eine groessere Nummer hat als das Startelement.
                        // (Fuer 1-Element-Segmente behalte dasjenige, das in Normrichtung beginnt).
                        // Achtung: Manche Visualisierungen (z.B. Geschwindigkeit) brauchen beide Segmente!
                        if (istZusi2 || endeNr > startNr || (endeNr == startNr && elementRichtung.richtung == Streckenelement::RICHTUNG_NORM))
                        {
                            // Zusi 3: x = Ost, y = Nord
                            item->moveBy(1000 * (strecke->utmPunkt.we - this->m_utmRefPunkt.we), 1000 * (strecke->utmPunkt.ns - this->m_utmRefPunkt.ns));
                            this->addItem(item.release());
                            anzahlSegmente++;
                        }
                    }

                    // Signale
                    auto& signal = elementRichtung.richtungsInfo().signal;

                    if (signal && !signal->signalbezeichnung.empty() &&
                            (istZusi2 || (signal->signaltyp != SignalTyp::Weiche
                            && signal->signaltyp != SignalTyp::Unbestimmt
                            && signal->signaltyp != SignalTyp::Sonstiges
                            && signal->signaltyp != SignalTyp::Bahnuebergang))) {
                        qreal phi = QLineF(elementRichtung.gegenrichtung().endpunkt().x, elementRichtung.gegenrichtung().endpunkt().y,
                                         elementRichtung.endpunkt().x, elementRichtung.endpunkt().y).angle();
                        QColor farbe = Qt::red;
                        switch (signal->signaltyp) {
                            case SignalTyp::Vorsignal:
                                farbe = Qt::darkGreen;
                                break;
                            case SignalTyp::Gleissperre:
                            case SignalTyp::Rangiersignal:
                                farbe = Qt::blue;
                                break;
                            default:
                                break;
                        }

                        auto si = unique_ptr<DreieckItem>(new DreieckItem(phi, QString::fromUtf8(signal->signalbezeichnung.data(), signal->signalbezeichnung.size()), farbe));
                        QPointF pos(elementRichtung.endpunkt().x, elementRichtung.endpunkt().y);
                        si->setPos(pos);
                        si->moveBy(1000 * (strecke->utmPunkt.we - this->m_utmRefPunkt.we), 1000 * (strecke->utmPunkt.ns - this->m_utmRefPunkt.ns));
                        this->addItem(si.release());

                        if (signal->signaltyp != SignalTyp::Vorsignal && !signal->betriebsstelle.empty())
                        {
                            if (betriebsstellenKoordinaten.find(signal->betriebsstelle) == betriebsstellenKoordinaten.end())
                            {
                                betriebsstellenKoordinaten[signal->betriebsstelle] = QRectF(pos, pos);
                            }
                            else
                            {
                                QRectF& r = betriebsstellenKoordinaten[signal->betriebsstelle];
                                // TODO somehow QRect::unite does not work
                                if (pos.x() < r.left()) { r.setLeft(pos.x()); }
                                if (pos.x() > r.right()) { r.setRight(pos.x()); }
                                if (pos.y() > r.bottom()) { r.setBottom(pos.y()); }
                                if (pos.y() < r.top()) { r.setTop(pos.y()); }
                            }
                        }
                    }
                }
            }
        }

        for (const auto& p : betriebsstellenKoordinaten) {
            std::string betriebsstelle = p.first;

#if 0
            auto ri = this->addRect(p.second);
            ri->moveBy(1000 * (strecke->utmPunkt.we - this->m_utmRefPunkt.we), 1000 * (strecke->utmPunkt.ns - this->m_utmRefPunkt.ns));
#endif

            auto ti = unique_ptr<Label>(new Label(QString::fromUtf8(betriebsstelle.data(), betriebsstelle.size())));
            ti->setAlignment(Qt::AlignCenter | Qt::AlignHCenter);
            ti->setPos((p.second.left() + p.second.right()) / 2.0, (p.second.top() + p.second.bottom()) / 2.0);
            ti->moveBy(1000 * (strecke->utmPunkt.we - this->m_utmRefPunkt.we), 1000 * (strecke->utmPunkt.ns - this->m_utmRefPunkt.ns));
            ti->setFlag(QGraphicsItem::ItemIgnoresTransformations);
            ti->setPen(QPen(Qt::black));
            ti->setBrush(QBrush(Qt::black));
            this->addItem(ti.release());
        }
    }

    // TODO: Kreise ohne jegliche Weichen werden nicht als Segmente erkannt.

    qDebug() << anzahlSegmente << "Segmente für" << anzahlStreckenelemente << "Streckenelemente";
}
