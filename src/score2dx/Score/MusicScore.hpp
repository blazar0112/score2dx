#pragma once

#include <array>
#include <map>

#include "ies/Common/SmartEnum.hxx"

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

        void
        SetDateTime(const std::string &dateTime);

    //! @brief ChartScore is default disabled, enable to use.
        ChartScore &
        EnableChartScore(Difficulty difficulty);

    //! @brief Overwrite difficulty's ChartScore and enable it.
        void
        SetChartScore(Difficulty difficulty,
                      const ChartScore &chartScore);

    //! @brief Disable difficulty and reset its ChartScore.
        void
        ResetChartScore(Difficulty difficulty);

    //! @brief Get ChartScore if it is enabled, otherwise nullptr.
        const ChartScore*
        GetChartScore(Difficulty difficulty)
        const;

    //! @brief Get ChartScore if it is enabled, otherwise nullptr.
        ChartScore*
        GetChartScore(Difficulty difficulty);

    //! @brief Get enabled ChartScores.
        std::map<Difficulty, const ChartScore*>
        GetChartScores()
        const;

        void
        Print()
        const;

        std::size_t
        GetEnableCount()
        const;

private:
    //! @brief MusicId = VersionIndex*1000+{MusicIndex in version music list}.
    //! e.g. "Elisha"'s VersionIndex is 17, MusicIndex is 0, MusicId is 17000.
    std::size_t mMusicId{0};

    PlayStyle mPlayStyle{PlayStyle::SinglePlay};

    std::size_t mPlayCount{0};

    //! @brief DateTime must in format of "YYYY-MM-DD HH:MM"  e.g. "2020-08-22 18:51".
    std::string mDateTime;

    std::array<ChartScore, DifficultySmartEnum::Size()> mChartScores;
    std::array<bool, DifficultySmartEnum::Size()> mEnables{};
};

}
