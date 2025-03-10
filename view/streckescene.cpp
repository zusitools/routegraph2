#include "streckescene.h"

#include "streckennetz.h"

#include "model/streckenelement.h"

#include "view/graphicsitems/streckensegmentitem.h"
#include "view/graphicsitems/dreieckitem.h"
#include "view/segmentierer.h"

#include <utm/utm.h>

#include <QDebug>

#include <cassert>
#include <cmath>
#include <optional>
#include <unordered_map>

namespace {
    Vec3 konvertiereUtmZone(const Vec3& in, const UTM& src, const UTM& dest) {
        auto destZone = static_cast<int>(dest.UTM_Zone);

        double lat, lon;
        utm_to_lat_lon(
            in.X + 1000.0 * src.UTM_WE,
            in.Y + 1000.0 * src.UTM_NS,
            static_cast<int>(src.UTM_Zone),
            /*southhemi=*/0,
            &lat,
            &lon);

        double easting, northing;
        lat_lon_to_utm(
            lat,
            lon,
            &destZone,
            &easting,
            &northing);

        return {
            static_cast<decltype(Vec3::X)>(easting) - static_cast<decltype(Vec3::X)>(1000) * dest.UTM_WE,
            static_cast<decltype(Vec3::Y)>(northing) - static_cast<decltype(Vec3::Y)>(1000) * dest.UTM_NS,
            in.Z
        };
    }
}

StreckeScene::StreckeScene(const Streckennetz& streckennetz, Visualisierung& visualisierung, bool zeigeBetriebsstellen, QObject *parent)
    : QGraphicsScene(parent)
    , m_utmRefPunkt{}
{
    size_t anzahlSegmente = 0, anzahlStreckenelemente = 0;

    // Berechne Bounding-Rect der Szene aus den Koordinaten der Streckenelemente (plus etwas konstantem Puffer)
    // Das genuegt als Annaeherung und spart das aufwaendige, detaillierte Berechnen des Bounding-Rects durch Qt.
    float minX = std::numeric_limits<float>::max();
    float minY = std::numeric_limits<float>::max();
    float maxX = std::numeric_limits<float>::min();
    float maxY = std::numeric_limits<float>::min();

    const auto& begin = streckennetz.cbegin();
    const auto& end = streckennetz.cend();

    // Transformiere Strecken in einheitliche UTM-Zone.
    size_t anzahlStreckenMitUtmPunkt = 0;
    for (auto it = begin; it != end; ++it)
    {
        const auto& strecke = it->second;
        if (!strecke->UTM) {
            continue;
        }

        ++anzahlStreckenMitUtmPunkt;
        if (this->m_utmRefPunkt.UTM_Zone == 0) {
            this->m_utmRefPunkt.UTM_Zone = strecke->UTM->UTM_Zone;
        } else if (strecke->UTM->UTM_Zone != this->m_utmRefPunkt.UTM_Zone) {
            qDebug() << "Transformiere Strecke von Zone" << strecke->UTM->UTM_Zone << "nach" << this->m_utmRefPunkt.UTM_Zone;

            const Vec3 utmRefPunktKonvertiert = konvertiereUtmZone(Vec3 {0, 0, 0}, *strecke->UTM, this->m_utmRefPunkt);
            const UTM utmNeu {
                static_cast<decltype(UTM::UTM_WE)>(utmRefPunktKonvertiert.X / 1000),
                static_cast<decltype(UTM::UTM_NS)>(utmRefPunktKonvertiert.Y / 1000),
                this->m_utmRefPunkt.UTM_Zone,
            };

            for (auto& streckenelement : strecke->children_StrElement)
            {
                if (!streckenelement) {
                    continue;
                }

                streckenelement->g = konvertiereUtmZone(streckenelement->g, *strecke->UTM, utmNeu);
                streckenelement->b = konvertiereUtmZone(streckenelement->b, *strecke->UTM, utmNeu);
            }

            *strecke->UTM = utmNeu;
        }
    }

    // Berechne UTM-Referenzpunkt als Mittelwert der Strecken-Referenzpunkte
    double utmRefWe = 0.0;
    double utmRefNs = 0.0;

    for (auto it = begin; it != end; ++it)
    {
        const auto& strecke = it->second;
        if (strecke->UTM) {
            utmRefWe += strecke->UTM->UTM_WE / static_cast<double>(anzahlStreckenMitUtmPunkt);
            utmRefNs += strecke->UTM->UTM_NS / static_cast<double>(anzahlStreckenMitUtmPunkt);
        }
    }
    this->m_utmRefPunkt.UTM_WE = static_cast<int>(utmRefWe);
    this->m_utmRefPunkt.UTM_NS = static_cast<int>(utmRefNs);

    std::unique_ptr<Segmentierer> segmentierer = visualisierung.segmentierer();
    const auto richtungen_zusi2 = { StreckenelementRichtung::Norm };
    const auto richtungen_zusi3 = { StreckenelementRichtung::Norm, StreckenelementRichtung::Gegen };

    for (auto it = begin; it != end; ++it)
    {
        const auto& strecke = it->second;
        const UTM strecke_utm = (strecke->UTM ? *strecke->UTM : UTM());
        const auto utm_dx = 1000 * (strecke_utm.UTM_WE - this->m_utmRefPunkt.UTM_WE);
        const auto utm_dy = 1000 * (strecke_utm.UTM_NS - this->m_utmRefPunkt.UTM_NS);

        // Die Betriebsstellen werden pro Streckenmodul beschriftet, da manche Betriebsstellennamen
        // (z.B. Sbk-Bezeichnungen) in mehreren Modulen vorkommen und dann falsch platziert wuerden.
        std::unordered_map<std::string, QRectF> betriebsstellenKoordinaten;

        bool istZusi2 = false;
        auto richtungen = istZusi2 ? richtungen_zusi2 : richtungen_zusi3;
        float offset = (segmentierer->beideRichtungen() || istZusi2) ? 0.49f : 0.0f;

        for (const auto& streckenelement : strecke->children_StrElement)
        {
            if (streckenelement)
            {
                minX = std::min(minX, streckenelement->g.X + utm_dx);
                minX = std::min(minX, streckenelement->b.X + utm_dx);
                maxX = std::max(maxX, streckenelement->g.X + utm_dx);
                maxX = std::max(maxX, streckenelement->b.X + utm_dx);
                minY = std::min(minY, streckenelement->g.Y + utm_dy);
                minY = std::min(minY, streckenelement->b.Y + utm_dy);
                maxY = std::max(maxY, streckenelement->g.Y + utm_dy);
                maxY = std::max(maxY, streckenelement->b.Y + utm_dy);

                anzahlStreckenelemente++;
                for (StreckenelementRichtung richtung : richtungen)
                {
                    StreckenelementUndRichtung elementRichtung { streckenelement.get(), richtung }; // streckenelement->richtung(richtung);
                    // Streckenelement-Segmente
                    if (segmentierer->istSegmentStart(elementRichtung))
                    {
                        auto item = std::make_unique<StreckensegmentItem>(elementRichtung, *segmentierer, offset, nullptr);
                        auto startNr = streckenelement->Nr;
                        auto endeNr = item->ende()->Nr;

                        // Fuer Zusi-3-Strecken wird jedes Segment doppelt gefunden (einmal von jedem Ende).
                        // Manche Visualisierungen sind nicht richtungsspezifisch und brauchen daher nur eines davon.
                        // Behalte nur die Segmente, deren Endelement eine groessere Nummer hat als das Startelement.
                        // (Fuer 1-Element-Segmente behalte dasjenige, das in Normrichtung beginnt).
                        if (istZusi2 || segmentierer->beideRichtungen() || endeNr > startNr ||
                                (endeNr == startNr && elementRichtung.getRichtung() == StreckenelementRichtung::Norm))
                        {
                            // Zusi 3: x = Ost, y = Nord
                            visualisierung.setzeDarstellung(*item);
                            item->moveBy(utm_dx, utm_dy);
                            this->addItem(item.release());
                            anzahlSegmente++;
                        }
                    }

                    // Signale
                    const auto& richtungsInfo = elementRichtung.richtungsInfo();
                    if (richtungsInfo.has_value()) {
                        const auto& signal = richtungsInfo->Signal;

                        if (signal && !signal->Signalname.empty() &&
                                (istZusi2 || (static_cast<SignalTyp>(signal->SignalTyp) != SignalTyp::Weiche
                                && static_cast<SignalTyp>(signal->SignalTyp) != SignalTyp::Unbestimmt
                                && static_cast<SignalTyp>(signal->SignalTyp) != SignalTyp::Sonstiges
                                && static_cast<SignalTyp>(signal->SignalTyp) != SignalTyp::Bahnuebergang))) {
                            Vec3 vec = elementRichtung.endpunkt() - elementRichtung.gegenrichtung().endpunkt();
                            float phi = atan2(-vec.Y, vec.X);
                            QColor farbe = Qt::red;
                            switch (static_cast<SignalTyp>(signal->SignalTyp)) {
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

                            auto si = std::make_unique<DreieckItem>(phi, QString::fromUtf8(signal->Signalname.data(), signal->Signalname.size()), farbe);
                            string tooltip = signal->NameBetriebsstelle + " " + signal->Signalname;
                            if (!signal->Stellwerk.empty()) {
                                tooltip += "\n[" + signal->Stellwerk + "]";
                            }
                            si->setToolTip(QString::fromUtf8(tooltip.data(), tooltip.size()));
                            QPointF pos(elementRichtung.endpunkt().X, elementRichtung.endpunkt().Y);
                            si->setPos(pos);
                            si->moveBy(utm_dx, utm_dy);
                            this->addItem(si.release());

                            if (zeigeBetriebsstellen && (static_cast<SignalTyp>(signal->SignalTyp) != SignalTyp::Vorsignal) && !signal->NameBetriebsstelle.empty())
                            {
                                if (betriebsstellenKoordinaten.find(signal->NameBetriebsstelle) == betriebsstellenKoordinaten.end())
                                {
                                    betriebsstellenKoordinaten[signal->NameBetriebsstelle] = QRectF(pos, pos);
                                }
                                else
                                {
                                    QRectF& r = betriebsstellenKoordinaten[signal->NameBetriebsstelle];
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
        }

        const auto elementRichtung = [&strecke](const ReferenzElement& refpunkt) -> StreckenelementUndRichtung {
            if ((refpunkt.StrElement < 0) || (static_cast<size_t>(refpunkt.StrElement) >= strecke->children_StrElement.size())) {
                return { nullptr, static_cast<StreckenelementRichtung>(refpunkt.StrNorm) };
            } else {
                return { strecke->children_StrElement[refpunkt.StrElement].get(), static_cast<StreckenelementRichtung>(refpunkt.StrNorm) };
            }
        }; // TODO refpunkt->elementRichtung()

        for (const auto& refpunkt : strecke->children_ReferenzElemente)
        {
            if (!refpunkt || !elementRichtung(*refpunkt).getStreckenelement()) continue;

            if (static_cast<ReferenzpunktTyp>(refpunkt->RefTyp) == ReferenzpunktTyp::Aufgleispunkt) {
                Vec3 vec = elementRichtung(*refpunkt).endpunkt() - elementRichtung(*refpunkt).gegenrichtung().endpunkt();
                qreal phi = atan2(-vec.Y, vec.X);

                assert(refpunkt->Info.data() != nullptr);
                auto si = std::make_unique<DreieckItem>(phi,
                        QString::fromUtf8(refpunkt->Info.data(), refpunkt->Info.size()), Qt::magenta);
                si->setPos(elementRichtung(*refpunkt).endpunkt().X, elementRichtung(*refpunkt).endpunkt().Y);
                si->moveBy(utm_dx, utm_dy);
                this->addItem(si.release());
            }
        }

        for (const auto& p : betriebsstellenKoordinaten) {
            std::string betriebsstelle = p.first;

#if 0
            auto ri = this->addRect(p.second);
            ri->moveBy(1000 * (strecke->utmPunkt.UTM_WE - this->m_utmRefPunkt.UTM_WE), 1000 * (strecke->utmPunkt.UTM_NS - this->m_utmRefPunkt.UTM_NS));
#endif

            auto ti = std::unique_ptr<Label>(new Label(QString::fromUtf8(betriebsstelle.data(), betriebsstelle.size())));
            ti->setAlignment(Qt::AlignCenter | Qt::AlignHCenter);
            ti->setPos((p.second.left() + p.second.right()) / 2.0, (p.second.top() + p.second.bottom()) / 2.0);
            ti->moveBy(utm_dx, utm_dy);
            ti->setFlag(QGraphicsItem::ItemIgnoresTransformations);
            ti->setPen(QPen(Qt::black));
            ti->setBrush(QBrush(Qt::black));
            this->addItem(ti.release());
        }
    }

    // TODO: Kreise ohne jegliche Weichen werden nicht als Segmente erkannt.

    qDebug() << anzahlSegmente << "Segmente für" << anzahlStreckenelemente << "Streckenelemente";

    this->setSceneRect(QRectF(QPointF(std::min(minX, maxX) - 10, std::min(minY, maxY) - 10), QPointF(std::max(minX, maxX) + 10, std::max(minY, maxY) + 10)));
}
