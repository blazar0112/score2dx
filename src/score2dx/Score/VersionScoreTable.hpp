#pragma once

#include <array>
#include <map>
#include <string>
#include <vector>

#include "score2dx/Iidx/Definition.hpp"
#include "score2dx/Score/MusicScore.hpp"

namespace score2dx
{

//! @brief Manage a music's MusicScore by version.
class VersionScoreTable
{
public:
        explicit VersionScoreTable(std::size_t musicId);

    //! @brief Add MusicScore from sourceVersion.
    //! @note Because it's possible to play older version before inherit data.
    //! If score datetime is after scoreVersionIndex, then it's recognize as from scoreVersionIndex
    //! and adjust to end of sourceVersion.
    //! MusicScore's datetime cannot be version before score version.
    //! TimeLine use origin datetime as key to keep original score order.
    //! (need consider if play old version machine after new version released before transfering data)
        void
        AddMusicScore(std::size_t scoreVersionIndex,
                      const MusicScore &musicScore);

    //! @brief Add ChartScore from sourceVersion.
    //! @note Will adjust MusicScore datetime with same rule described in AddMusicScore.
        void
        AddChartScore(std::size_t scoreVersionIndex,
                      const std::string &dateTime,
                      PlayStyle playStyle,
                      Difficulty difficulty,
                      const ChartScore &chartScore);

    //! @brief Get all music score in version by timeline.
    //! @note MusicScore datetime may adjust to version end if exceeds version end.
    //! However timeline key is unadjusted datetime.
        const std::map<std::string, MusicScore> &
        GetMusicScores(std::size_t scoreVersionIndex,
                       PlayStyle playStyle)
        const;

        const ChartScore*
        GetBestChartScore(std::size_t scoreVersionIndex,
                          PlayStyle playStyle,
                          Difficulty difficulty)
        const;

private:
    std::size_t mMusicId;
    //! @brief Array of {Index=PlayStyle, Vector of {Index=ScoreVersionIndex, Map of {OriginDateTime, MusicScore}}}.
    std::array<std::vector<std::map<std::string, MusicScore>>, PlayStyleSmartEnum::Size()> mScoreTimeLineTable;
};

std::string
AdjustDateTime(std::size_t scoreVersionIndex,
               const std::string &dateTime);

}
