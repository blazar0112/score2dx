#pragma once

#include <array>
#include <map>
#include <memory>
#include <set>
#include <vector>

#include "ies/Common/RangeSide.hpp"
#include "ies/Common/SmartEnum.hxx"

#include "score2dx/Analysis/CareerRecord.hpp"
#include "score2dx/Core/MusicDatabase.hpp"
#include "score2dx/Iidx/Definition.hpp"
#include "score2dx/Score/PlayerScore.hpp"
#include "score2dx/Score/ScoreLevel.hpp"

namespace score2dx
{

IES_SMART_ENUM(FindChartScoreOption,
    AtDateTime,
    BeforeDateTime
);

struct Statistics
{
    //! @brief Total ChartIdList available for this statistics.
    //! @note It should be equivalent to:
    //!     Sum of ChartIdListByClearType
    //!     (since every chart score should have at least NO_PLAY)
    //! @note Not every chart score have current Score/DjLevel/ScoreLevelCategory.
    //! So sum of ChartIdListByDjLevel/ChartIdListByScoreLevelRange may only be part of total ChartIdList.
    std::set<std::size_t> ChartIdList;

    //! @brief Arry of {Index=ClearType, ChartIdList}.
    std::array<std::set<std::size_t>, ClearTypeSmartEnum::Size()> ChartIdListByClearType;

    //! @brief Array of {Index=DjLevel, ChartIdList}.
    std::array<std::set<std::size_t>, DjLevelSmartEnum::Size()> ChartIdListByDjLevel;

    //! @brief Array of {Index=ScoreLevelCategory, ChartIdList}.
    std::array<std::set<std::size_t>, ScoreLevelCategorySmartEnum::Size()> ChartIdListByScoreLevelCategory;
};

struct ActivityData
{
    const MusicScore* CurrentMusicScore{nullptr};
    const MusicScore* PreviousMusicScore{nullptr};
};

//! @brief Activity analysis of specific date time range.
struct ActivityAnalysis
{
    std::map<ies::RangeSide, std::string> DateTimeRange;

    //! @brief PreviousSnapshot of all available music.
    //! Map of {PlayStyle, Map of {MusicId, MusicScore}}.
    std::map<PlayStyle, std::map<std::size_t, MusicScore>> PreviousSnapshot;

    //! @brief Updating activity after snapshot, sorted by datetime.
    //! Map of {PlayStyle, Map of {DateTime, Map of {MusicId, MusicScore}}}.
    std::map<PlayStyle, std::map<std::string, std::map<std::size_t, MusicScore>>> ActivityByDateTime;

    //! @brief Activity Snapshot of each date time of all available musics.
    //! Map of {PlayStyle, Map of {DateTime, Map of {MusicId, ActivityData}}}.
    std::map<PlayStyle, std::map<std::string, std::map<std::size_t, ActivityData>>> ActivitySnapshotByDateTime;
};

//! @brief Data analysis from PlayerScore.
//! 1. BestScore of current version, and career best from PlayerScore.
//! 2. Score Statistics
//! 3. Acitivty by date.
//! @note BestScore includes SPB data, but Statistics do not include SPB.
struct ScoreAnalysis
{
    std::unique_ptr<CareerRecord> CareerRecordPtr;

    //! @brief Array of {Index=PlayStyle, Statistics}.
    //! @note It should be equivalent to
    //!     Sum of levels in StatisticsByStyleLevel
    //!     Sum of difficulty in StatisticsByStyleDifficulty (of each style)
    //!     Sum of difficulty and version in StatisticsByVersionStyleDifficulty (of each style)
    std::array<Statistics, PlayStyleSmartEnum::Size()> StatisticsByStyle;

    //! @brief Vector of {Index=VersionIndex, Array of {Index=Style, Statistics}}.
    std::vector<std::array<Statistics, PlayStyleSmartEnum::Size()>> StatisticsByVersionStyle;

    //! @brief Array of {Index=PlayStyle, Array of {Index=Level, Statistics}}. Level = 0 is unused.
    std::array<std::array<Statistics, MaxLevel+1>, PlayStyleSmartEnum::Size()> StatisticsByStyleLevel;

    //! @brief Array of {Index=StyleDifficulty, Statistics}.
    std::array<Statistics, StyleDifficultySmartEnum::Size()> StatisticsByStyleDifficulty;

    //! @brief Vector of {Index=VersionIndex, Array of {Index=StyleDifficulty, Statistics}}.
    std::vector<std::array<Statistics, StyleDifficultySmartEnum::Size()>> StatisticsByVersionStyleDifficulty;
};

class Analyzer
{
public:
        explicit Analyzer(const MusicDatabase &musicDatabase);

        void
        SetActiveVersionIndex(std::size_t activeVersionIndex);

        std::size_t
        GetActiveVersionIndex()
        const;

        ScoreAnalysis
        Analyze(const PlayerScore &playerScore)
        const;

    //! @brief Analyze activity during current active version date time range.
    //! @note In CSV may have initial inherited data records with play count zero at time of data transfer.
    //! But third party import data may not have play count set correctly.
    //! So play count = 0 records are kept.
        ActivityAnalysis
        AnalyzeVersionActivity(const PlayerScore &playerScore)
        const;

    //! @brief Analyze activity during specific date time range.
    //! @note DateTimeRange need inside a version (the activeVersion).
        ActivityAnalysis
        AnalyzeActivity(const PlayerScore &playerScore,
                        const std::string &beginDateTime,
                        const std::string &endDateTime)
        const;

private:
    const MusicDatabase &mMusicDatabase;
    //! @brief Current active version, default to latest version in music database.
    std::size_t mActiveVersionIndex;

    //! @brief Find Status of ChartScore at given time, consider active version, version change, availibilty.
    //! Not found if:
    //! 1. Player don't have score for music.
    //! 2. Music/Chart does not available at datetime's active version.
    //! (Note: later chart added during a version like SPL is consider
    //!     have NO_PLAY at the beginning of that version, to simplify the problem.)
    //! @note If chart availibilty is continued at datetime's active version,
    //!     then clear type is inherited from previous score data if exist such data.
    //! Else the chart is regarded wipe to NO_PLAY at beginning of that version.
    //! @example
    //! Clear EASY          HARD
    //! Score  100          50
    //! Time     *    VB    *              VE
    //!                ^ VersionBegin       ^ VersionEnd
    //!          t1   t2    t3
    //! FindChartScore [t1, t2) = {EASY, 100}, [t2, t3) = {EASY, 0}, [t3, VE] = {HARD, 50}
    //! If t1 is very early version, and music is deleted in between, then [t2, t3) = {NO_PLAY, 0}.

        std::optional<ChartScore>
        FindChartScoreByTime(const PlayerScore &playerScore,
                             std::size_t musicId,
                             PlayStyle playStyle,
                             Difficulty difficulty,
                             const std::string &dateTime,
                             FindChartScoreOption option)
        const;
};

}
