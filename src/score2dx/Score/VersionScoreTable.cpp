#include "score2dx/Score/VersionScoreTable.hpp"

#include <iostream>

#include "score2dx/Iidx/Version.hpp"

namespace score2dx
{

VersionScoreTable::
VersionScoreTable(std::size_t musicId)
:   mMusicId(musicId)
{
    for (auto playStyle : PlayStyleSmartEnum::ToRange())
    {
        mScoreTimeLineTable[static_cast<std::size_t>(playStyle)].resize(VersionNames.size());
    }
}

void
VersionScoreTable::
AddMusicScore(std::size_t scoreVersionIndex,
              const MusicScore &musicScore)
{
    if (musicScore.GetMusicId()!=mMusicId)
    {
        throw std::runtime_error("musicScore.GetMusicId()!=mMusicId");
    }

    auto playStyleIndex = static_cast<std::size_t>(musicScore.GetPlayStyle());
    auto &dateTime = musicScore.GetDateTime();

    auto adjustedDateTime = AdjustDateTime(scoreVersionIndex, dateTime);

    //'' note: timeline table record origin datetime since it's organized by version.
    auto [it, flag] = mScoreTimeLineTable[playStyleIndex][scoreVersionIndex].emplace(dateTime, musicScore);
    auto &tableMusicScore = it->second;

    tableMusicScore.SetDateTime(adjustedDateTime);
}

void
VersionScoreTable::
AddChartScore(std::size_t scoreVersionIndex,
              const std::string &dateTime,
              PlayStyle playStyle,
              Difficulty difficulty,
              const ChartScore &chartScore)
{
    auto playStyleIndex = static_cast<std::size_t>(playStyle);
    auto adjustedDateTime = AdjustDateTime(scoreVersionIndex, dateTime);

    auto [it, flag] = mScoreTimeLineTable[playStyleIndex][scoreVersionIndex].emplace
    (
        std::piecewise_construct,
        std::forward_as_tuple(dateTime),
        std::forward_as_tuple(mMusicId, playStyle, 0, dateTime)
    );
    auto &musicScore = it->second;
    musicScore.SetChartScore(difficulty, chartScore);
    musicScore.SetDateTime(adjustedDateTime);
}

const std::map<std::string, MusicScore> &
VersionScoreTable::
GetMusicScores(std::size_t scoreVersionIndex,
               PlayStyle playStyle)
const
{
    auto playStyleIndex = static_cast<std::size_t>(playStyle);

    if (scoreVersionIndex>=VersionNames.size())
    {
        throw std::runtime_error("scoreVersionIndex out of bound.");
    }

    return mScoreTimeLineTable[playStyleIndex][scoreVersionIndex];
}

std::string
AdjustDateTime(std::size_t scoreVersionIndex,
               const std::string &dateTime)
{
    if (dateTime.empty())
    {
        throw std::runtime_error("requires non-empty datetime.");
    }

    if (dateTime<GetVersionDateTimeRange(GetFirstSupportDateTimeVersionIndex()).at(ies::RangeSide::Begin))
    {
        throw std::runtime_error("datetime before first support version.");
    }

    auto versionDateTimeRange = GetVersionDateTimeRange(scoreVersionIndex);
    if (dateTime<versionDateTimeRange.at(ies::RangeSide::Begin))
    {
        throw std::runtime_error("datetime before source version.");
    }

    auto &versionEnd = versionDateTimeRange.at(ies::RangeSide::End);
    if (!versionEnd.empty() && dateTime>versionEnd)
    {
        return versionEnd;
    }

    return dateTime;
}

}
