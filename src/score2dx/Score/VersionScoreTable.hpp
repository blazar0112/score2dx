#pragma once

#include <array>
#include <map>
#include <string>
#include <vector>

#include "score2dx/Iidx/Definition.hpp"
#include "score2dx/Score/MusicScore.hpp"

namespace score2dx
{

class VersionScoreTable
{
public:
        VersionScoreTable();

    //! @brief Add MusicScore from sourceVersion.
    //! @note Because it's possible to play older version before inherit data.
    //! If score version is after sourceVersionIndex, then it's recognize as from sourceVersion
    //! and adjust to end of sourceVersion.
    //! Version of MusicScore datetime cannot be before source version.
        void
        AddMusicScore(const MusicScore &musicScore, std::size_t sourceVersionIndex);

        const std::map<std::string, MusicScore> &
        GetMusicScores(PlayStyle playStyle, std::size_t versionIndex)
        const;

private:
    //! @brief Array of {Index=PlayStyle, Vector of {Index=ScoreSourceVersionIndex, Map of {OriginDateTime, MusicScore}}}.
    std::array<std::vector<std::map<std::string, MusicScore>>, PlayStyleSmartEnum::Size()> mScoreTimeLineTable;

    std::string mFirstSupportVersionBeginDateTime;
};

}
