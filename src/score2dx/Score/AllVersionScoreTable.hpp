#pragma once

#include <map>
#include <string>
#include <vector>

#include "score2dx/Iidx/Definition.hpp"
#include "score2dx/Iidx/Music.hpp"
#include "score2dx/Score/MusicScore.hpp"

namespace score2dx
{

//! @brief Manage a Music's all version MusicScores.
//! A MusicScore needs to identify its ScoreVersionIndex, because datetime is only a
//! suggestion, player can have score later then release date.
//! (Or in rare cases, the Pendual "preview" system).
class AllVersionScoreTable
{
public:
    explicit AllVersionScoreTable(const Music& music);

    //! @brief Add MusicScore from sourceVersion.
    //! @note Because it's possible to play older version before inherit data.
    //! If score datetime is after scoreVersionIndex, then it's recognize as from scoreVersionIndex
    //! and adjust to end of sourceVersion.
    //! MusicScore's datetime cannot be version before score version.
    //! TimeLine use origin datetime as key to keep original score order.
    //! (need consider if play old version machine after new version released before transfering data)
    void
    AddMusicScore(
        std::size_t scoreVersionIndex,
        const MusicScore& musicScore);

    void
    Compile();

    //! @brief Get all music score in version by timeline.
    //! @note MusicScore datetime may adjust to version end if exceeds version end.
    //! However timeline key is unadjusted datetime.
    const std::map<std::string, MusicScore>&
    GetMusicScores(std::size_t scoreVersionIndex)
    const;

    const ChartScore*
    GetBestChartScore(
        std::size_t scoreVersionIndex,
        Difficulty difficulty)
    const;

    //! @brief Check inconsistency score in each version (does not propagate) and remove inconsistent or redundant music score.
    //! @note Because how Export is written, there maybe MusicScore only store a single ChartScore.
    //! This function propagate ChartScore inside a version, cross version clear type will propagate later.
    //! e.g.     N H A
    //! t1         c1
    //! t2           c2
    //! t3         c3
    //! will be modified to like CSV updates
    //! t1         c1
    //! t2         c1c2
    //! t3         c3c2
    void
    CleanupInVersionScores();

private:
    const Music& mMusic;
    std::size_t mMusicId;

    //! @brief Vector of {Index=ScoreVersionIndex, Map of {OriginDateTime, MusicScore}}.
    std::vector<std::map<std::string, MusicScore>> mAllVersionTimeLineScoreTable;
};

std::string
AdjustDateTime(
    std::size_t scoreVersionIndex,
    const std::string& dateTime);

}
