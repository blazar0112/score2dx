#include "score2dx/Score/PlayerScore.hpp"

#include <iostream>
#include <stdexcept>

#include "ies/StdUtil/Find.hxx"
#include "ies/Time/TimeUtilFormat.hxx"

namespace s2Time = ies::Time;

namespace score2dx
{

PlayerScore::
PlayerScore(const std::string &iidxId)
:   mIidxId(iidxId)
{
    if (!IsIidxId(iidxId))
    {
        throw std::runtime_error("["+iidxId+"] is not a valid IIDX ID.");
    }
}

const std::string &
PlayerScore::
GetIidxId()
const
{
    return mIidxId;
}

void
PlayerScore::
AddMusicScore(std::size_t scoreVersionIndex,
              const MusicScore &musicScore)
{
    auto musicId = musicScore.GetMusicId();
    auto [it, flag] = mVersionScoreTables.emplace(musicId, musicId);
    auto &scoreTable = it->second;
    scoreTable.AddMusicScore(scoreVersionIndex, musicScore);
}

void
PlayerScore::
AddChartScore(std::size_t scoreVersionIndex,
              std::size_t musicId,
              PlayStyle playStyle,
              Difficulty difficulty,
              const std::string &dateTime,
              const ChartScore &chartScore)
{
    auto [it, flag] = mVersionScoreTables.emplace(musicId, musicId);
    auto &scoreTable = it->second;
    scoreTable.AddChartScore(scoreVersionIndex, dateTime, playStyle, difficulty, chartScore);
}

const std::map<std::size_t, VersionScoreTable> &
PlayerScore::
GetVersionScoreTables()
const
{
    return mVersionScoreTables;
}

}
