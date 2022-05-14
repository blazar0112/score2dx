#include "score2dx/Iidx/VersionDateTimeRange.hpp"

namespace score2dx
{

VersionDateTimeRange::
VersionDateTimeRange(const std::string &beginDateTime, const std::string &endDateTime)
{
    if (beginDateTime.empty())
    {
        throw std::runtime_error("beginDateTime cannot be empty.");
    }

    if (!endDateTime.empty() && beginDateTime>=endDateTime)
    {
        throw std::runtime_error("beginDateTime >= endDateTime.");
    }

    mDateTimeRange[static_cast<std::size_t>(ies::RangeSide::Begin)] = beginDateTime;
    mDateTimeRange[static_cast<std::size_t>(ies::RangeSide::End)] = endDateTime;
}

const std::string &
VersionDateTimeRange::
Get(ies::RangeSide side)
const
{
    return mDateTimeRange[static_cast<std::size_t>(side)];
}

}
