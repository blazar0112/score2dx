#include "score2dx/Score/AllVersionScoreTable.hpp"

#include "score2dx/Iidx/Version.hpp"

#include "ies/Common/IntegralRangeUsing.hpp"

namespace score2dx
{

AllVersionScoreTable::
AllVersionScoreTable(const Music& music)
:   mMusic(music)
,   mMusicId(music.GetMusicId())
{
    mAllVersionTimeLineScoreTable.resize(VersionNames.size());
}

void
AllVersionScoreTable::
AddMusicScore(
    std::size_t scoreVersionIndex,
    const MusicScore& musicScore)
{
    if (musicScore.GetMusicId()!=mMusicId)
    {
        throw std::runtime_error("musicScore.GetMusicId()!=mMusicId");
    }

    auto& dateTime = musicScore.GetDateTime();

    //'' note: timeline table record origin datetime since it's organized by version.
    mAllVersionTimeLineScoreTable[scoreVersionIndex].emplace(dateTime, musicScore);
}

const std::map<std::string, MusicScore>&
AllVersionScoreTable::
GetMusicScores(std::size_t scoreVersionIndex)
const
{
    if (scoreVersionIndex>=VersionNames.size())
    {
        throw std::runtime_error("scoreVersionIndex out of bound.");
    }

    return mAllVersionTimeLineScoreTable[scoreVersionIndex];
}

const ChartScore*
AllVersionScoreTable::
GetBestChartScore(
    std::size_t scoreVersionIndex,
    Difficulty difficulty)
const
{
    auto& musicScores = GetMusicScores(scoreVersionIndex);
    if (musicScores.empty()) { return nullptr; }

    auto &bestMusicScore = musicScores.rbegin()->second;
    auto* lastChartScore = bestMusicScore.GetChartScore(difficulty);
    if (lastChartScore)
    {
        return lastChartScore;
    }

    const ChartScore* bestChartScore = nullptr;
    for (auto &[dateTime, musicScore] : musicScores)
    {
        auto* chartScore = musicScore.GetChartScore(difficulty);
        if (chartScore)
        {
            bestChartScore = chartScore;
        }
    }

    return bestChartScore;
}

void
AllVersionScoreTable::
CleanupInVersionScores()
{
    for (auto scoreVersionIndex : IndexRange{0, mAllVersionTimeLineScoreTable.size()})
    {
        auto& versionScores = mAllVersionTimeLineScoreTable[scoreVersionIndex];
        std::size_t currentCount = 0;
        for (auto& [dateTime, musicScore] : versionScores)
        {
            auto count = musicScore.GetEnableCount();
            currentCount = count;
            (void)currentCount;
        }
    }
}

std::string
AdjustDateTime(
    std::size_t scoreVersionIndex,
    const std::string& dateTime)
{
    if (dateTime.empty())
    {
        throw std::runtime_error("requires non-empty datetime.");
    }

    if (dateTime<GetVersionDateTimeRange(GetFirstSupportDateTimeVersionIndex()).Get(ies::RangeSide::Begin))
    {
        throw std::runtime_error("datetime before first support version.");
    }

    auto &versionDateTimeRange = GetVersionDateTimeRange(scoreVersionIndex);
    if (dateTime<versionDateTimeRange.Get(ies::RangeSide::Begin))
    {
        throw std::runtime_error("datetime before source version.");
    }

    auto &versionEnd = versionDateTimeRange.Get(ies::RangeSide::End);
    if (!versionEnd.empty() && dateTime>versionEnd)
    {
        return versionEnd;
    }

    return dateTime;
}

}
