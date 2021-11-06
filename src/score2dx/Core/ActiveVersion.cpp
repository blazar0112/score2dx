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
    if (static_cast<std::size_t>(chartInfo.Level)>=mLevelSortedDifficultyLists.size())
    {
        throw std::runtime_error("invalid level");
    }
    mLevelSortedDifficultyLists[chartInfo.Level][musicId].emplace(styleDifficulty);
    mMusicChartInfoMap[musicId].emplace(styleDifficulty, std::move(chartInfo));
}

const ChartInfo*
ActiveVersion::
FindChartInfo(std::size_t musicId,
              StyleDifficulty styleDifficulty)
const
{
    auto findMusic = icl_s2::Find(mMusicChartInfoMap, musicId);
    if (!findMusic) { return nullptr; }

    auto findDifficulty = icl_s2::Find(findMusic.value()->second, styleDifficulty);
    if (!findDifficulty) { return nullptr; }

    return &(findDifficulty.value()->second);
}

const std::map<std::size_t, std::set<StyleDifficulty>> &
ActiveVersion::
GetChartDifficultyList(int level)
const
{
    if (static_cast<std::size_t>(level)>=mLevelSortedDifficultyLists.size())
    {
        throw std::runtime_error("invalid level");
    }
    return mLevelSortedDifficultyLists[level];
}

}
