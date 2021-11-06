#pragma once

#include <map>
#include <set>

#include "score2dx/Core/MusicInfo.hpp"
#include "score2dx/Iidx/Definition.hpp"

namespace score2dx
{

//! @brief ActiveVersion is a table keep track of Music/Chart availability at a certain version.
class ActiveVersion
{
public:
        explicit ActiveVersion(std::size_t versionIndex);

        std::size_t
        GetVersionIndex()
        const;

        void
        AddDifficulty(std::size_t musicId,
                      StyleDifficulty styleDifficulty,
                      ChartInfo chartInfo);

        const ChartInfo*
        FindChartInfo(std::size_t musicId,
                      StyleDifficulty styleDifficulty)
        const;

    //! @brief Get all chart difficulty with level={level} in Map of {MusicId, StyleDifficultyList}.
        const std::map<std::size_t, std::set<StyleDifficulty>> &
        GetChartDifficultyList(int level)
        const;

private:
    std::size_t mVersionIndex{0};

    //! @brief Map of {MusicId, Map of {StyleDifficulty, ChartInfo}}.
    std::map<std::size_t, std::map<StyleDifficulty, ChartInfo>> mMusicChartInfoMap;

    //! @brief Array of {Index=Level, Map of {MusicId, AvaiableStyleDifficultyList}}.
    //! @note Level = 0 is unused.
    std::array<std::map<std::size_t, std::set<StyleDifficulty>>, MaxLevel+1> mLevelSortedDifficultyLists;
};

}
