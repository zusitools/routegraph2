#include "streckescene.h"

#include "view/graphicsitems/streckensegmentitem.h"
#include "view/graphicsitems/dreieckitem.h"

#include <QDebug>

bool istSegmentStart(const StreckenelementUndRichtung &elementUndRichtung)
{
    if (!elementUndRichtung.hatVorgaenger())
    {
        return true;
    }

    auto vorgaenger = elementUndRichtung.vorgaenger();
    if (!vorgaenger.hatNachfolger())
    {
        return true;
    }

    if (vorgaenger->hatFktFlag(StreckenelementFlag::KeineGleisfunktion) !=
            elementUndRichtung->hatFktFlag(StreckenelementFlag::KeineGleisfunktion)) {
        return true;
    }

    return &*vorgaenger.nachfolger() != &*elementUndRichtung;
}

void setzeDarstellung(StreckensegmentItem &item, const StreckenelementUndRichtung &start)
{
    QPen pen = item.pen();
    if (start->hatFktFlag(StreckenelementFlag::KeineGleisfunktion))
    {
        pen.setColor(Qt::lightGray);
    } else {
        pen.setColor(Qt::black);
    }
    item.setPen(pen);
}

StreckeScene::StreckeScene(vector<reference_wrapper<unique_ptr<Strecke> > > strecken, QObject *parent) :
    QGraphicsScene(parent)
{
    this->setItemIndexMethod(QGraphicsScene::NoIndex);
    auto anzahlSegmente = 0;

    // Berechne UTM-Referenzpunkt als Mittelwert der Strecken-Referenzpunkte
    double utmRefWe = 0.0d;
    double utmRefNs = 0.0d;
    for (unique_ptr<Strecke> &strecke : strecken)
    {
        utmRefWe += strecke->utmPunkt.we / float(strecken.size());
        utmRefNs += strecke->utmPunkt.ns / float(strecken.size());
    }
    this->m_utmRefPunkt.we = int(utmRefWe);
    this->m_utmRefPunkt.ns = int(utmRefNs);

    for (unique_ptr<Strecke> &strecke : strecken)
    {
        bool istZusi2 = strecke->dateiInfo->formatVersion[0] == '2';
        auto richtungen = { Streckenelement::RICHTUNG_NORM, Streckenelement::RICHTUNG_GEGEN };
        if (istZusi2) richtungen = {Streckenelement::RICHTUNG_NORM };

        for (auto& streckenelement : strecke->streckenelemente)
        {
            if (streckenelement)
            {
                for (auto richtung : richtungen)
                {
                    if (istSegmentStart(streckenelement->richtung(richtung)))
                    {
                        auto item = new StreckensegmentItem(
                                    streckenelement->richtung(richtung),
                                    istSegmentStart, setzeDarstellung, nullptr);
                        auto startNr = streckenelement->nr;
                        auto endeNr = item->ende->nr;
                        // Fuer Zusi-3-Strecken wird jedes Segment doppelt gefunden (einmal von jedem Ende).
                        // Behalte nur die Segmente, deren Endelement eine groessere Nummer hat als das Startelement.
                        // (Fuer 1-Element-Segmente behalte dasjenige, das in Normrichtung beginnt).
                        if (!istZusi2 && (endeNr < startNr || (endeNr == startNr && richtung == Streckenelement::RICHTUNG_GEGEN)))
                        {
                            delete item;
                        }
                        else
                        {
                            // Zusi 3: x = Ost, y = Nord
                            item->moveBy(1000 * (strecke->utmPunkt.we - this->m_utmRefPunkt.we), 1000 * (strecke->utmPunkt.ns - this->m_utmRefPunkt.ns));
                            this->addItem(item);
                            anzahlSegmente++;

#ifdef DEBUG_SEGMENTE
                            auto ti = this->addText(QString::number(streckenelement->nr) + " Start");
                            if (richtung == Streckenelement::RICHTUNG_NORM)
                                ti->setPos(streckenelement->p1.x, streckenelement->p1.y);
                            else
                                ti->setPos(streckenelement->p2.x, streckenelement->p2.y);
                            ti->moveBy(1000 * (strecke->utmPunkt.we - this->m_utmRefPunkt.we), 1000 * (strecke->utmPunkt.ns - this->m_utmRefPunkt.ns));
                            ti->setTransform(QTransform().scale(-1, 1).rotate(180));
                            ti->setDefaultTextColor(item->pen().color());

                            ti = this->addText(QString::number(streckenelement->nr) + " Ende");
                            if (item->ende.richtung == Streckenelement::RICHTUNG_NORM)
                                ti->setPos(item->ende->p2.x, item->ende->p2.y);
                            else
                                ti->setPos(item->ende->p1.x, item->ende->p1.y);
                            ti->moveBy(1000 * (strecke->utmPunkt.we - this->m_utmRefPunkt.we), 1000 * (strecke->utmPunkt.ns - this->m_utmRefPunkt.ns));
                            ti->setTransform(QTransform().scale(-1, 1).rotate(180));
                            ti->setDefaultTextColor(item->pen().color());
#endif
                        }
                    }

                    auto& signal = streckenelement->richtungsInfo[richtung].signal;

                    if (signal) {
                        qreal phi;
                        if (richtung == Streckenelement::RICHTUNG_NORM)
                        {
                            phi = QLineF(streckenelement->p1.x, streckenelement->p1.y, streckenelement->p2.x, streckenelement->p2.y).angle();
                        }
                        else
                        {
                            phi = QLineF(streckenelement->p2.x, streckenelement->p2.y, streckenelement->p1.x, streckenelement->p1.y).angle();
                        }

                        auto si = unique_ptr<DreieckItem>(new DreieckItem(phi, QString::fromStdString(signal->signalbezeichnung), Qt::red));
                        if (richtung == Streckenelement::RICHTUNG_NORM)
                        {
                            si->setPos(streckenelement->p2.x, streckenelement->p2.y);
                        }
                        else
                        {
                            si->setPos(streckenelement->p1.x, streckenelement->p1.y);
                        }

                        si->moveBy(1000 * (strecke->utmPunkt.we - this->m_utmRefPunkt.we), 1000 * (strecke->utmPunkt.ns - this->m_utmRefPunkt.ns));
                        this->addItem(si.release());
                    }
                }
            }
        }
    }

    qDebug() << anzahlSegmente << "Segmente";
}
