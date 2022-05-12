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
    for (auto playStyle : PlayStyleSmartEnum::ToRange())
    {
        mMusicScores[playStyle];
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
AddMusicScore(const MusicScore &musicScore,
              std::size_t sourceVersionIndex)
{
    auto playStyle = musicScore.GetPlayStyle();
    auto musicId = musicScore.GetMusicId();
    auto &dateTime = musicScore.GetDateTime();

    if (auto findMusicId = ies::Find(mMusicScores.at(playStyle), musicId))
    {
        if (auto findMusicScore = ies::Find(findMusicId.value()->second, dateTime))
        {
            return;
        }
    }

    mMusicScores[playStyle][musicId].emplace(dateTime, musicScore);

    mVersionScoreTables[musicId].AddMusicScore(musicScore, sourceVersionIndex);
}

const std::map<size_t, std::map<std::string, MusicScore>> &
PlayerScore::
GetMusicScores(PlayStyle playStyle)
const
{
    return mMusicScores.at(playStyle);
}

void
PlayerScore::
AddChartScore(std::size_t musicId,
              PlayStyle playStyle,
              Difficulty difficulty,
              const std::string &dateTime,
              const ChartScore &chartScore)
{
    auto &allTimeMusicScores = mMusicScores[playStyle][musicId];
    auto findMusicScore = ies::Find(allTimeMusicScores, dateTime);
    if (!findMusicScore)
    {
        auto [versionIndex, musicIndex] = ToIndexes(musicId);
        allTimeMusicScores.emplace(
            std::piecewise_construct,
            std::forward_as_tuple(dateTime),
            std::forward_as_tuple(ToMusicId(versionIndex, musicIndex), playStyle, 0, dateTime)
        );
    }

    auto &musicScore = allTimeMusicScores.at(dateTime);
    musicScore.SetChartScore(difficulty, chartScore);
}

std::map<std::string, const ChartScore*>
PlayerScore::
GetChartScores(std::size_t musicId, PlayStyle playStyle, Difficulty difficulty)
const
{
    std::map<std::string, const ChartScore*> chartScores;

    auto findMusic = ies::Find(mMusicScores.at(playStyle), musicId);
    if (findMusic)
    {
        for (auto &[dateTime, musicScore] : findMusic.value()->second)
        {
            auto* chartScorePtr = musicScore.GetChartScore(difficulty);
            if (chartScorePtr)
            {
                chartScores[dateTime] = chartScorePtr;
            }
        }
    }

    return chartScores;
}

const std::map<std::size_t, VersionScoreTable> &
PlayerScore::
GetVersionScoreTables()
const
{
    return mVersionScoreTables;
}

}
