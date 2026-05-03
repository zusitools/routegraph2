#include "streckescene.h"

#include "streckennetz.h"

#include "model/streckenelement.h"

#include "view/bahnsteige.h"
#include "view/graphicsitems/streckensegmentitem.h"
#include "view/graphicsitems/dreieckitem.h"
#include "view/segmentierer.h"
#include "view/zwerte.h"

#include <utm/utm.h>

#include <QBrush>
#include <QDebug>
#include <QGraphicsEllipseItem>
#include <QGraphicsPathItem>
#include <QPainterPath>
#include <QPen>

#include <cassert>
#include <cmath>
#include <optional>
#include <unordered_map>

namespace {
    Vec3 konvertiereUtmZone(const Vec3& in, const UTM& src, const UTM& dest) {
        auto destZone = static_cast<int>(dest.UTM_Zone);

        double lat, lon;
        utm_to_lat_lon(
            in.x + 1000.0 * src.UTM_WE,
            in.y + 1000.0 * src.UTM_NS,
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
            static_cast<decltype(Vec3::x)>(easting) - static_cast<decltype(Vec3::x)>(1000) * dest.UTM_WE,
            static_cast<decltype(Vec3::y)>(northing) - static_cast<decltype(Vec3::y)>(1000) * dest.UTM_NS,
            in.z
        };
    }
}

StreckeScene::StreckeScene(const Streckennetz& streckennetz, Visualisierung& visualisierung, bool zeigeBetriebsstellen, bool zeigeBahnsteige, QObject *parent)
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
                static_cast<decltype(UTM::UTM_WE)>(utmRefPunktKonvertiert.x / 1000),
                static_cast<decltype(UTM::UTM_NS)>(utmRefPunktKonvertiert.y / 1000),
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

    // Pro Streckenelement: UTM-Versatz seiner Strecke gegenüber dem Szenen-Referenzpunkt.
    // Wird sowohl für die Darstellung von modulübergreifenden Bahnsteigen als auch für
    // die Hervorhebung von Fahrstraßen benötigt.
    for (auto it = begin; it != end; ++it) {
        const auto& strecke = it->second;
        const UTM strecke_utm = (strecke->UTM ? *strecke->UTM : UTM());
        const auto dx = 1000.0 * (strecke_utm.UTM_WE - this->m_utmRefPunkt.UTM_WE);
        const auto dy = 1000.0 * (strecke_utm.UTM_NS - this->m_utmRefPunkt.UTM_NS);
        for (const auto& streckenelement : strecke->children_StrElement) {
            if (streckenelement) {
                this->m_elementToOffset.emplace(streckenelement.get(), std::make_pair(dx, dy));
            }
        }
    }

    for (auto it = begin; it != end; ++it)
    {
        const auto& strecke_pfad = zusixml::ZusiPfad::vonZusiPfad(it->first);
        const auto& strecke = it->second;
        const UTM strecke_utm = (strecke->UTM ? *strecke->UTM : UTM());
        const auto utm_dx = 1000 * (strecke_utm.UTM_WE - this->m_utmRefPunkt.UTM_WE);
        const auto utm_dy = 1000 * (strecke_utm.UTM_NS - this->m_utmRefPunkt.UTM_NS);

        // Modulgrenzen ermitteln: für jedes Streckenelement mit Nachfolgern im
        // Nachbarmodul den jeweiligen Endpunkt in Szenen-Koordinaten merken.
        for (const auto& streckenelement : strecke->children_StrElement) {
            if (!streckenelement) {
                continue;
            }
            for (const auto& nachfolger : streckenelement->children_NachNormModul) {
                this->m_modulgrenzen.push_back({
                    QPointF(streckenelement->b.x + utm_dx, streckenelement->b.y + utm_dy),
                    zusixml::ZusiPfad::vonZusiPfad(nachfolger.Datei.Dateiname, strecke_pfad)
                });
            }
            for (const auto& nachfolger : streckenelement->children_NachGegenModul) {
                this->m_modulgrenzen.push_back({
                    QPointF(streckenelement->g.x + utm_dx, streckenelement->g.y + utm_dy),
                    zusixml::ZusiPfad::vonZusiPfad(nachfolger.Datei.Dateiname, strecke_pfad)
                });
            }
        }

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
                minX = std::min(minX, streckenelement->g.x + utm_dx);
                minX = std::min(minX, streckenelement->b.x + utm_dx);
                maxX = std::max(maxX, streckenelement->g.x + utm_dx);
                maxX = std::max(maxX, streckenelement->b.x + utm_dx);
                minY = std::min(minY, streckenelement->g.y + utm_dy);
                minY = std::min(minY, streckenelement->b.y + utm_dy);
                maxY = std::max(maxY, streckenelement->g.y + utm_dy);
                maxY = std::max(maxY, streckenelement->b.y + utm_dy);

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
                            float phi = atan2(-vec.y, vec.x);
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
                            QPointF pos(elementRichtung.endpunkt().x, elementRichtung.endpunkt().y);
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
                qreal phi = atan2(-vec.y, vec.x);

                assert(refpunkt->Info.data() != nullptr);
                auto si = std::make_unique<DreieckItem>(phi,
                        QString::fromUtf8(refpunkt->Info.data(), refpunkt->Info.size()), Qt::magenta);
                si->setPos(elementRichtung(*refpunkt).endpunkt().x, elementRichtung(*refpunkt).endpunkt().y);
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

        if (zeigeBahnsteige) {
            zeichneBahnsteige(*this, *strecke, this->m_elementToOffset);
        }
    }

    // TODO: Kreise ohne jegliche Weichen werden nicht als Segmente erkannt.

    qDebug() << anzahlSegmente << "Segmente für" << anzahlStreckenelemente << "Streckenelemente";

    this->setSceneRect(QRectF(QPointF(std::min(minX, maxX) - 10, std::min(minY, maxY) - 10), QPointF(std::max(minX, maxX) + 10, std::max(minY, maxY) + 10)));
}

void StreckeScene::zeigeFahrstrasse(const std::vector<StreckenelementUndRichtung>& pfad)
{
    this->verbirgFahrstrasse();
    if (pfad.empty()) {
        return;
    }

    QPainterPath path;
    bool angefangen = false;

    for (const auto& er : pfad) {
        const auto* el = er.getStreckenelement();
        if (!el) {
            continue;
        }
        const auto offsetIt = this->m_elementToOffset.find(el);
        if (offsetIt == this->m_elementToOffset.end()) {
            continue;
        }
        const auto& [dx, dy] = offsetIt->second;
        const auto& start = er.gegenrichtung().endpunkt();
        const auto& ende = er.endpunkt();

        if (!angefangen) {
            path.moveTo(start.x + dx, start.y + dy);
            angefangen = true;
        } else {
            path.lineTo(start.x + dx, start.y + dy);
        }
        path.lineTo(ende.x + dx, ende.y + dy);
    }

    if (!angefangen) {
        return;
    }

    auto* item = new QGraphicsPathItem(path);
    QPen pen(QColor(255, 200, 0, 200));
    pen.setWidth(8);
    pen.setCosmetic(true);
    pen.setCapStyle(Qt::FlatCap);
    pen.setJoinStyle(Qt::RoundJoin);
    item->setPen(pen);
    // Über den Gleisen, aber unter den Signal-Dreiecken (ZWERT_MARKIERUNG = 101).
    item->setZValue(ZWERT_GLEIS + 0.5);
    this->addItem(item);
    this->m_fahrstrasseHighlight = item;
}

void StreckeScene::verbirgFahrstrasse()
{
    if (this->m_fahrstrasseHighlight) {
        this->removeItem(this->m_fahrstrasseHighlight);
        delete this->m_fahrstrasseHighlight;
        this->m_fahrstrasseHighlight = nullptr;
    }
    this->setzeFahrstrassenDetailMarker(std::nullopt);
}

std::optional<QPointF> StreckeScene::punktInSzene(const StreckenelementUndRichtung& er) const
{
    const auto* el = er.getStreckenelement();
    if (!el) {
        return std::nullopt;
    }
    const auto offsetIt = this->m_elementToOffset.find(el);
    if (offsetIt == this->m_elementToOffset.end()) {
        return std::nullopt;
    }
    const auto& [dx, dy] = offsetIt->second;
    const auto& ende = er.endpunkt();
    return QPointF(ende.x + dx, ende.y + dy);
}

void StreckeScene::setzeFahrstrassenDetailMarker(const std::optional<QPointF>& punkt)
{
    if (this->m_detailMarker) {
        this->removeItem(this->m_detailMarker);
        delete this->m_detailMarker;
        this->m_detailMarker = nullptr;
    }
    if (!punkt.has_value()) {
        return;
    }

    // Halbtransparenter roter Kreis. Größe ist in Pixeln (cosmetic), unabhängig vom Zoom.
    constexpr qreal radiusPx = 12.0;
    auto* item = new QGraphicsEllipseItem(QRectF(-radiusPx, -radiusPx, 2 * radiusPx, 2 * radiusPx));
    item->setPos(*punkt);
    item->setBrush(QBrush(QColor(255, 0, 0, 110)));
    QPen pen(QColor(180, 0, 0, 220));
    pen.setWidth(2);
    pen.setCosmetic(true);
    item->setPen(pen);
    item->setFlag(QGraphicsItem::ItemIgnoresTransformations);
    item->setZValue(ZWERT_GLEIS + 0.7);
    this->addItem(item);
    this->m_detailMarker = item;
}

QRectF StreckeScene::boundingRectFuerPfad(const std::vector<StreckenelementUndRichtung>& pfad) const
{
    QRectF result;
    bool ersterPunkt = true;

    const auto erweitere = [&](qreal x, qreal y) {
        if (ersterPunkt) {
            result = QRectF(QPointF(x, y), QPointF(x, y));
            ersterPunkt = false;
        } else {
            if (x < result.left())   result.setLeft(x);
            if (x > result.right())  result.setRight(x);
            if (y < result.top())    result.setTop(y);
            if (y > result.bottom()) result.setBottom(y);
        }
    };

    for (const auto& er : pfad) {
        const auto* el = er.getStreckenelement();
        if (!el) {
            continue;
        }
        const auto offsetIt = this->m_elementToOffset.find(el);
        if (offsetIt == this->m_elementToOffset.end()) {
            continue;
        }
        const auto& [dx, dy] = offsetIt->second;
        const auto& start = er.gegenrichtung().endpunkt();
        const auto& ende = er.endpunkt();
        erweitere(start.x + dx, start.y + dy);
        erweitere(ende.x + dx, ende.y + dy);
    }

    return result;
}

std::vector<StreckeScene::Modulgrenze> StreckeScene::modulgrenzenInUmgebung(const QPointF& szenePunkt, qreal radius) const
{
    std::vector<Modulgrenze> result;
    for (const auto& mg : this->m_modulgrenzen) {
        if (std::abs(mg.szenePunkt.x() - szenePunkt.x()) <= radius &&
                std::abs(mg.szenePunkt.y() - szenePunkt.y()) <= radius) {
            result.push_back(mg);
        }
    }
    return result;
}
