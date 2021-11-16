#pragma once

#include <map>

#include "icl_s2/Common/SmartEnum.hxx"

#include "score2dx/Iidx/Definition.hpp"
#include "score2dx/Score/ChartScore.hpp"

namespace score2dx
{

//! @brief MusicScore stores a Music's a PlayStyle-DateTime's all difficulty chart scores.
//! @note Each line in CSV, by definition, is a MusicScore.
class MusicScore
{
public:
    //! @note DateTime should be valid if construct from CSV.
    //! For other purpose it can be empty.
        MusicScore(std::size_t musicId,
                   PlayStyle playStyle,
                   std::size_t playCount,
                   std::string dateTime);

    //! @brief MusicId = VersionIndex*1000+{MusicIndex in version music list}.
    //! e.g. "Elisha"'s VersionIndex is 17, MusicIndex is 0, MusicId is 17000.
        std::size_t
        GetMusicId()
        const;

        PlayStyle
        GetPlayStyle()
        const;

        std::size_t
        GetPlayCount()
        const;

        void
        SetPlayCount(std::size_t playCount);

        const std::string &
        GetDateTime()
        const;

    //! @brief Add chart score for difficulty, overwrite if exist.
        void
        AddChartScore(Difficulty difficulty,
                      const ChartScore &chartScore);

        const ChartScore*
        FindChartScore(Difficulty difficulty)
        const;

        ChartScore*
        FindChartScore(Difficulty difficulty);

        const std::map<Difficulty, ChartScore> &
        GetChartScores()
        const;

        void
        Print()
        const;

private:
    //! @brief MusicId = VersionIndex*1000+{MusicIndex in version music list}.
    //! e.g. "Elisha"'s VersionIndex is 17, MusicIndex is 0, MusicId is 17000.
    std::size_t mMusicId{0};

    PlayStyle mPlayStyle{PlayStyle::SinglePlay};

    std::size_t mPlayCount{0};

    //! @brief DateTime must in format of "YYYY-MM-DD HH:MM"  e.g. "2020-08-22 18:51".
    std::string mDateTime;

    std::map<Difficulty, ChartScore> mChartScores;
};

}
