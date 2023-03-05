#include "score2dx/Core/ActiveVersion.hpp"

#include <iostream>

#include "ies/StdUtil/Find.hxx"

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
              const ChartInfo& chartInfo)
{
    if (static_cast<std::size_t>(chartInfo.Level)>=mChartIdListByLevel.size())
    {
        throw std::runtime_error("invalid level");
    }

    auto chartId = ToChartId(musicId, styleDifficulty);

    mChartIds.emplace(chartId);
    mChartIdListByLevel[chartInfo.Level].emplace(chartId);

    auto [playStyle, difficulty] = Split(styleDifficulty);
    auto styleIndex = static_cast<std::size_t>(playStyle);
    mMusicAvailableCharts[musicId][styleIndex].emplace(difficulty);
}

const std::set<std::size_t>&
ActiveVersion::
GetChartIdList()
const
{
    return mChartIds;
}

const std::set<std::size_t>&
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

const std::set<Difficulty>&
ActiveVersion::
GetAvailableCharts(std::size_t musicId,
                   PlayStyle playStyle)
const
{
    auto findMusicCharts = ies::Find(mMusicAvailableCharts, musicId);
    if (!findMusicCharts)
    {
        throw std::runtime_error("not available charts for music ["+ToMusicIdString(musicId)+"]");
    }

    auto& styleSortedMusicCharts = findMusicCharts.value()->second;
    auto styleIndex = static_cast<std::size_t>(playStyle);
    return styleSortedMusicCharts[styleIndex];
}

}
