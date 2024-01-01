#include "score2dx/Core/ActiveVersion.hpp"

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
    if (chartInfo.Level>MaxLevel)
    {
        throw std::runtime_error("invalid level");
    }

    auto [playStyle, difficulty] = Split(styleDifficulty);
    auto styleIndex = ToIndex(playStyle);
    auto chartId = ToChartId(musicId, styleDifficulty);

    mStyleChartIds[styleIndex].emplace(chartId);
    mStyleChartIdsByLevel[styleIndex][static_cast<std::size_t>(chartInfo.Level)].emplace(chartId);
    mMusicAvailableCharts[musicId][styleIndex].emplace(difficulty);
}

const std::set<std::size_t>&
ActiveVersion::
GetChartIds(PlayStyle playStyle)
const
{
    auto styleIndex = ToIndex(playStyle);
    return mStyleChartIds[styleIndex];
}

const std::set<std::size_t>&
ActiveVersion::
GetChartIds(PlayStyle playStyle, int level)
const
{
    if (level>MaxLevel)
    {
        throw std::runtime_error("invalid level");
    }

    auto styleIndex = ToIndex(playStyle);
    return mStyleChartIdsByLevel[styleIndex][static_cast<std::size_t>(level)];
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
    auto styleIndex = ToIndex(playStyle);
    return styleSortedMusicCharts[styleIndex];
}

}
