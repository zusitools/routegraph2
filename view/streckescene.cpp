#include "streckescene.h"

#include "model/streckenelement.hpp"

#include "view/graphicsitems/streckensegmentitem.h"
#include "view/graphicsitems/dreieckitem.h"

#include <QDebug>

#include <cassert>
#include <cmath>

class Segmentierer
{
protected:
    virtual bool istSegmentGrenze(const StreckenelementUndRichtung &vorgaenger, const StreckenelementUndRichtung &nachfolger)
    {
        return false;
    }

    /* Segment-Startelemente sind Elemente, die nicht der erste Nachfolger ihres Vorgaengers sind
     * oder keinen Vorgaenger haben. Zusaetzlich kann ein beliebiger Callback angegeben werden,
     * der fuer zwei aufeinanderfolgende Elemente entscheidet, ob sie getrennt werden sollen, weil sie
     * sich in einer relevanten Eigenschaft unterscheiden. */
    virtual bool istSegmentStart(const StreckenelementUndRichtung &elementUndRichtung)
    {
        if (!elementUndRichtung.hatVorgaenger())
        {
            return true;
        }

        auto vorgaenger = elementUndRichtung.vorgaenger();
        return !vorgaenger.hatNachfolger() ||
                (vorgaenger.nachfolger(0) != elementUndRichtung) ||
                istSegmentGrenze(vorgaenger, elementUndRichtung);
    }

public:
    bool operator()(const StreckenelementUndRichtung &elementUndRichtung) {
        return this->istSegmentStart(elementUndRichtung);
    }
};

class RichtungsInfoSegmentierer : public Segmentierer
{
protected:
    virtual bool istSegmentGrenze(const StreckenelementRichtungsInfo &vorgaenger, const StreckenelementRichtungsInfo &nachfolger) = 0;

    virtual bool istSegmentGrenze(const StreckenelementUndRichtung &vorgaenger, const StreckenelementUndRichtung &nachfolger) override
    {
        return istSegmentGrenze(vorgaenger.richtungsInfo(), nachfolger.richtungsInfo()) ||
                istSegmentGrenze(vorgaenger.gegenrichtung().richtungsInfo(), nachfolger.gegenrichtung().richtungsInfo());
    }
};

class GleisfunktionSegmentierer : public Segmentierer
{
protected:
    virtual bool istSegmentGrenze(const StreckenelementUndRichtung &vorgaenger, const StreckenelementUndRichtung &nachfolger) override
    {
        return vorgaenger->hatFktFlag(StreckenelementFlag::KeineGleisfunktion) !=
                nachfolger->hatFktFlag(StreckenelementFlag::KeineGleisfunktion);
    }
};

class GeschwindigkeitSegmentierer : public RichtungsInfoSegmentierer
{
protected:
    virtual bool istSegmentGrenze(const StreckenelementRichtungsInfo &vorgaenger, const StreckenelementRichtungsInfo &nachfolger) override
    {
        return vorgaenger.vmax != nachfolger.vmax;
    }
};

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
        int kmh_div10 = std::round(geschwindigkeit * 3.6) / 10 - 1;
        if (kmh_div10 < 0) kmh_div10 = 0;
        if (kmh_div10 > 16) kmh_div10 = 16;
        // <= 10 km/h -> hue 300
        // ...
        // >= 170 km/h -> hue 0
        int hue = 300 - (300.0/16.0) * kmh_div10;
        pen.setColor(QColor::fromHsv(hue, 255, 255));
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

    GeschwindigkeitSegmentierer segmentierer;

    for (unique_ptr<Strecke> &strecke : strecken)
    {
        bool istZusi2 = strecke->dateiInfo->formatVersion[0] == '2';
        auto richtungen = { Streckenelement::RICHTUNG_NORM, Streckenelement::RICHTUNG_GEGEN };
        if (istZusi2) richtungen = {Streckenelement::RICHTUNG_NORM };

        for (auto& streckenelement : strecke->streckenelemente)
        {
            if (streckenelement)
            {
                anzahlStreckenelemente++;
                for (auto richtung : richtungen)
                {
                    // Streckenelement-Segmente
                    if (segmentierer(streckenelement->richtung(richtung)))
                    {
                        auto item = new StreckensegmentItem(
                                    streckenelement->richtung(richtung),
                                    segmentierer, setzeDarstellung_Geschwindigkeit, nullptr);
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

                    if (signal && !signal->signalbezeichnung.empty() &&
                            (istZusi2 || (signal->signaltyp != SignalTyp::Weiche
                            && signal->signaltyp != SignalTyp::Unbestimmt
                            && signal->signaltyp != SignalTyp::Sonstiges))) {
                        qreal phi;
                        if (richtung == Streckenelement::RICHTUNG_NORM)
                        {
                            phi = QLineF(streckenelement->p1.x, streckenelement->p1.y, streckenelement->p2.x, streckenelement->p2.y).angle();
                        }
                        else
                        {
                            phi = QLineF(streckenelement->p2.x, streckenelement->p2.y, streckenelement->p1.x, streckenelement->p1.y).angle();
                        }

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

                        auto si = unique_ptr<DreieckItem>(new DreieckItem(phi, QString::fromStdString(signal->signalbezeichnung), farbe));
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

    // TODO: Kreise ohne jegliche Weichen werden nicht als Segmente erkannt.

    qDebug() << anzahlSegmente << "Segmente für" << anzahlStreckenelemente << "Streckenelemente";
}
