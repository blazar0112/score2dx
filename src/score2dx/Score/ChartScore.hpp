#pragma once

#include <optional>
#include <string>

#include "icl_s2/Common/SmartEnum.hxx"

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
};

std::string
ToString(const ChartScore &chartScore);

}
