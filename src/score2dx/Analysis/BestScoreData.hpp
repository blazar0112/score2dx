#pragma once

#include <map>
#include <set>
#include <string>

#include "icl_s2/Common/SmartEnum.hxx"

#include "score2dx/Score/MusicScore.hpp"

namespace score2dx
{

ICL_S2_SMART_ENUM(BestDifferType,
    ExScore,
    Miss
);

ICL_S2_SMART_ENUM(BestScoreType,
    BestExScore,
    SecondBestExScore,
    BestMiss,
    SecondBestMiss
);

struct ChartScoreRecord
{
    ChartScore ChartScoreProp;
    std::size_t VersionIndex{0};
    std::string DateTime;

        ChartScoreRecord() = default;
        ChartScoreRecord(const ChartScore &chartScore,
                         std::size_t versionIndex,
                         const std::string &dateTime)
        :   ChartScoreProp(chartScore),
            VersionIndex(versionIndex),
            DateTime(dateTime)
        {}
};

//! @brief BestScoreData help track career best and active version best score data of one music in a style.
//! This is only auxiliary data, logic and database usage keep in Analyzer.
class BestScoreData
{
public:
        BestScoreData(std::size_t activeVersionIndex,
                      std::size_t musicId,
                      PlayStyle playStyle);

        std::size_t
        GetActiveVersion()
        const;

        const MusicScore &
        GetVersionBestMusicScore()
        const;

    //! @brief Initialize chart in music score, regardless have score or not.
        void
        RegisterActiveChart(Difficulty difficulty);

    //! @brief Update chart score by incremental date time.
    //! To avoid copy date time many times, dateTime is assumed to be incremental.
    //! @note Check if play count is incremental for each difficulty.
        void
        UpdateChartScore(Difficulty difficulty,
                         const std::string &dateTime,
                         const ChartScore &chartScore,
                         std::size_t playCount);

        const ChartScoreRecord*
        FindBestChartScoreRecord(BestScoreType bestScoreType,
                                 Difficulty difficulty)
        const;

    //! @brief Get best chart score record that differs from version best. May be nullptr.
    //! @note nullptr when:
    //!     1. version best miss is not available
    //!     2. target differ record is not available
    //!     So that for non-null return, can safely calculate difference.
    //!
    //! For example:
    //! S = version best, S1 = best score, S2 = second best score
    //! Any unavailable: nullptr.
    //! S == S1: return S2 or nullptr if S2 is not available.
    //! S1 > S: return S1
        const ChartScoreRecord*
        FindBestDifferChartScoreRecord(BestDifferType bestDifferType,
                                       Difficulty difficulty)
        const;

private:
    std::size_t mActiveVersionIndex{0};

    //! @brief Best score of current active version with play count.
    //! Update rule: last chart score for each difficulty in version, and max play count among records.
    //! @note Though it seems better to use earliest record when scores are same,
    //! consider miss count and other data update, it's better to use last data.
    //! For example:
    //! Time Difficulty Score Miss Clear PlayCount
    //! 1/2  SPH        100   1    HARD  3
    //! 1/2  SPA        200   10   EASY  3   <- Earliest SPA best EX Score.
    //! 1/3  SPA        200   3    HARD  4   <- Last SPA update
    //! 1/4  SPN        50    0    FC    5   <- Max play count
    //! @note DateTime is omitted, since each difficulty have different best chart score date time.
    MusicScore mVersionBestMusicScore;

    //! @brief Career best of each BestScoreType for each difficulty. Only record non-trivial scores.
    //! Map of {BestScoreType, Map of {Difficulty, CareerBestChartScore}}.
    //! Update rule:
    //!     ExScore: earliest highest ex score.
    //!     Second ExScore: earliest 2nd highest ex score.
    //!     Miss: earliest lowest miss if miss is available.
    //!     Second Miss: earliest 2nd lowest miss if miss is available.
    //! @note Career means among all versions, not limited to active version.
    //! e.g. active = 27, can search to 29 best score.
    //! @note Impl should ensure BestScoreType key always exist.
    //! Difficulty can be missing if difficulty have not updated, or no suitable data for best score type.
    std::map<BestScoreType, std::map<Difficulty, ChartScoreRecord>> mCareerBestRecords;

    //! @brief Help check if chart play count inside version is incremental.
    //! Map of {Difficulty, VersionPlayCount}.
    std::map<Difficulty, std::size_t> mChartVersionPlayCounts;

    //! @brief Records difficulty that has updated during date time of active version.
    std::set<Difficulty> mVersionUpdatedDifficultySet;

        ChartScoreRecord*
        FindNonConstBestChartScoreRecord(BestScoreType bestScoreType,
                                         Difficulty difficulty);
};

}
