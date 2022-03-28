#include "score2dx/Analysis/Analyzer.hpp"

#include <iostream>

#include "ies/Common/IntegralRangeUsing.hpp"
#include "ies/StdUtil/Find.hxx"
#include "ies/Time/TimeUtilFormat.hxx"

#include "score2dx/Iidx/Version.hpp"
#include "score2dx/Score/ScoreLevel.hpp"

namespace s2Time = ies::Time;

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
    if (!ies::Find(mMusicDatabase.GetActiveVersions(), activeVersionIndex))
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
    auto &versionBeginDateTime = versionDateTimeRange.at(ies::RangeSide::Begin);

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

        auto findVersionBeginChartScore = FindChartScoreByTime(
            playerScore, musicId, chartPlayStyle,
            difficulty, versionBeginDateTime, FindChartScoreOption::AtDateTime
        );

        if (!findVersionBeginChartScore)
        {
            std::cout << "Cannot find version begin chart score.\n";
            throw std::runtime_error("not findVersionBeginChartScore");
        }
        auto &versionBeginChartScore = findVersionBeginChartScore.value();

        auto &bestScoreData = analysis.MusicBestScoreData[musicId].at(chartPlayStyle);
        bestScoreData.InitializeVersionBeginChartScore(difficulty, versionBeginChartScore);

        auto findContainingAvailableRange = mMusicDatabase.FindContainingAvailableVersionRange(musicId, styleDifficulty, mActiveVersionIndex);
        if (!findContainingAvailableRange)
        {
            throw std::runtime_error("active version incorrectly include chart version not contained.");
        }
        auto &containingAvailableVersionRange = findContainingAvailableRange.value();

        auto &musicScores = playerScore.GetMusicScores(chartPlayStyle);
        auto findMusicId = ies::Find(musicScores, musicId);
        if (findMusicId)
        {
            auto &musicScoreByDateTime = findMusicId.value()->second;

            for (auto &[dateTime, musicScore] : musicScoreByDateTime)
            {
                auto findDateTimeVersionIndex = FindVersionIndexFromDateTime(dateTime);
                if (!findDateTimeVersionIndex) { continue; }
                auto dateTimeVersionIndex = findDateTimeVersionIndex.value();
                if (!containingAvailableVersionRange.IsInRange(dateTimeVersionIndex))
                {
                    continue;
                }

                //'' It is possible that some chart is only available at certain date time after version begin.
                //'' for example: arena SPL added later.
                auto* chartScorePtr = musicScore.GetChartScore(difficulty);
                if (!chartScorePtr) { continue; }
                auto &chartScore = *chartScorePtr;

                auto inconsistency = bestScoreData.UpdateChartScore(difficulty, dateTime, chartScore, musicScore.GetPlayCount());
                if (!inconsistency.empty())
                {
                    auto title = mMusicDatabase.GetLatestMusicInfo(musicId).GetField(MusicInfoField::Title);
                    std::cout << "[" << ToMusicIdString(musicId)
                              << "][" << ToString(styleDifficulty)
                              << "] " << title << "\n"
                              << inconsistency
                              << "ChartAvailabe version" << ToString(containingAvailableVersionRange) << "\n";
                    for (auto &[debugDateTime, debugMusicScore] : musicScoreByDateTime)
                    {
                        debugMusicScore.Print();
                    }
                }
            }
        }

        auto* verBestChartScorePtr = bestScoreData.GetVersionBestMusicScore().GetChartScore(difficulty);
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

    s2Time::Print<std::chrono::milliseconds>(s2Time::CountNs(begin), "Analyze");

    return analysis;
}

ActivityAnalysis
Analyzer::
AnalyzeVersionActivity(const PlayerScore &playerScore)
const
{
    auto timeRange = GetVersionDateTimeRange(mActiveVersionIndex);
    return AnalyzeActivity(playerScore, timeRange.at(ies::RangeSide::Begin), timeRange.at(ies::RangeSide::End));
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
    activityAnalysis.DateTimeRange[ies::RangeSide::Begin] = beginDateTime;
    activityAnalysis.DateTimeRange[ies::RangeSide::End] = endDateTime;

    for (auto playStyle : PlayStyleSmartEnum::ToRange())
    {
        activityAnalysis.PreviousSnapshot[playStyle];
        activityAnalysis.ActivityByDateTime[playStyle];
        activityAnalysis.ActivitySnapshotByDateTime[playStyle];
    }

    auto findVersionIndex = FindVersionIndexFromDateTime(beginDateTime);
    if (!findVersionIndex)
    {
        return activityAnalysis;
    }
    auto versionIndex = findVersionIndex.value();

    auto* activeVersionPtr = mMusicDatabase.FindActiveVersion(versionIndex);
    if (!activeVersionPtr)
    {
        throw std::runtime_error("invalid active version");
    }

    auto &activeVersion = *activeVersionPtr;
    auto versionDateTimeRange = GetVersionDateTimeRange(versionIndex);
    auto &versionBeginDateTime = versionDateTimeRange.at(ies::RangeSide::Begin);

    for (auto &[chartId, chartInfo] : activeVersion.GetChartInfos())
    {
        auto [musicId, chartPlayStyle, difficulty] = ToMusicStyleDiffculty(chartId);

        auto &snapshotMusicScores = activityAnalysis.PreviousSnapshot[chartPlayStyle];

        auto findChartScoreBeforeTime = FindChartScoreByTime(
            playerScore, musicId, chartPlayStyle,
            difficulty, beginDateTime, FindChartScoreOption::BeforeDateTime
        );
        if (!findChartScoreBeforeTime) { continue; }

        if (!ies::Find(snapshotMusicScores, musicId))
        {
            snapshotMusicScores.emplace(
                std::piecewise_construct,
                std::forward_as_tuple(musicId),
                std::forward_as_tuple(musicId, chartPlayStyle, 0, "")
            );
        }

        auto &snapshotMusicScore = snapshotMusicScores.at(musicId);

        snapshotMusicScore.SetChartScore(difficulty, findChartScoreBeforeTime.value());

        auto findMusicScores = ies::Find(playerScore.GetMusicScores(chartPlayStyle), musicId);
        if (!findMusicScores)
        {
            continue;
        }

        std::string previousDateTime;
        for (auto &[dateTime, musicScore] : findMusicScores.value()->second)
        {
            if (dateTime>=versionBeginDateTime && dateTime<beginDateTime)
            {
                snapshotMusicScore.SetPlayCount(musicScore.GetPlayCount());
            }

            if (dateTime>=beginDateTime
                && (endDateTime.empty() || dateTime<=endDateTime))
            {
                auto* chartScorePtr = musicScore.GetChartScore(difficulty);
                if (chartScorePtr)
                {
                    auto &activityMusicScoreById = activityAnalysis.ActivityByDateTime[chartPlayStyle][dateTime];
                    if (!ies::Find(activityMusicScoreById, musicId))
                    {
                        activityMusicScoreById.emplace(musicId, musicScore);
                        auto &activitySnapshot = activityAnalysis.ActivitySnapshotByDateTime[chartPlayStyle];
                        auto &activityData = activitySnapshot[dateTime][musicId];
                        activityData.CurrentMusicScore = &activityMusicScoreById.at(musicId);
                        if (previousDateTime.empty())
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

    s2Time::Print<std::chrono::milliseconds>(s2Time::CountNs(begin), "AnalyzeActivity");
    return activityAnalysis;
}

void
Analyzer::
AnalyzeHistory(const PlayerScore &playerScore,
               std::size_t musicId,
               StyleDifficulty styleDifficulty)
const
{
    auto [playStyle, difficulty] = score2dx::Split(styleDifficulty);
    auto chartScores = playerScore.GetChartScores(musicId, playStyle, difficulty);
    std::cout << "Music ["+mMusicDatabase.GetLatestMusicInfo(musicId).GetField(score2dx::MusicInfoField::Title)+"]:\n";
    //std::cout << "Available versions: " << mMusicDatabase.
    for (auto &[dateTime, chartScorePtr] : chartScores)
    {
        auto &chartScore = *chartScorePtr;
        std::cout   << "Record ["+dateTime+"] Clear: "+ToString(chartScore.ClearType)
                    << ", DJ Level: "+ToString(chartScore.DjLevel)
                    << ", EX Score: " << chartScore.ExScore
                    << ", MissCount: ";
        if (chartScore.MissCount)
        {
            std::cout << chartScore.MissCount.value();
        }
        else
        {
            std::cout << "N/A";
        }
        std::cout << std::endl;
    }
}

std::optional<ChartScore>
Analyzer::
FindChartScoreByTime(const PlayerScore &playerScore,
                     std::size_t musicId,
                     PlayStyle playStyle,
                     Difficulty difficulty,
                     const std::string &dateTime,
                     FindChartScoreOption option)
const
{
    auto findMusicScoresById = ies::Find(playerScore.GetMusicScores(playStyle), musicId);
    if (!findMusicScoresById)
    {
        return ChartScore{};
    }

    auto findVersionIndex = FindVersionIndexFromDateTime(dateTime);
    if (!findVersionIndex)
    {
        return ChartScore{};
    }
    auto versionIndex = findVersionIndex.value();

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
    auto &versionBeginDateTime = versionDateTimeRange.at(ies::RangeSide::Begin);
    for (auto &[recordDateTime, musicScore] : musicScoreById)
    {
        auto* chartScorePtr = musicScore.GetChartScore(difficulty);
        if (!chartScorePtr) { continue; }

        auto findRecordVersionIndex = FindVersionIndexFromDateTime(recordDateTime);
        if (!findRecordVersionIndex) { continue; }
        auto recordVersionIndex = findRecordVersionIndex.value();

        if (recordDateTime<versionBeginDateTime
            && containingAvailableVersionRange.IsInRange(recordVersionIndex))
        {
            chartScore.ClearType = chartScorePtr->ClearType;
        }

        if ((recordDateTime<dateTime
            || (option==FindChartScoreOption::AtDateTime
                &&recordDateTime==dateTime))
            && recordDateTime>=versionBeginDateTime)
        {
            chartScore = *chartScorePtr;
        }

        if (recordDateTime>dateTime
            || (option==FindChartScoreOption::BeforeDateTime
                &&recordDateTime==dateTime))
        {
            break;
        }
    }

    return chartScore;
}

}
