#include "score2dx/Score/MusicScore.hpp"

#include <iostream>

#include "icl_s2/StdUtil/Find.hxx"

namespace score2dx
{

MusicScore::
MusicScore(std::size_t musicId,
           PlayStyle playStyle,
           std::size_t playCount,
           std::string dateTime)
:   mMusicId(musicId),
    mPlayStyle(playStyle),
    mPlayCount(playCount),
    mDateTime(std::move(dateTime))
{
    //! @todo: check date time.
}

std::size_t
MusicScore::
GetMusicId()
const
{
    return mMusicId;
}

PlayStyle
MusicScore::
GetPlayStyle()
const
{
    return mPlayStyle;
}

std::size_t
MusicScore::
GetPlayCount()
const
{
    return mPlayCount;
}

void
MusicScore::
SetPlayCount(std::size_t playCount)
{
    mPlayCount = playCount;
}

const std::string &
MusicScore::
GetDateTime()
const
{
    return mDateTime;
}

void
MusicScore::
AddChartScore(Difficulty difficulty,
              const ChartScore &chartScore)
{
    mChartScores[difficulty] = chartScore;
}

const ChartScore*
MusicScore::
FindChartScore(Difficulty difficulty)
const
{
    if (auto findScore = icl_s2::Find(mChartScores, difficulty))
    {
        return &(findScore.value()->second);
    }
    return nullptr;
}

ChartScore*
MusicScore::
FindChartScore(Difficulty difficulty)
{
    return const_cast<ChartScore*>(std::as_const(*this).FindChartScore(difficulty));
}

const std::map<Difficulty, ChartScore> &
MusicScore::
GetChartScores()
const
{
    return mChartScores;
}

void
MusicScore::
Print()
const
{
    std::cout << "MusicScore ["+ToFormatted(GetMusicId())+"]["+GetDateTime()+"]:\n"
              << "PlayCount: " << mPlayCount << "\n";
    for (auto &[difficulty, chartScore] : mChartScores)
    {
        auto styleDifficulty = ConvertToStyleDifficulty(GetPlayStyle(), difficulty);
        std::cout   << "["+ToString(styleDifficulty)+"]: "
                    << ToString(chartScore.ClearType)+"|"
                    << ToString(chartScore.DjLevel)+"|"
                    << std::to_string(chartScore.ExScore)
                    << "\n";
    }
}

}
