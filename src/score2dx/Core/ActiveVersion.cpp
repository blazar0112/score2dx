#include "score2dx/Core/ActiveVersion.hpp"

#include <iostream>

#include "icl_s2/StdUtil/Find.hxx"

namespace score2dx
{

ActiveVersion::
ActiveVersion(std::size_t versionIndex)
:   mVersionIndex(versionIndex)
{
}

std::size_t
ActiveVersion::
GetVersionIndex()
const
{
    return mVersionIndex;
}

void
ActiveVersion::
AddDifficulty(std::size_t musicId,
              StyleDifficulty styleDifficulty,
              ChartInfo chartInfo)
{
    if (static_cast<std::size_t>(chartInfo.Level)>=mChartIdListByLevel.size())
    {
        throw std::runtime_error("invalid level");
    }

    auto chartId = ToChartId(musicId, styleDifficulty);

    mChartIdListByLevel[chartInfo.Level].emplace(chartId);
    mChartInfos.emplace(chartId, std::move(chartInfo));
}

const ChartInfo*
ActiveVersion::
FindChartInfo(std::size_t musicId,
              StyleDifficulty styleDifficulty)
const
{
    auto chartId = ToChartId(musicId, styleDifficulty);
    auto findChartInfo = icl_s2::Find(mChartInfos, chartId);
    if (!findChartInfo) { return nullptr; }

    return &(findChartInfo.value()->second);
}

const std::map<std::size_t, ChartInfo> &
ActiveVersion::
GetChartInfos()
const
{
    return mChartInfos;
}

const std::set<std::size_t> &
ActiveVersion::
GetChartIdList(int level)
const
{
    if (static_cast<std::size_t>(level)>=mChartIdListByLevel.size())
    {
        throw std::runtime_error("invalid level");
    }
    return mChartIdListByLevel[level];
}

}
