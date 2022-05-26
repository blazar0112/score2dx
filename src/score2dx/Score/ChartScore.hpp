#pragma once

#include <optional>
#include <string>

#include "ies/Common/SmartEnum.hxx"

#include "score2dx/Iidx/Definition.hpp"

namespace score2dx
{

struct ChartScore
{
    int Level{0};
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

//! @brief See if chartScore is a default initialized ChartScore.
bool
IsDefault(const ChartScore &chartScore);

//! @brief See if chartScore is trivial, that is everything default initialized except Level.
bool
IsTrivial(const ChartScore &chartScore);

//! @brief See if two chartScores are equivalent in value fields (ExScore/PGreatCount/GreatCount/MissCount).
bool
IsValueEqual(const ChartScore &lhs, const ChartScore &rhs);

}
