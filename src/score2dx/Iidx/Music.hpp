#pragma once

#include <array>
#include <list>
#include <map>
#include <set>
#include <vector>

#include "ies/Common/SmartEnum.hxx"

#include "score2dx/Iidx/ChartInfo.hpp"
#include "score2dx/Iidx/MusicInfo.hpp"

namespace score2dx
{

//! @brief ChartStatus denotes chart's availbility in timeline.
//! NotAvailable: before chart debut (all versions before music's version)
//!     Exception: two copula music 'Routing' and 'Shakunetsu Pt.2 Long Train Running'
//!     "previewed" in pendual, will have 22 as BeginAvailable.
//! BeginAvailable denotes first debut or revived version.
//! Available denotes continued available version after BeginAvailable.
//! Removed denotes version after music removed (to before it's next revive).
//! Example: DPN of 'LUV TO ME(disco mix)' has ChartVersions '00-04, 06, 10-15'
//!     00 01 02 03 04 05 06 07 08 09 10 11 12 13 14 15 16 17 18 19 20 21 22 23 24 25 26 27 28 29
//!     BA A  A  A  A  RM BA RM RM RM BA A  A  A  A  A  RM RM RM RM RM RM RM RM RM RM RM RM RM RM
IES_SMART_ENUM(ChartStatus,
    NotAvailable,
    BeginAvailable,
    Available,
    Removed
);

struct ChartAvailability
{
    ChartStatus ChartAvailableStatus{ChartStatus::NotAvailable};
    //! @brief ChartInfo may change because level changed.
    ChartInfo ChartInfoProp{0, 0};
    //! @brief Two charts are regarded as same chart if they have same notes.
    //! This Index point to unique note chart and can use to identify chart.
    //! @note ChartInfo may be changed (level change).
    std::size_t ChartIndex{0};
};

class Music
{
public:
        explicit Music(std::size_t musicId, const std::string &title);

        std::size_t
        GetMusicId()
        const;

        const MusicInfo &
        GetMusicInfo()
        const;

    //! @note Cannot set title.
        void
        SetMusicInfoField(MusicInfoField field, const std::string &fieldString);

        void
        AddAvailability(StyleDifficulty styleDifficulty,
                        const std::map<std::string, ChartInfo> &chartInfoByChartVersions);

    //! @brief Get first version of each chart first available. Vector of {Index=ChartIndex, FirstVersionIndexChartAvailable}.
    //! @example
    //! Chart 0: {0, 1, 4, 5} (remove at 2, revived at 4)
    //!       1: {2, 3} (replace Chart[0] in 2, removed at 4)
    //! GetChartFirstAvailableVersions(): {0, 2}
    //! FindSameChartVersions(styleDifficulty, 0): {0, 1, 4, 5}
    //! FindSameChartVersions(styleDifficulty, 1): {2, 3}
        std::vector<std::size_t>
        GetChartFirstAvailableVersions(StyleDifficulty styleDifficulty)
        const;

        const ChartAvailability &
        GetChartAvailability(StyleDifficulty styleDifficulty, std::size_t versionIndex)
        const;

    //! @brief Find versions has same chart as [versionIndex].
    //! Version can be after [versionIndex]. Must contain versionIndex if available.
    //! @note Empty if chart is not available at [versionIndex].
        std::vector<std::size_t>
        FindSameChartVersions(StyleDifficulty styleDifficulty, std::size_t versionIndex)
        const;

private:
    std::size_t mMusicId;
    MusicInfo mMusicInfo;

    //! @brief Array of {Index=StyleDifficulty, Vector of {Index=VersionIndex, ChartAvailability}}.
    std::array<std::vector<ChartAvailability>, StyleDifficultySmartEnum::Size()> mChartAvailabilityTable;

    //! @brief Array of {Index=StyleDifficulty, Vector of {Index=ChartIndex, ChartNote}}.
    std::array<std::vector<int>, StyleDifficultySmartEnum::Size()> mChartNoteByIndex;
};

}
