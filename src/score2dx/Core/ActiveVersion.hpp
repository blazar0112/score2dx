#pragma once

#include <array>
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

    //! @brief Map of {ChartId, ChartInfo}.
        const std::map<std::size_t, ChartInfo> &
        GetChartInfos()
        const;

    //! @brief Get chart id list which level={level}.
        const std::set<std::size_t> &
        GetChartIdList(int level)
        const;

private:
    std::size_t mVersionIndex{0};

    //! @brief Map of {ChartId, ChartInfo}.
    std::map<std::size_t, ChartInfo> mChartInfos;

    //! @brief Array of {Index=Level, ChartIdList}.
    //! @note Level = 0 is unused.
    std::array<std::set<std::size_t>, MaxLevel+1> mChartIdListByLevel;
};

}
