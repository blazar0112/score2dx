#include "score2dx/Iidx/Version.hpp"

#include <array>
#include <map>

#include "icl_s2/Common/IntegralRangeUsing.hpp"
#include "icl_s2/StdUtil/Find.hxx"

namespace
{

//! @brief Map of {VersionIndex, Array of [BeginDateTime, EndDateTime]}.
//! @note tricoro use Ver.UP release date, not tricoro-machine limited release date (2012-09-19).
const std::map<std::size_t, std::array<std::string, icl_s2::RangeSideSmartEnum::Size()>> VersionDateTimeRangeMap
{
    {28, {"2020-10-28 00:00", ""}},
    {27, {"2019-10-16 00:00", "2020-10-27 23:59"}},
    {26, {"2018-11-07 00:00", "2019-10-15 23:59"}},
    {25, {"2017-12-21 00:00", "2018-11-06 23:59"}},
    {24, {"2016-10-26 00:00", "2017-12-20 23:59"}},
    {23, {"2015-11-11 00:00", "2016-10-25 23:59"}},
    {22, {"2014-09-17 00:00", "2015-11-10 23:29"}},
    {21, {"2013-11-13 00:00", "2014-09-16 23:59"}},
    {20, {"2012-09-25 00:00", "2013-11-12 23:59"}},
    {19, {"2011-09-15 00:00", "2012-09-24 23:59"}},
    {18, {"2010-09-15 00:00", "2011-09-14 23:59"}},
    {17, {"2009-10-21 00:00", "2010-09-14 23:59"}}
};

}

namespace score2dx
{

std::optional<std::size_t>
FindVersionIndex(const std::string &dbVersionName)
{
    static std::map<std::string, std::size_t> VersionIndexMap;
    if (VersionIndexMap.empty())
    {
        for (auto index : IndexRange{0, VersionNames.size()})
        {
            VersionIndexMap[VersionNames[index]] = index;
        }
    }

    if (auto findIndex = icl_s2::Find(VersionIndexMap, dbVersionName))
    {
        return findIndex.value()->second;
    }

    return std::nullopt;
}

std::map<icl_s2::RangeSide, std::string>
GetVersionDateTimeRange(std::size_t versionIndex)
{
    std::map<icl_s2::RangeSide, std::string> range;

    if (auto findVersion = icl_s2::Find(VersionDateTimeRangeMap, versionIndex))
    {
        auto &dateTimeArray = findVersion.value()->second;
        for (auto rangeSide : icl_s2::RangeSideSmartEnum::ToRange())
        {
            range[rangeSide] = dateTimeArray[static_cast<std::size_t>(rangeSide)];
        }
    }

    return range;
}

}
