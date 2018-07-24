#include "neigungvisualisierung.h"

#include "model/streckenelement.h"

#include <QPen>
#include <QColor>

#include <cmath>

namespace {
    constexpr static qreal neigungAbstufung = 0.005;
    constexpr static int anzahlNeigungsStufen = 8;
    constexpr static int colormap[anzahlNeigungsStufen][3] = {
        { 0, 0, 0 },
        { 255, 128, 255 },
        { 0, 128, 255 },
        { 0, 255, 255 },
        { 0, 255, 0 },
        { 255, 230, 0 },
        { 255, 128, 0 },
        { 255, 0, 0 },
    };
}

bool NeigungSegmentierer::istSegmentGrenze(const StreckenelementUndRichtung vorgaenger, const StreckenelementUndRichtung nachfolger) const
{
    const bool gleisfunktionGleich = (hatFktFlag(*vorgaenger, StreckenelementFlag::KeineGleisfunktion) ==
            hatFktFlag(*nachfolger, StreckenelementFlag::KeineGleisfunktion));
    const bool neigungGleich = (std::fabs(std::trunc(vorgaenger->Neigung() / neigungAbstufung)) == std::fabs(std::trunc(nachfolger->Neigung() / neigungAbstufung)));
    return !gleisfunktionGleich || !neigungGleich;
}

void NeigungVisualisierung::setzeDarstellung(StreckensegmentItem& item)
{
    QPen pen = item.pen();

    const int neigungIndex = std::max(0, std::min(anzahlNeigungsStufen - 1, static_cast<int>(std::fabs(std::trunc(item.start()->Neigung() / neigungAbstufung)))));

    if (hatFktFlag(*item.start(), StreckenelementFlag::KeineGleisfunktion)) {
        pen.setColor(Qt::lightGray);
    } else {
        pen.setColor(QColor::fromRgb(colormap[neigungIndex][0], colormap[neigungIndex][1], colormap[neigungIndex][2]));
    }

    item.setPen(pen);
}

std::unique_ptr<QGraphicsScene> NeigungVisualisierung::legende()
{
    auto result = std::make_unique<QGraphicsScene>();
    auto segmentierer = this->segmentierer();
    StrElement streckenelement {};
    for (int i = 0; i < anzahlNeigungsStufen; ++i) {
        streckenelement.b.Z = /* Rundungsfehler-Ausgleich */ 1.01 * i * neigungAbstufung * Visualisierung::legendeElementLaenge;
        this->neuesLegendeElement(*result, *segmentierer, streckenelement,
            i == anzahlNeigungsStufen - 1 ?
                (QString::fromUtf8("⩾ ") + QString::number(static_cast<int>(i * neigungAbstufung * 1000.0))) + QString::fromUtf8("‰") :
                (QString::fromUtf8("< ") + QString::number(static_cast<int>((i+1) * neigungAbstufung * 1000.0))) + QString::fromUtf8("‰"));
    }
    return result;
}
