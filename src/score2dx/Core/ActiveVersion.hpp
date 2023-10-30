#pragma once

#include <array>
#include <map>
#include <set>

#include "score2dx/Iidx/ChartInfo.hpp"
#include "score2dx/Iidx/Definition.hpp"
#include "score2dx/Iidx/MusicInfo.hpp"

namespace score2dx
{

//! @brief ActiveVersion is a table keep track of Music/Chart availability at a certain version.
//! @note Information is stored in MusicDatabase's AllTimeMusics, ActiveVersion is auxiliary to
//! collect all charts available at a version, and filter by level.
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
                      const ChartInfo& chartInfo);

        const std::set<std::size_t>&
        GetChartIdList()
        const;

    //! @brief Get chart id list which level={level}.
        const std::set<std::size_t>&
        GetChartIdList(int level)
        const;

        const std::set<Difficulty>&
        GetAvailableCharts(std::size_t musicId,
                           PlayStyle playStyle)
        const;

private:
    std::size_t mVersionIndex{0};

    //! @brief Set of {ChartId}.
    std::set<std::size_t> mChartIds;

    //! @brief Array of {Index=Level, ChartIdList}.
    //! @note Level = 0 is unused.
    std::array<std::set<std::size_t>, MaxLevel+1> mChartIdListByLevel;

    //! @brief Map of {MusicId, Array{Index=PlayStyle, AvailableDifficulties}}.
    std::map<std::size_t, std::array<std::set<Difficulty>, PlayStyleSmartEnum::Size()>> mMusicAvailableCharts;
};

}
