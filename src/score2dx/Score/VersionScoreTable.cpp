#include "score2dx/Score/VersionScoreTable.hpp"

#include <iostream>

#include "score2dx/Iidx/Version.hpp"

namespace score2dx
{

VersionScoreTable::
VersionScoreTable()
{
    for (auto playStyle : PlayStyleSmartEnum::ToRange())
    {
        mScoreTimeLineTable[static_cast<std::size_t>(playStyle)].resize(VersionNames.size());
    }
}

void
VersionScoreTable::
AddMusicScore(const MusicScore &musicScore, std::size_t sourceVersionIndex)
{
    auto playStyleIndex = static_cast<std::size_t>(musicScore.GetPlayStyle());
    auto &dateTime = musicScore.GetDateTime();

    if (dateTime.empty())
    {
        throw std::runtime_error("requires non-empty datetime.");
    }

    if (dateTime<GetVersionDateTimeRange(GetFirstSupportDateTimeVersionIndex()).at(ies::RangeSide::Begin))
    {
        throw std::runtime_error("cannot add music score because datetime before first support version.");
    }

    auto versionDateTimeRange = GetVersionDateTimeRange(sourceVersionIndex);
    if (dateTime<versionDateTimeRange.at(ies::RangeSide::Begin))
    {
        throw std::runtime_error("cannot add music score because datetime before source version.");
    }

    auto [it, flag] = mScoreTimeLineTable[playStyleIndex][sourceVersionIndex].emplace(dateTime, musicScore);
    auto &tableMusicScore = it->second;

    auto &versionEnd = versionDateTimeRange.at(ies::RangeSide::End);
    if (!versionEnd.empty() && dateTime>versionEnd)
    {
        std::cout << "adjust music " << ToMusicIdString(musicScore.GetMusicId())
                  << " from [" << dateTime
                  << "] to end of source version [" << versionEnd
                  << "].\n";
        tableMusicScore.SetDateTime(versionEnd);
    }
}

const std::map<std::string, MusicScore> &
VersionScoreTable::
GetMusicScores(PlayStyle playStyle, std::size_t versionIndex)
const
{
    auto playStyleIndex = static_cast<std::size_t>(playStyle);

    if (versionIndex>=VersionNames.size())
    {
        throw std::runtime_error("version index out of bound.");
    }

    return mScoreTimeLineTable[playStyleIndex][versionIndex];
}

}
