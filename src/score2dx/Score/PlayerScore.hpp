#pragma once

#include <map>
#include <memory>
#include <string>

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
        explicit PlayerScore(const std::string &iidxId);

        const std::string &
        GetIidxId()
        const;

    //! @brief Add MusicScore, usually from CSV data.
    //! @note Does nothing if exist MusicScore with same date time.
    //! (Not check if adding musicScore and existing MusicScore are same or not.)
        void
        AddMusicScore(const MusicScore &musicScore,
                      std::size_t sourceVersionIndex);

    //! @brief Map of {MusicId, Map of {DateTime, MusicScore}}.
        const std::map<size_t, std::map<std::string, MusicScore>> &
        GetMusicScores(PlayStyle playStyle)
        const;

    //! @brief Add ChartScore, because it's from iidxme data or manually input.
    //! @note Update if already exist StyleDifficulty's ChartScore in MusicScore.
        void
        AddChartScore(std::size_t musicId,
                      PlayStyle playStyle,
                      Difficulty difficulty,
                      const std::string &dateTime,
                      const ChartScore &chartScore);

    //! @brief Get all historic ChartScore of a styleDifficulty of musicId from CSV files,
    //! sorted by DateTime of CSV.
    //! @return Map of {DateTime, matching musicId style difficulty ChartScore}.
        std::map<std::string, const ChartScore*>
        GetChartScores(std::size_t musicId, PlayStyle playStyle, Difficulty difficulty)
        const;

private:
    std::string mIidxId;
    //! @brief Map of {PlayStyle, Map of {MusicId, Map of {DateTime, MusicScore}}}.
    std::map<PlayStyle, std::map<size_t, std::map<std::string, MusicScore>>> mMusicScores;

    VersionScoreTable mVersionScoreTable;
};

}
