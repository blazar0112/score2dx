#pragma once

#include <optional>
#include <string>

#include "ies/Common/SmartEnum.hxx"

#include "score2dx/Iidx/Definition.hpp"

namespace score2dx
{

struct ChartScore
{
    ClearType ClearType{ClearType::NO_PLAY};
    DjLevel DjLevel{DjLevel::F};
    int ExScore{0};
    int PGreatCount{0};
    int GreatCount{0};
    std::optional<int> MissCount;

        auto operator<=>(const ChartScore&) const = default;
};

std::string
ToString(const ChartScore &chartScore);

bool
IsDefault(const ChartScore &chartScore);

}
