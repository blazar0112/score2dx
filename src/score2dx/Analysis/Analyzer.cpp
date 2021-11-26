#include "score2dx/Analysis/Analyzer.hpp"

#include <iostream>

#include "icl_s2/Common/IntegralRangeUsing.hpp"
#include "icl_s2/StdUtil/Find.hxx"
#include "icl_s2/Time/TimeUtilFormat.hxx"

#include "score2dx/Iidx/Version.hpp"
#include "score2dx/Score/ScoreLevel.hpp"

namespace s2Time = icl_s2::Time;

namespace score2dx
{

Statistics::
Statistics()
{
    for (auto clearType : ClearTypeSmartEnum::ToRange())
    {
        ChartIdListByClearType[clearType];
    }

    for (auto djLevel : DjLevelSmartEnum::ToRange())
    {
        ChartIdListByDjLevel[djLevel];
    }

    for (auto scoreLevelCategory : ScoreLevelCategorySmartEnum::ToRange())
    {
        ChartIdListByScoreLevelCategory[scoreLevelCategory];
    }
}

Analyzer::
Analyzer(const MusicDatabase &musicDatabase)
:   mMusicDatabase(musicDatabase)
{
    SetActiveVersionIndex(GetLatestVersionIndex());
}

void
Analyzer::
SetActiveVersionIndex(std::size_t activeVersionIndex)
{
    if (!icl_s2::Find(mMusicDatabase.GetActiveVersions(), activeVersionIndex))
    {
        throw std::runtime_error("ActiveVersion "+std::to_string(activeVersionIndex)+" is not supported.");
    }

    mActiveVersionIndex = activeVersionIndex;
}

std::size_t
Analyzer::
GetActiveVersionIndex()
const
{
    return mActiveVersionIndex;
}

ScoreAnalysis
Analyzer::
Analyze(const PlayerScore &playerScore)
const
{
    auto begin = s2Time::Now();

    ScoreAnalysis analysis;

    for (auto playStyle : PlayStyleSmartEnum::ToRange())
    {
        analysis.StatisticsByStyle[playStyle];
        analysis.StatisticsByStyleLevel[playStyle];
        analysis.ActivityByDate[playStyle];
    }

    for (auto styleDifficulty : StyleDifficultySmartEnum::ToRange())
    {
        if (styleDifficulty==StyleDifficulty::SPB||styleDifficulty==StyleDifficulty::DPB)
        {
            continue;
        }
        analysis.StatisticsByStyleDifficulty[styleDifficulty];
    }

    analysis.StatisticsByVersionStyle.resize(mActiveVersionIndex+1);
    analysis.StatisticsByVersionStyleDifficulty.resize(mActiveVersionIndex+1);
    for (auto versionIndex : IndexRange{0, mActiveVersionIndex+1})
    {
        for (auto playStyle : PlayStyleSmartEnum::ToRange())
        {
            analysis.StatisticsByVersionStyle[versionIndex][playStyle];
        }

        for (auto styleDifficulty : StyleDifficultySmartEnum::ToRange())
        {
            if (styleDifficulty==StyleDifficulty::SPB||styleDifficulty==StyleDifficulty::DPB)
            {
                continue;
            }
            analysis.StatisticsByVersionStyleDifficulty[versionIndex][styleDifficulty];
        }
    }

    auto* activeVersionPtr = mMusicDatabase.FindActiveVersion(mActiveVersionIndex);
    if (!activeVersionPtr)
    {
        throw std::runtime_error("invalid active version");
    }

    auto &activeVersion = *activeVersionPtr;

    auto versionDateTimeRange = GetVersionDateTimeRange(mActiveVersionIndex);
    auto &versionBeginDateTime = versionDateTimeRange.at(icl_s2::RangeSide::Begin);

    for (auto &[chartId, chartInfo] : activeVersion.GetChartInfos())
    {
        auto [musicId, chartPlayStyle, difficulty] = ToMusicStyleDiffculty(chartId);
        for (auto playStyle : PlayStyleSmartEnum::ToRange())
        {
            analysis.MusicBestScoreData[musicId].emplace(
                std::piecewise_construct,
                std::forward_as_tuple(playStyle),
                std::forward_as_tuple(mActiveVersionIndex, musicId, playStyle)
            );
        }

        auto styleDifficulty = ConvertToStyleDifficulty(chartPlayStyle, difficulty);

        if (chartInfo.Note<=0)
        {
            std::cout << "[" << ToMusicIdString(musicId)
                      << "][" << mMusicDatabase.GetLatestMusicInfo(musicId).GetField(MusicInfoField::Title)
                      << "][" << ToString(styleDifficulty)
                      << "] Note is non-positive\nLevel: " << chartInfo.Level
                      << ", Note: " << chartInfo.Note
                      << ".\n";
            continue;
        }

        auto findVersionBeginChartScore = FindChartScoreAtTime(playerScore, musicId, chartPlayStyle, difficulty, versionBeginDateTime);
        if (!findVersionBeginChartScore)
        {
            std::cout << "Cannot find version begin chart score.\n";
            throw std::runtime_error("not findVersionBeginChartScore");
        }
        auto &versionBeginChartScore = findVersionBeginChartScore.value();

        auto &bestScoreData = analysis.MusicBestScoreData[musicId].at(chartPlayStyle);
        bestScoreData.InitializeVersionBeginChartScore(difficulty, versionBeginChartScore);

        auto &musicScores = playerScore.GetMusicScores(chartPlayStyle);
        auto findMusicId = icl_s2::Find(musicScores, musicId);
        if (findMusicId)
        {
            auto &musicScoreByDateTime = findMusicId.value()->second;

            for (auto &[dateTime, musicScore] : musicScoreByDateTime)
            {
                //'' It is possible that some chart is only available at certain date time after version begin.
                //'' for example: arena SPL added later.
                auto* chartScorePtr = musicScore.FindChartScore(difficulty);
                if (!chartScorePtr) { continue; }
                auto &chartScore = *chartScorePtr;

                bestScoreData.UpdateChartScore(difficulty, dateTime, chartScore, musicScore.GetPlayCount());

                auto tokens = icl_s2::SplitString(" ", dateTime);
                auto &isoDate = tokens[0];
                analysis.ActivityByDate[chartPlayStyle].emplace(isoDate);
            }
        }

        auto* verBestChartScorePtr = bestScoreData.GetVersionBestMusicScore().FindChartScore(difficulty);
        if (!verBestChartScorePtr) { throw std::runtime_error("verBestChartScorePtr is nullptr"); }
        auto &versionBestChartScore = *verBestChartScorePtr;

        auto [scoreLevel, scoreRange] = FindScoreLevelRange(chartInfo.Note, versionBestChartScore.ExScore);
        auto category = ScoreLevelCategory::AMinus;
        if (scoreLevel>=ScoreLevel::A)
        {
            if (scoreLevel==ScoreLevel::AA) { category = ScoreLevelCategory::AAMinus; }
            if (scoreLevel==ScoreLevel::AAA) { category = ScoreLevelCategory::AAAMinus; }
            if (scoreLevel==ScoreLevel::Max) { category = ScoreLevelCategory::MaxMinus; }
            if (scoreRange!=ScoreRange::LevelMinus)
            {
                category = static_cast<ScoreLevelCategory>(static_cast<int>(category)+1);
            }
        }

        auto versionIndex = ToIndexes(musicId).first;
        if (versionIndex>mActiveVersionIndex)
        {
            throw std::runtime_error("versionIndex>mActiverVersionIndex");
        }

        if (styleDifficulty==StyleDifficulty::SPB||styleDifficulty==StyleDifficulty::DPB)
        {
            continue;
        }

        std::set<Statistics*> analysisStatsList
        {
            &analysis.StatisticsByStyle[chartPlayStyle],
            &analysis.StatisticsByStyleLevel[chartPlayStyle][chartInfo.Level],
            &analysis.StatisticsByStyleDifficulty[styleDifficulty],
            &analysis.StatisticsByVersionStyle[versionIndex][chartPlayStyle],
            &analysis.StatisticsByVersionStyleDifficulty[versionIndex][styleDifficulty]
        };

        for (auto stats : analysisStatsList)
        {
            stats->ChartIdList.emplace(chartId);
            stats->ChartIdListByClearType[versionBestChartScore.ClearType].emplace(chartId);
            if (versionBestChartScore.ClearType!=ClearType::NO_PLAY
                &&versionBestChartScore.ExScore!=0)
            {
                stats->ChartIdListByDjLevel[versionBestChartScore.DjLevel].emplace(chartId);
                stats->ChartIdListByScoreLevelCategory[category].emplace(chartId);
            }
        }
    }

    for (auto versionIndex : IndexRange{GetFirstDateTimeAvailableVersionIndex(), GetLatestVersionIndex()+1})
    {
        auto versionDateTimeRange = GetVersionDateTimeRange(versionIndex);
        auto &versionBeginDateTime = versionDateTimeRange.at(icl_s2::RangeSide::Begin);
        auto tokens = icl_s2::SplitString(" ", versionBeginDateTime);
        auto &isoDate = tokens[0];

        for (auto playStyle : PlayStyleSmartEnum::ToRange())
        {
            analysis.ActivityByDate[playStyle].emplace(isoDate);
        }
    }

    s2Time::Print<std::chrono::milliseconds>(s2Time::CountNs(begin), "Analyze");

    return analysis;
}

ActivityAnalysis
Analyzer::
AnalyzeVersionActivity(const PlayerScore &playerScore)
const
{
    std::cout << "AnalyzeVersionActivity Ver [" << ToVersionString(mActiveVersionIndex) << "]\n";
    auto timeRange = GetVersionDateTimeRange(mActiveVersionIndex);
    return AnalyzeActivity(playerScore, timeRange.at(icl_s2::RangeSide::Begin), timeRange.at(icl_s2::RangeSide::End));
}

ActivityAnalysis
Analyzer::
AnalyzeActivity(const PlayerScore &playerScore,
                const std::string &beginDateTime,
                const std::string &endDateTime)
const
{
    auto begin = s2Time::Now();
    ActivityAnalysis activityAnalysis;

    for (auto playStyle : PlayStyleSmartEnum::ToRange())
    {
        activityAnalysis.BeginSnapshot[playStyle];
        activityAnalysis.ActivityByDateTime[playStyle];
        activityAnalysis.ActivitySnapshotByDateTime[playStyle];
    }
    activityAnalysis.DateTimeRange[icl_s2::RangeSide::Begin] = beginDateTime;
    activityAnalysis.DateTimeRange[icl_s2::RangeSide::End] = endDateTime;

    auto versionIndex = FindVersionIndexFromDateTime(beginDateTime);
    auto* activeVersionPtr = mMusicDatabase.FindActiveVersion(versionIndex);
    if (!activeVersionPtr)
    {
        throw std::runtime_error("invalid active version");
    }

    auto &activeVersion = *activeVersionPtr;
    auto versionDateTimeRange = GetVersionDateTimeRange(versionIndex);
    auto &versionBeginDateTime = versionDateTimeRange.at(icl_s2::RangeSide::Begin);

    for (auto &[chartId, chartInfo] : activeVersion.GetChartInfos())
    {
        auto [musicId, chartPlayStyle, difficulty] = ToMusicStyleDiffculty(chartId);
        //auto styleDifficulty = ConvertToStyleDifficulty(chartPlayStyle, difficulty);

        auto &snapshotMusicScores = activityAnalysis.BeginSnapshot[chartPlayStyle];

        auto findChartScoreAtTime = FindChartScoreAtTime(playerScore, musicId, chartPlayStyle, difficulty, beginDateTime);
        if (!findChartScoreAtTime) { continue; }

        if (!icl_s2::Find(snapshotMusicScores, musicId))
        {
            snapshotMusicScores.emplace(
                std::piecewise_construct,
                std::forward_as_tuple(musicId),
                std::forward_as_tuple(musicId, chartPlayStyle, 0, beginDateTime)
            );
        }

        auto &snapshotMusicScore = snapshotMusicScores.at(musicId);

        snapshotMusicScore.AddChartScore(difficulty, findChartScoreAtTime.value());

        auto findMusicScores = icl_s2::Find(playerScore.GetMusicScores(chartPlayStyle), musicId);
        if (!findMusicScores)
        {
            continue;
        }

        auto previousDateTime = beginDateTime;
        for (auto &[dateTime, musicScore] : findMusicScores.value()->second)
        {
            if (dateTime>=versionBeginDateTime && dateTime<=beginDateTime)
            {
                snapshotMusicScore.SetPlayCount(musicScore.GetPlayCount());
            }

            if (dateTime>beginDateTime
                && (endDateTime.empty() || dateTime<=endDateTime))
            {
                auto* findChartScore = musicScore.FindChartScore(difficulty);
                if (findChartScore)
                {
                    auto &activityMusicScoreById = activityAnalysis.ActivityByDateTime[chartPlayStyle][dateTime];
                    if (!icl_s2::Find(activityMusicScoreById, musicId))
                    {
                        activityMusicScoreById.emplace(musicId, musicScore);
                        auto &activitySnapshot = activityAnalysis.ActivitySnapshotByDateTime[chartPlayStyle];
                        auto &activityData = activitySnapshot[dateTime][musicId];
                        activityData.CurrentMusicScore = &activityMusicScoreById.at(musicId);
                        if (previousDateTime==beginDateTime)
                        {
                            activityData.PreviousMusicScore = &snapshotMusicScore;
                        }
                        else
                        {
                            auto &previousActivityData = activitySnapshot.at(previousDateTime).at(musicId);
                            activityData.PreviousMusicScore = previousActivityData.CurrentMusicScore;
                        }

                        if (!activityData.PreviousMusicScore)
                        {
                            throw std::runtime_error("activityData.PreviousMusicScore is nullptr");
                        }
                        previousDateTime = dateTime;
                    }
                }
            }
        }
    }

    //''  BeginSnapshot t0 MMMMM
    //''                t1  M
    //''                t2 M   M
    //''                t3 MM M
    //''                   ^ActivityByDateTime
    //'' fill ActivitySnapshotByDateTime so that every ActivitySnapshot have same music ids as begin snapshot
    //'' and with activity data pointers point correctly
    //''  BeginSnapshot t1 21222
    //''                t2 12221
    //''                t3 11212
    //'' 1: already set above, 2: here fill point to previous 1/2 or begin.
    //'' 1: Current: MusicScore in activity, Previous: MusicScore in activity/begin (see above graph)
    //'' 2: Current & Previous: both set to previous MusicScore in activity/begin
    for (auto &[playStyle, activityMusicScores] : activityAnalysis.ActivityByDateTime)
    {
        auto previousDateTime = beginDateTime;
        for (auto &[dateTime, musicScoreById] : activityMusicScores)
        {
            auto &activityDataById = activityAnalysis.ActivitySnapshotByDateTime.at(playStyle).at(dateTime);

            for (auto &[musicId, musicScore] : activityAnalysis.BeginSnapshot.at(playStyle))
            {
                if (icl_s2::Find(musicScoreById, musicId))
                {
                    continue;
                }

                auto &activityData = activityDataById[musicId];
                if (previousDateTime==beginDateTime)
                {
                    activityData.CurrentMusicScore = &musicScore;
                    activityData.PreviousMusicScore = &musicScore;
                }
                else
                {
                    auto &previousActivityData = activityAnalysis.ActivitySnapshotByDateTime.at(playStyle).at(previousDateTime).at(musicId);
                    activityData.CurrentMusicScore = previousActivityData.CurrentMusicScore;
                    activityData.PreviousMusicScore = previousActivityData.CurrentMusicScore;
                }
            }

            previousDateTime = dateTime;
        }
    }

    s2Time::Print<std::chrono::milliseconds>(s2Time::CountNs(begin), "AnalyzeActivity");
    return activityAnalysis;
}

std::optional<ChartScore>
Analyzer::
FindChartScoreAtTime(const PlayerScore &playerScore,
                     std::size_t musicId,
                     PlayStyle playStyle,
                     Difficulty difficulty,
                     const std::string &dateTime)
const
{
    auto findMusicScoresById = icl_s2::Find(playerScore.GetMusicScores(playStyle), musicId);
    if (!findMusicScoresById)
    {
        return ChartScore{};
    }

    auto versionIndex = FindVersionIndexFromDateTime(dateTime);
    if ((musicId/1000)>versionIndex)
    {
        std::cout << "(musicId/1000)>versionIndex, Id [" << ToMusicIdString(musicId) << ", ver [" << ToVersionString(versionIndex) << "].\n";
        return std::nullopt;
    }

    auto styleDifficulty = ConvertToStyleDifficulty(playStyle, difficulty);
    auto findContainingAvailableRange = mMusicDatabase.FindContainingAvailableVersionRange(musicId, styleDifficulty, versionIndex);
    if (!findContainingAvailableRange)
    {
        return std::nullopt;
    }

    auto &containingAvailableVersionRange = findContainingAvailableRange.value();

    ChartScore chartScore;
    auto &musicScoreById = findMusicScoresById.value()->second;
    auto versionDateTimeRange = GetVersionDateTimeRange(versionIndex);
    auto &versionBeginDateTime = versionDateTimeRange.at(icl_s2::RangeSide::Begin);
    for (auto &[recordDateTime, musicScore] : musicScoreById)
    {
        auto* findChartScore = musicScore.FindChartScore(difficulty);
        if (!findChartScore) { continue; }

        auto recordVersionIndex = FindVersionIndexFromDateTime(recordDateTime);

        if (recordDateTime<versionBeginDateTime
            && containingAvailableVersionRange.IsInRange(recordVersionIndex))
        {
            chartScore.ClearType = findChartScore->ClearType;
        }

        if (recordDateTime<=dateTime && recordDateTime>=versionBeginDateTime)
        {
            chartScore = *findChartScore;
        }

        if (recordDateTime>dateTime) { break; }
    }

    return chartScore;
}

}
