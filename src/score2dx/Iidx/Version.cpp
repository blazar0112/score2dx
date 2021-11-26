#include "score2dx/Iidx/Version.hpp"

#include <array>
#include <map>

#include "fmt/format.h"

#include "icl_s2/Common/IntegralRangeUsing.hpp"
#include "icl_s2/StdUtil/Find.hxx"
#include "icl_s2/String/SplitString.hpp"

namespace
{

//! @brief Map of {VersionIndex, Array of [BeginDateTime, EndDateTime]}.
//! EndDateTime = 23:59 of day before next version release day.
//! @note tricoro use Ver.UP release date, not tricoro-machine limited release date (2012-09-19).
const std::map<std::size_t, std::array<std::string, icl_s2::RangeSideSmartEnum::Size()>> VersionDateTimeRangeMap
{
    {29, {"2021-10-13 00:00", ""}},
    {28, {"2020-10-28 00:00", "2021-10-12 23:59"}},
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

std::string
ToVersionString(std::size_t versionIndex)
{
    return fmt::format("{:02}", versionIndex);
}

bool
IsValidVersion(const std::string &version)
{
    if (version.size()!=2)
    {
        return false;
    }

    if (!std::all_of(version.begin(), version.end(), ::isdigit))
    {
        return false;
    }

    auto versionIndex = std::stoull(version);
    if (versionIndex>GetLatestVersionIndex())
    {
        return false;
    }

    return true;
}

std::size_t
GetLatestVersionIndex()
{
    return VersionNames.size()-1;
}

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

std::size_t
GetFirstDateTimeAvailableVersionIndex()
{
    return VersionDateTimeRangeMap.begin()->first;
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

std::size_t
FindVersionIndexFromDateTime(const std::string &dateTime)
{
    auto versionIndex = VersionDateTimeRangeMap.begin()->first;
    for (auto &[ver, dateTimeRange] : VersionDateTimeRangeMap)
    {
        if (dateTime>=dateTimeRange[static_cast<int>(icl_s2::RangeSide::Begin)])
        {
            versionIndex = ver;
        }
    }

    return versionIndex;
}

VersionDateType
FindVersionDateType(const std::string &dateTime)
{
    if (dateTime.empty())
    {
        return VersionDateType::VersionEnd;
    }

    auto versionIndex = FindVersionIndexFromDateTime(dateTime);
    auto versionDateTimeRange = GetVersionDateTimeRange(versionIndex);
    auto tokens = icl_s2::SplitString(" ", dateTime);
    auto &date = tokens[0];

    if (icl_s2::Find(versionDateTimeRange.at(icl_s2::RangeSide::Begin), date))
    {
        return VersionDateType::VersionBegin;
    }

    if (icl_s2::Find(versionDateTimeRange.at(icl_s2::RangeSide::End), date))
    {
        return VersionDateType::VersionEnd;
    }

    return VersionDateType::None;
}

}
