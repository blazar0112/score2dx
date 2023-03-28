#pragma once

#include <map>
#include <memory>
#include <string>

#include "score2dx/Core/MusicDatabase.hpp"
#include "score2dx/Iidx/Definition.hpp"
#include "score2dx/Score/ChartScore.hpp"
#include "score2dx/Score/MusicScore.hpp"
#include "score2dx/Score/VersionScoreTable.hpp"

namespace score2dx
{

//! @brief PlayerScore stores a history score data of a player.
class PlayerScore
{
public:
        PlayerScore(const MusicDatabase &musicDatabase, const std::string &iidxId);

        const std::string &
        GetIidxId()
        const;

    //! @brief Add MusicScore, usually from CSV data.
    //! @note Does nothing if exist MusicScore with same date time.
    //! (Not check if adding musicScore and existing MusicScore are same or not.)
        void
        AddMusicScore(std::size_t scoreVersionIndex,
                      const MusicScore &musicScore);

/*
    //! @brief Add ChartScore, because it's from iidxme data or manually input.
    //! @note Update if already exist StyleDifficulty's ChartScore in MusicScore.
        void
        AddChartScore(std::size_t scoreVersionIndex,
                      std::size_t musicId,
                      PlayStyle playStyle,
                      Difficulty difficulty,
                      const std::string &dateTime,
                      const ChartScore &chartScore);
*/

    //! @brief Propagate clear mark since AddMusic/ChartScore does not propagate now.
    //! Use after add all scores.
        void
        Propagate();

    //! @brief Get VersionScoreTables: Map of {MusicId, VersionScoreTable}.
        const std::map<std::size_t, VersionScoreTable> &
        GetVersionScoreTables()
        const;

private:
    const MusicDatabase &mMusicDatabase;
    std::string mIidxId;

    //! @brief Map of {MusicId, VersionScoreTable}.
    std::map<std::size_t, VersionScoreTable> mVersionScoreTables;

    //! @brief Progate same containing versions' score clear type.
        void
        PropagateClear(std::size_t musicId,
                       PlayStyle playStyle);
};

}
