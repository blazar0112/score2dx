#include "score2dx/Iidx/VersionDateTimeRange.hpp"

#include <iostream>

#include "fmt/core.h"

namespace score2dx
{

VersionDateTimeRange::
VersionDateTimeRange(const std::string &beginDateTime, const std::string &endDateTime)
{
    //'' there's static init for VersionDateTimeRange so cannot throw to upper level.
    try
    {
        if (beginDateTime.empty())
        {
            throw std::runtime_error("beginDateTime cannot be empty.");
        }

        if (!endDateTime.empty() && beginDateTime>=endDateTime)
        {
            throw std::runtime_error{fmt::format("beginDateTime [{}] >= endDateTime [{}].", beginDateTime, endDateTime)};
        }

        mDateTimeRange[static_cast<std::size_t>(ies::RangeSide::Begin)] = beginDateTime;
        mDateTimeRange[static_cast<std::size_t>(ies::RangeSide::End)] = endDateTime;
    }
    catch (const std::exception& e)
    {
        std::cerr   << "VersionDateTimeRange::VersionDateTimeRange(): exception:\n"
                    << e.what() << std::endl;
        std::terminate();
    }
}

const std::string &
VersionDateTimeRange::
Get(ies::RangeSide side)
const
{
    return mDateTimeRange[static_cast<std::size_t>(side)];
}

}
