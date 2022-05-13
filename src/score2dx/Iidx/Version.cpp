#include "score2dx/Iidx/Version.hpp"

#include <array>
#include <iostream>
#include <map>

#include "fmt/format.h"

#include "ies/Common/IntegralRangeUsing.hpp"
#include "ies/StdUtil/Find.hxx"
#include "ies/String/SplitString.hpp"

#include "score2dx/Core/CheckedParse.hxx"

namespace
{

//! @brief Map of {VersionIndex, Array of [BeginDateTime, EndDateTime]}.
//! EndDateTime = 23:59 of day before next version release day.
//! @note tricoro use Ver.UP release date, not tricoro-machine limited release date (2012-09-19).
const std::map<std::size_t, std::array<std::string, ies::RangeSideSmartEnum::Size()>> VersionDateTimeRangeMap
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

    if (auto findIndex = ies::Find(VersionIndexMap, dbVersionName))
    {
        return findIndex.value()->second;
    }

    return std::nullopt;
}

std::size_t
GetFirstSupportDateTimeVersionIndex()
{
    return VersionDateTimeRangeMap.begin()->first;
}

const ies::IndexRange &
GetSupportScoreVersionRange()
{
    static ies::IndexRange ScoreVersionRange{GetFirstSupportDateTimeVersionIndex(), VersionNames.size()};
    return ScoreVersionRange;
}

std::map<ies::RangeSide, std::string>
GetVersionDateTimeRange(std::size_t versionIndex)
{
    std::map<ies::RangeSide, std::string> range;

    if (auto findVersion = ies::Find(VersionDateTimeRangeMap, versionIndex))
    {
        auto &dateTimeArray = findVersion.value()->second;
        for (auto rangeSide : ies::RangeSideSmartEnum::ToRange())
        {
            range[rangeSide] = dateTimeArray[static_cast<std::size_t>(rangeSide)];
        }
    }

    return range;
}

std::optional<std::size_t>
FindVersionIndexFromDateTime(const std::string &dateTime)
{
    auto &[firstVersionIndex, firstVersionDateTimeRange] = *VersionDateTimeRangeMap.begin();
    auto &firstVersionBeginDateTime = firstVersionDateTimeRange[static_cast<int>(ies::RangeSide::Begin)];
    if (dateTime<firstVersionBeginDateTime)
    {
        return std::nullopt;
    }

    auto versionIndex = firstVersionIndex;
    for (auto &[ver, dateTimeRange] : VersionDateTimeRangeMap)
    {
        if (dateTime>=dateTimeRange[static_cast<int>(ies::RangeSide::Begin)])
        {
            versionIndex = ver;
        }
    }

    return {versionIndex};
}

VersionDateType
FindVersionDateType(const std::string &dateTime)
{
    if (dateTime.empty())
    {
        return VersionDateType::VersionEnd;
    }

    auto findVersionIndex = FindVersionIndexFromDateTime(dateTime);
    if (!findVersionIndex)
    {
        return VersionDateType::None;
    }
    auto versionIndex = findVersionIndex.value();

    auto versionDateTimeRange = GetVersionDateTimeRange(versionIndex);
    auto tokens = ies::SplitString(" ", dateTime);
    auto &date = tokens[0];

    if (ies::Find(versionDateTimeRange.at(ies::RangeSide::Begin), date))
    {
        return VersionDateType::VersionBegin;
    }

    if (ies::Find(versionDateTimeRange.at(ies::RangeSide::End), date))
    {
        return VersionDateType::VersionEnd;
    }

    return VersionDateType::None;
}

std::string
ToString(const ies::IntegralRangeList<std::size_t> &availableVersions)
{
    std::string s;
    auto ranges = availableVersions.GetRanges();
    auto isFirst = true;
    for (auto &range : ranges)
    {
        if (!isFirst) s += ", ";

        if (range.size()==1)
        {
            s += ToVersionString(range.GetMin());
        }
        else
        {
            s += ToVersionString(range.GetMin())+"-"+ToVersionString(range.GetMax());
        }

        isFirst = false;
    }

    return s;
}

ies::IntegralRangeList<std::size_t>
ToRangeList(const std::string &availableVersions)
{
    ies::IntegralRangeList<std::size_t> rangeList;

    if (ies::Find(availableVersions, "cs"))
    {
        return rangeList;
    }

    std::size_t start = 0;
    std::size_t end = 0;

    std::string_view view{availableVersions};

    while ((start = view.find_first_not_of(", ", end))!=std::string_view::npos)
    {
        end = view.find(',', start+1);
        if (end==std::string_view::npos)
        {
            end = view.length();
        }

        auto versionRange = view.substr(start, end-start);
        if (versionRange.size()==2)
        {
            std::size_t versionIndex = 0;
            CheckedParse(versionRange, versionIndex, "versionRange[SingleVersionIndex]");
            rangeList.AddRange({versionIndex, versionIndex+1});
        }
        //'' for 00-29 like case, note it's range [00, 29], not [00, 29).
        else if (versionRange.size()==5)
        {
            std::size_t beginVersionIndex = 0;
            std::size_t endVersionIndex = 0;
            CheckedParse(versionRange.substr(0, 2), beginVersionIndex, "versionRange[beginVersionIndex]");
            CheckedParse(versionRange.substr(3, 2), endVersionIndex, "versionRange[endVersionIndex]");
            rangeList.AddRange({beginVersionIndex, endVersionIndex+1});
        }
        else
        {
            std::cout << versionRange << std::endl;
            throw std::runtime_error("incorrect availableVersions "+availableVersions);
        }
    }

    return rangeList;
}

}
