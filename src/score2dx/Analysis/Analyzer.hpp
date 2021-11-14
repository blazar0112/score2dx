#pragma once

#include <array>
#include <map>
#include <set>
#include <vector>

#include "icl_s2/Common/SmartEnum.hxx"

#include "score2dx/Core/MusicDatabase.hpp"
#include "score2dx/Iidx/Definition.hpp"
#include "score2dx/Score/PlayerScore.hpp"
#include "score2dx/Score/ScoreLevel.hpp"

namespace score2dx
{

//! @brief Special ScoreLevelRange enum for statistics.
ICL_S2_SMART_ENUM(StatisticScoreLevelRange,
    AMinus,
    AEqPlus,
    AAMinus,
    AAEqPlus,
    AAAMinus,
    AAAEqPlus,
    MaxMinus,
    Max
);

std::string
ToPrettyString(StatisticScoreLevelRange statisticScoreLevelRange);

StatisticScoreLevelRange
FindStatisticScoreLevelRange(int note, int exScore);

StatisticScoreLevelRange
FindStatisticScoreLevelRange(ScoreLevelRange scoreLevelRange);

struct Statistics
{
    //! @brief Total ChartIdList available for this statistics.
    //! @note It should be equivalent to:
    //!     Sum of ChartIdListByClearType
    //!     (since every chart score should have at least NO_PLAY)
    //! @note Not every chart score have current Score/DjLevel/StatisticScoreLevelRange.
    //! So sum of ChartIdListByDjLevel/ChartIdListByScoreLevelRange may only be part of total ChartIdList.
    std::set<std::size_t> ChartIdList;

    //! @brief Map of {ClearType, ChartIdList}.
    std::map<ClearType, std::set<std::size_t>> ChartIdListByClearType;

    //! @brief Map of {DjLevel, ChartIdList}.
    std::map<DjLevel, std::set<std::size_t>> ChartIdListByDjLevel;

    //! @brief Map of {StatisticScoreLevelRange, ChartIdList}.
    std::map<StatisticScoreLevelRange, std::set<std::size_t>> ChartIdListByScoreLevelRange;

        Statistics();
};

struct CareerBestChartScore
{
    ChartScore BestChartScore;
    std::size_t VersionIndex{0};
    std::string DateTime;
};

struct BestScoreData
{
    //! @note DateTime is omitted, since each chart have different best, can have play count of version.
    MusicScore VersionBestMusicScore;

    //! @brief Career BestExScore for each difficulty. Map of {Difficulty, CareerBestChartScore}.
    std::map<Difficulty, CareerBestChartScore> CareerBestChartScores;

        BestScoreData(std::size_t musicId,
                      PlayStyle playStyle);
};

//! @note BestScore includes SPB data, but Statistics do not include SPB.
struct ScoreAnalysis
{
    //! @brief Map of {MusicId, Map of {PlayStyle, BestScoreData}}.
    std::map<std::size_t, std::map<PlayStyle, BestScoreData>> MusicBestScoreData;

    //! @brief Map of {PlayStyle, Statistics}.
    //! @note It should be equivalent to
    //!     Sum of levels in StatisticsByStyleLevel
    //!     Sum of difficulty in StatisticsByStyleDifficulty (of each style)
    //!     Sum of difficulty and version in StatisticsByVersionStyleDifficulty (of each style)
    std::map<PlayStyle, Statistics> StatisticsByStyle;

    //! @brief Vector of {Index=VersionIndex, Map of {Style, Statistics}}.
    std::vector<std::map<PlayStyle, Statistics>> StatisticsByVersionStyle;

    //! @brief Map of {PlayStyle, Array of {Index=Level, Statistics}}. Level = 0 is unused.
    std::map<PlayStyle, std::array<Statistics, MaxLevel+1>> StatisticsByStyleLevel;

    //! @brief Map of {StyleDifficulty, Statistics}.
    std::map<StyleDifficulty, Statistics> StatisticsByStyleDifficulty;

    //! @brief Vector of {Index=VersionIndex, Map of {StyleDifficulty, Statistics}}.
    std::vector<std::map<StyleDifficulty, Statistics>> StatisticsByVersionStyleDifficulty;
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

private:
    const MusicDatabase &mMusicDatabase;
    //! @brief Current active version, default to latest version in music database.
    std::size_t mActiverVersionIndex;
};

}
