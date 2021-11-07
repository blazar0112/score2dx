#include "score2dx/Score/PlayerScore.hpp"

#include <iostream>
#include <stdexcept>

#include "icl_s2/StdUtil/Find.hxx"
#include "icl_s2/Time/TimeUtilFormat.hxx"

namespace s2Time = icl_s2::Time;

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
AddMusicScore(const MusicScore &musicScore)
{
    auto playStyle = musicScore.GetPlayStyle();
    auto musicId = musicScore.GetMusicId();
    auto &dateTime = musicScore.GetDateTime();

    if (auto findMusicId = icl_s2::Find(mMusicScores.at(playStyle), musicId))
    {
        if (auto findMusicScore = icl_s2::Find(findMusicId.value()->second, dateTime))
        {
            return;
        }
    }

    mMusicScores[playStyle][musicId].emplace(dateTime, musicScore);
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
    auto findMusicScore = icl_s2::Find(allTimeMusicScores, dateTime);
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
    musicScore.AddChartScore(difficulty, chartScore);
}

std::map<std::string, const ChartScore*>
PlayerScore::
GetChartScores(std::size_t musicId, PlayStyle playStyle, Difficulty difficulty)
const
{
    std::map<std::string, const ChartScore*> chartScores;

    auto findMusic = icl_s2::Find(mMusicScores.at(playStyle), musicId);
    if (findMusic)
    {
        for (auto &[dateTime, musicScore] : findMusic.value()->second)
        {
            auto chartScore = musicScore.FindChartScore(difficulty);
            if (chartScore)
            {
                chartScores[dateTime] = chartScore;
            }
        }
    }

    return chartScores;
}

}
