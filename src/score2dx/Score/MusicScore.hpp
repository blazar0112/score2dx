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
    //! @note Verify if dateTime format is correct.
    //! (But not check if dateTime and version are consistent.)
        MusicScore(std::size_t versionIndex,
                   std::size_t musicIndex,
                   PlayStyle playStyle,
                   std::size_t playCount,
                   const std::string &dateTime);

        std::size_t
        GetVersionIndex()
        const;

        std::size_t
        GetMusicIndex()
        const;

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

        const std::string &
        GetDateTime()
        const;

        void
        AddChartScore(Difficulty difficulty,
                      const ChartScore &chartScore);

        const ChartScore*
        FindChartScore(Difficulty difficulty)
        const;

        const std::map<Difficulty, ChartScore> &
        GetChartScores()
        const;

        void
        Print()
        const;

private:
    std::size_t mVersionIndex{0};
    std::size_t mMusicIndex{0};

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
