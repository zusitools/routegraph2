#include "oberbauvisualisierung.h"

#include <cmath>

#include <QPen>
#include <QColor>

#include "zusi_file_lib/src/model/streckenelement.hpp"
#include "zusi_file_lib/src/common/types.hpp"

#include "view/graphicsitems/streckensegmentitem.h"
#include "view/graphicsitems/label.h"

bool OberbauSegmentierer::istSegmentGrenze(const StreckenelementUndRichtung &vorgaenger, const StreckenelementUndRichtung &nachfolger) const
{
    return vorgaenger->oberbauName != nachfolger->oberbauName;
}

void OberbauVisualisierung::setzeDarstellung(StreckensegmentItem& item)
{
    QPen pen = item.pen();

    std::string oberbauName { item.start()->oberbauName };
    auto it = this->colors_.find(oberbauName);
    if (it == std::end(this->colors_)) {
        static constexpr int kNumColors = 14;
        static constexpr int colormap[kNumColors] = { 0, 40, 53, 70, 130, 160, 180, 197, 214, 247, 258, 270, 286, 300 };
        QColor color = QColor::fromHsv(
                colormap[this->colors_.size() % kNumColors],
                255,
                std::max(0, static_cast<int>(255 - (this->colors_.size() / kNumColors) * 64)));
        this->colors_[oberbauName] = color;
        pen.setColor(color);
    } else {
        pen.setColor(it->second);
    }
    item.setPen(pen);
}

unique_ptr<QGraphicsScene> OberbauVisualisierung::legende()
{
    auto result = std::make_unique<QGraphicsScene>();
    auto segmentierer = this->segmentierer();
    Streckenelement streckenelement;
    for (const auto& it : this->colors_) {
        streckenelement.oberbauName = it.first;
        this->neuesLegendeElement(*result, *segmentierer, streckenelement, it.first == "" ? QString("(k.A.)") : QString::fromUtf8(it.first.data(), it.first.size()));
    }
    return result;
}
