#pragma once

#include <array>
#include <string>

#include "ies/Common/RangeSide.hpp"

namespace score2dx
{

class VersionDateTimeRange
{
public:
        VersionDateTimeRange(const std::string &beginDateTime, const std::string &endDateTime);

        const std::string &
        Get(ies::RangeSide side)
        const;

private:
    std::array<std::string, ies::RangeSideSmartEnum::Size()> mDateTimeRange;
};

}
