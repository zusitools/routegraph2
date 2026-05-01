#include "bahnsteige.h"

#include <algorithm>
#include <cmath>
#include <memory>
#include <vector>

#include <QBrush>
#include <QGraphicsEllipseItem>
#include <QGraphicsPolygonItem>
#include <QGraphicsScene>
#include <QPen>
#include <QPointF>
#include <QPolygonF>

#include "model/streckenelement.h"
#include "view/zwerte.h"

#include "zusi_parser/zusi_types.hpp"

namespace {

constexpr qreal kBahnsteigBreite = 5.0;
constexpr qreal kMittePunktDurchmesser = kBahnsteigBreite * 0.8;
constexpr float kMaxLaenge = 1000.0f;

enum class Seite : int {
    Rechts = +1,
    Links  = -1,
};

EreignisTyp anfangFuer(Seite seite) {
    return seite == Seite::Rechts ? EreignisTyp::BahnsteiganfangRechts : EreignisTyp::BahnsteiganfangLinks;
}

EreignisTyp endeFuer(Seite seite) {
    return seite == Seite::Rechts ? EreignisTyp::BahnsteigendeRechts : EreignisTyp::BahnsteigendeLinks;
}

EreignisTyp mitteFuer(Seite seite) {
    return seite == Seite::Rechts ? EreignisTyp::BahnsteigmitteRechts : EreignisTyp::BahnsteigmitteLinks;
}

struct Offset {
    qreal dx { 0.0 };
    qreal dy { 0.0 };
};

Offset offsetVon(
    const StrElement* element,
    const std::unordered_map<const StrElement*, std::pair<qreal, qreal>>& utmOffsetByElement)
{
    const auto it = utmOffsetByElement.find(element);
    if (it == utmOffsetByElement.end()) {
        return {};
    }
    return { it->second.first, it->second.second };
}

float laengeVonElement(const StreckenelementUndRichtung& er) {
    const Vec3 vec = er.endpunkt() - er.gegenrichtung().endpunkt();
    return std::sqrt(vec.X * vec.X + vec.Y * vec.Y);
}

// Liefert den auf Bahnsteigbreite skalierten Senkrechten-Vektor zur Element-Richtung,
// ausgerichtet auf die angegebene Seite.
QPointF perpVek(const Vec3& dir, Seite seite, qreal breite) {
    const float veclen = std::sqrt(dir.X * dir.X + dir.Y * dir.Y);
    if (veclen < 1e-6f) {
        return { 0.0, 0.0 };
    }
    const qreal s = static_cast<qreal>(static_cast<int>(seite));
    return {
        static_cast<qreal>(dir.Y) * breite * s / veclen,
       -static_cast<qreal>(dir.X) * breite * s / veclen,
    };
}

void zeichnePolygon(
    QGraphicsScene& scene,
    const std::vector<StreckenelementUndRichtung>& path,
    Seite seite,
    const std::unordered_map<const StrElement*, std::pair<qreal, qreal>>& utmOffsetByElement)
{
    if (path.empty()) {
        return;
    }

    std::vector<Vec3> dirs(path.size());
    for (size_t i = 0; i < path.size(); ++i) {
        dirs[i] = path[i].endpunkt() - path[i].gegenrichtung().endpunkt();
    }

    std::vector<QPointF> innen;
    std::vector<QPointF> aussen;
    innen.reserve(path.size() + 1);
    aussen.reserve(path.size() + 1);

    // Erster Knotenpunkt: Anfang des ersten Elements
    {
        const auto off = offsetVon(path[0].getStreckenelement(), utmOffsetByElement);
        const auto& p = path[0].gegenrichtung().endpunkt();
        const QPointF perp = perpVek(dirs[0], seite, kBahnsteigBreite);
        innen.emplace_back(p.X + off.dx, p.Y + off.dy);
        aussen.emplace_back(p.X + off.dx + perp.x(), p.Y + off.dy + perp.y());
    }

    // Innere Knotenpunkte: Mittel der angrenzenden Element-Senkrechten,
    // damit der Bahnsteig an Krümmungen/Weichen optisch zusammenhängend bleibt
    for (size_t i = 1; i < path.size(); ++i) {
        const auto off = offsetVon(path[i].getStreckenelement(), utmOffsetByElement);
        const auto& p = path[i].gegenrichtung().endpunkt();
        const QPointF perp1 = perpVek(dirs[i - 1], seite, kBahnsteigBreite);
        const QPointF perp2 = perpVek(dirs[i], seite, kBahnsteigBreite);
        const QPointF perp { (perp1.x() + perp2.x()) / 2.0, (perp1.y() + perp2.y()) / 2.0 };
        innen.emplace_back(p.X + off.dx, p.Y + off.dy);
        aussen.emplace_back(p.X + off.dx + perp.x(), p.Y + off.dy + perp.y());
    }

    // Letzter Knotenpunkt: Ende des letzten Elements
    {
        const auto& last = path.back();
        const auto off = offsetVon(last.getStreckenelement(), utmOffsetByElement);
        const auto& p = last.endpunkt();
        const QPointF perp = perpVek(dirs.back(), seite, kBahnsteigBreite);
        innen.emplace_back(p.X + off.dx, p.Y + off.dy);
        aussen.emplace_back(p.X + off.dx + perp.x(), p.Y + off.dy + perp.y());
    }

    QPolygonF polygon;
    polygon.reserve(static_cast<int>(innen.size() + aussen.size()));
    for (const auto& pt : innen) {
        polygon << pt;
    }
    for (auto it = aussen.rbegin(); it != aussen.rend(); ++it) {
        polygon << *it;
    }

    auto item = std::make_unique<QGraphicsPolygonItem>(polygon);
    item->setBrush(QBrush(Qt::lightGray));
    item->setPen(QPen(Qt::NoPen));
    item->setZValue(ZWERT_GLEIS - 1);
    scene.addItem(item.release());
}

// Tiefensuche: ausgehend von @p anfang werden alle Nachfolger-Pfade verfolgt,
// bis ein Element mit dem zur Seite passenden Bahnsteigende-Ereignis erreicht
// wird. Pfade, die innerhalb von kMaxLaenge kein Ende erreichen, werden verworfen.
void sucheBahnsteig(
    QGraphicsScene& scene,
    const StreckenelementUndRichtung anfang,
    Seite seite,
    const std::unordered_map<const StrElement*, std::pair<qreal, qreal>>& utmOffsetByElement)
{
    const EreignisTyp endeTyp = endeFuer(seite);

    struct Frame {
        StreckenelementUndRichtung element;
        std::vector<StreckenelementUndRichtung> path;
        float laenge;
    };

    std::vector<Frame> stack;
    stack.push_back({ anfang, { anfang }, 0.0f });

    while (!stack.empty()) {
        Frame frame = std::move(stack.back());
        stack.pop_back();

        const auto& cur = frame.element;
        const auto& info = cur.richtungsInfo();
        if (info.has_value() && hatEreignis(*info, endeTyp)) {
            zeichnePolygon(scene, frame.path, seite, utmOffsetByElement);
            continue;
        }

        for (size_t i = 0; i < cur.anzahlNachfolger(); ++i) {
            if (!cur.hatNachfolger(i)) {
                continue;
            }
            const auto next = cur.nachfolger(i);
            if (!next) {
                continue;
            }

            const float newLaenge = frame.laenge + laengeVonElement(next);
            if (newLaenge > kMaxLaenge) {
                continue;
            }

            // Zykel vermeiden
            const bool inPath = std::any_of(frame.path.begin(), frame.path.end(),
                [&next](const StreckenelementUndRichtung& el) { return el == next; });
            if (inPath) {
                continue;
            }

            auto neuerPfad = frame.path;
            neuerPfad.push_back(next);
            stack.push_back({ next, std::move(neuerPfad), newLaenge });
        }
    }
}

void zeichneMitte(
    QGraphicsScene& scene,
    const StreckenelementUndRichtung er,
    Seite seite,
    const std::unordered_map<const StrElement*, std::pair<qreal, qreal>>& utmOffsetByElement)
{
    const auto off = offsetVon(er.getStreckenelement(), utmOffsetByElement);
    // Ereignisse wirken am Ende des Elements: bei Norm-Richtung b, bei Gegen-Richtung g.
    const auto& p = er.endpunkt();
    const Vec3 dir = er.endpunkt() - er.gegenrichtung().endpunkt();
    const QPointF perp = perpVek(dir, seite, kBahnsteigBreite / 2.0);

    const qreal cx = p.X + off.dx + perp.x();
    const qreal cy = p.Y + off.dy + perp.y();
    const qreal r = kMittePunktDurchmesser / 2.0;

    auto item = std::make_unique<QGraphicsEllipseItem>(cx - r, cy - r, kMittePunktDurchmesser, kMittePunktDurchmesser);
    item->setBrush(QBrush(Qt::darkGray));
    item->setPen(QPen(Qt::NoPen));
    item->setZValue(ZWERT_GLEIS);
    scene.addItem(item.release());
}

}  // namespace

void zeichneBahnsteige(
    QGraphicsScene& scene,
    const Strecke& strecke,
    const std::unordered_map<const StrElement*, std::pair<qreal, qreal>>& utmOffsetByElement)
{
    static constexpr StreckenelementRichtung richtungen[] {
        StreckenelementRichtung::Norm,
        StreckenelementRichtung::Gegen,
    };
    static constexpr Seite seiten[] { Seite::Rechts, Seite::Links };

    for (const auto& streckenelementPtr : strecke.children_StrElement) {
        if (!streckenelementPtr) {
            continue;
        }
        const StrElement* streckenelement = streckenelementPtr.get();

        for (const auto richtung : richtungen) {
            const StreckenelementUndRichtung er { streckenelement, richtung };
            const auto& info = er.richtungsInfo();
            if (!info.has_value()) {
                continue;
            }

            for (const auto seite : seiten) {
                if (hatEreignis(*info, anfangFuer(seite))) {
                    sucheBahnsteig(scene, er, seite, utmOffsetByElement);
                }
                if (hatEreignis(*info, mitteFuer(seite))) {
                    zeichneMitte(scene, er, seite, utmOffsetByElement);
                }
            }
        }
    }
}
