#include "streckescene.h"

#include "model/streckenelement.hpp"

#include "view/graphicsitems/streckensegmentitem.h"
#include "view/graphicsitems/dreieckitem.h"
#include "view/segmentierer.h"

#include <QDebug>

#include <cassert>
#include <cmath>

StreckeScene::StreckeScene(const vector<unique_ptr<Strecke>>& strecken, Visualisierung& visualisierung, QObject *parent) :
    QGraphicsScene(parent)
{
    this->setItemIndexMethod(QGraphicsScene::NoIndex);
    size_t anzahlSegmente = 0, anzahlStreckenelemente = 0;

    // Berechne UTM-Referenzpunkt als Mittelwert der Strecken-Referenzpunkte
    double utmRefWe = 0.0;
    double utmRefNs = 0.0;
    for (const unique_ptr<Strecke>& strecke : strecken)
    {
        utmRefWe += strecke->utmPunkt.we / float(strecken.size());
        utmRefNs += strecke->utmPunkt.ns / float(strecken.size());
    }
    this->m_utmRefPunkt.we = int(utmRefWe);
    this->m_utmRefPunkt.ns = int(utmRefNs);

    unique_ptr<Segmentierer> segmentierer = visualisierung.segmentierer();
    const auto richtungen_zusi2 = { Streckenelement::RICHTUNG_NORM };
    const auto richtungen_zusi3 = { Streckenelement::RICHTUNG_NORM, Streckenelement::RICHTUNG_GEGEN };

    for (const unique_ptr<Strecke>& strecke : strecken)
    {
        // Die Betriebsstellen werden pro Streckenmodul beschriftet, da manche Betriebsstellennamen
        // (z.B. Sbk-Bezeichnungen) in mehreren Modulen vorkommen und dann falsch platziert wuerden.
        std::unordered_map<std::string, QRectF> betriebsstellenKoordinaten;

        bool istZusi2 = strecke->formatVersion.size() > 0 && strecke->formatVersion[0] == '2';
        auto richtungen = istZusi2 ? richtungen_zusi2 : richtungen_zusi3;
        float offset = (segmentierer->beideRichtungen() || istZusi2) ? 0.49 : 0.0;

        for (auto& streckenelement : strecke->streckenelemente)
        {
            if (streckenelement)
            {
                anzahlStreckenelemente++;
                for (streckenelement_richtung_t richtung : richtungen)
                {
                    StreckenelementUndRichtung elementRichtung = streckenelement->richtung(richtung);
                    // Streckenelement-Segmente
                    if (segmentierer->istSegmentStart(elementRichtung))
                    {
                        auto item = std::make_unique<StreckensegmentItem>(elementRichtung, *segmentierer, offset, nullptr);
                        streckenelement_nr_t startNr = streckenelement->nr;
                        streckenelement_nr_t endeNr = item->ende()->nr;

                        // Fuer Zusi-3-Strecken wird jedes Segment doppelt gefunden (einmal von jedem Ende).
                        // Manche Visualisierungen sind nicht richtungsspezifisch und brauchen daher nur eines davon.
                        // Behalte nur die Segmente, deren Endelement eine groessere Nummer hat als das Startelement.
                        // (Fuer 1-Element-Segmente behalte dasjenige, das in Normrichtung beginnt).
                        if (istZusi2 || segmentierer->beideRichtungen() || endeNr > startNr ||
                                (endeNr == startNr && elementRichtung.richtung == Streckenelement::RICHTUNG_NORM))
                        {
                            // Zusi 3: x = Ost, y = Nord
                            visualisierung.setzeDarstellung(*item);
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
                        Punkt3D vec = elementRichtung.endpunkt() - elementRichtung.gegenrichtung().endpunkt();
                        float phi = atan2(-vec.y, vec.x);
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

                        auto si = std::make_unique<DreieckItem>(phi, QString::fromUtf8(signal->signalbezeichnung.data(), signal->signalbezeichnung.size()), farbe);
                        string tooltip = signal->betriebsstelle + " " + signal->signalbezeichnung;
                        if (!signal->stellwerk.empty()) {
                            tooltip += "\n[" + signal->stellwerk + "]";
                        }
                        si->setToolTip(QString::fromUtf8(tooltip.data(), tooltip.size()));
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

#if 0
        for (auto& refpunkt : strecke->referenzpunkte)
        {
            if (!refpunkt || !refpunkt->elementRichtung.streckenelement) continue;

            if (refpunkt->referenzTyp == Referenzpunkt::Typ::Aufgleispunkt) {
                Punkt3D vec = refpunkt->elementRichtung.endpunkt() - refpunkt->elementRichtung.gegenrichtung().endpunkt();
                qreal phi = atan2(-vec.y, vec.x);

                assert(refpunkt->beschreibung.data() != NULL);
                auto si = std::make_unique<DreieckItem>(phi,
                        QString::fromUtf8(refpunkt->beschreibung.data(), refpunkt->beschreibung.size()), Qt::magenta);
                si->setPos(refpunkt->elementRichtung.endpunkt().x, refpunkt->elementRichtung.endpunkt().y);
                si->moveBy(1000 * (strecke->utmPunkt.we - this->m_utmRefPunkt.we), 1000 * (strecke->utmPunkt.ns - this->m_utmRefPunkt.ns));
                this->addItem(si.release());
            }
        }
#endif

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
