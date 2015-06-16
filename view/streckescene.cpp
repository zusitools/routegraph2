#include "streckescene.h"

#include "view/graphicsitems/streckensegmentitem.h"

#include <QDebug>

bool istSegmentStart(const StreckenelementUndRichtung elementUndRichtung)
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

    return vorgaenger.nachfolger().streckenelement.lock().get() != elementUndRichtung.streckenelement.lock().get();
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

        for (auto streckenelement : strecke->streckenelemente)
        {
            if (streckenelement != nullptr)
            {
                for (auto richtung : richtungen)
                {
                    if (istSegmentStart(StreckenelementUndRichtung(streckenelement, richtung)))
                    {
                        auto item = new StreckensegmentItem(
                                    StreckenelementUndRichtung(streckenelement, richtung),
                                    istSegmentStart, nullptr);
                        auto startNr = streckenelement->nr;
                        auto endeNr = item->ende.streckenelement.lock()->nr;
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

                            // Debug
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
                                ti->setPos(item->ende.streckenelement.lock()->p2.x, item->ende.streckenelement.lock()->p2.y);
                            else
                                ti->setPos(item->ende.streckenelement.lock()->p1.x, item->ende.streckenelement.lock()->p1.y);
                            ti->moveBy(1000 * (strecke->utmPunkt.we - this->m_utmRefPunkt.we), 1000 * (strecke->utmPunkt.ns - this->m_utmRefPunkt.ns));
                            ti->setTransform(QTransform().scale(-1, 1).rotate(180));
                            ti->setDefaultTextColor(item->pen().color());
                        }
                    }
                }
            }
        }
    }

    qDebug() << anzahlSegmente << "Segmente";
}
