#include "score2dx/Score/MusicScore.hpp"

#include <iostream>

#include "ies/StdUtil/Find.hxx"

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
SetDateTime(const std::string &dateTime)
{
    mDateTime = dateTime;
}

ChartScore &
MusicScore::
EnableChartScore(Difficulty difficulty)
{
    auto index = static_cast<std::size_t>(difficulty);
    mEnables[index] = true;
    return mChartScores[index];
}

void
MusicScore::
SetChartScore(Difficulty difficulty,
              const ChartScore &chartScore)
{
    auto index = static_cast<std::size_t>(difficulty);
    mChartScores[index] = chartScore;
    EnableChartScore(difficulty);
}

void
MusicScore::
ResetChartScore(Difficulty difficulty)
{
    auto index = static_cast<std::size_t>(difficulty);
    mEnables[index] = false;
    mChartScores[index] = ChartScore{};
}

const ChartScore*
MusicScore::
GetChartScore(Difficulty difficulty)
const
{
    auto index = static_cast<std::size_t>(difficulty);
    if (mEnables[index])
    {
        return &mChartScores[index];
    }
    return nullptr;
}

ChartScore*
MusicScore::
GetChartScore(Difficulty difficulty)
{
    return const_cast<ChartScore*>(std::as_const(*this).GetChartScore(difficulty));
}

std::map<Difficulty, const ChartScore*>
MusicScore::
GetChartScores()
const
{
    std::map<Difficulty, const ChartScore*> chartScores;
    for (auto difficulty : DifficultySmartEnum::ToRange())
    {
        auto index = static_cast<std::size_t>(difficulty);
        if (mEnables[index])
        {
            chartScores[difficulty] = &mChartScores[index];
        }
    }
    return chartScores;
}

void
MusicScore::
Print()
const
{
    std::cout << "MusicScore ["+ToMusicIdString(GetMusicId())+"]["+GetDateTime()+"]:\n"
              << "PlayCount: " << mPlayCount << "\n";
    for (auto &[difficulty, chartScore] : GetChartScores())
    {
        auto styleDifficulty = ConvertToStyleDifficulty(GetPlayStyle(), difficulty);
        std::cout   << "["+ToString(styleDifficulty)+"]: "
                    << ToString(chartScore->ClearType)+"|"
                    << ToString(chartScore->DjLevel)+"|"
                    << std::to_string(chartScore->ExScore)
                    << "\n";
    }
}

}
