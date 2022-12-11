#include "score2dx/Analysis/Analyzer.hpp"

#include <iostream>

#include "ies/Common/IntegralRangeUsing.hpp"
#include "ies/StdUtil/Find.hxx"
#include "ies/Time/TimeUtilFormat.hxx"

#include "score2dx/Core/ScopeProfiler.hxx"
#include "score2dx/Iidx/Version.hpp"
#include "score2dx/Score/ScoreLevel.hpp"

namespace s2Time = ies::Time;

namespace score2dx
{

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
    ScopeProfiler<std::chrono::milliseconds> profiler{"Analyze"};

    ScoreAnalysis analysis;
    analysis.CareerRecordPtr = std::make_unique<CareerRecord>(mActiveVersionIndex);
    auto &careerRecord = *analysis.CareerRecordPtr;

    analysis.StatisticsByVersionStyle.resize(mActiveVersionIndex+1);
    analysis.StatisticsByVersionStyleDifficulty.resize(mActiveVersionIndex+1);

    auto* activeVersionPtr = mMusicDatabase.FindActiveVersion(mActiveVersionIndex);
    if (!activeVersionPtr)
    {
        throw std::runtime_error("invalid active version");
    }

    auto &activeVersion = *activeVersionPtr;

    //! @brief Map of {MusicId, Set of {SameMusicId active ChartId}}.
    std::map<std::size_t, std::set<std::size_t>> musicIdSortedChartIdSets;

    for (auto chartId : activeVersion.GetChartIdList())
    {
        auto [musicId, chartPlayStyle, difficulty] = ToMusicStyleDiffculty(chartId);
        musicIdSortedChartIdSets[musicId].emplace(chartId);
    }

    auto &versionScoreTables = playerScore.GetVersionScoreTables();
    for (auto &[musicId, chartIdSet] : musicIdSortedChartIdSets)
    {
        auto &music = mMusicDatabase.GetMusic(musicId);

        auto findScoreTable = ies::Find(versionScoreTables, musicId);

        for (auto chartId : chartIdSet)
        {
            auto [chartMusicId, chartPlayStyle, difficulty] = ToMusicStyleDiffculty(chartId);
            auto styleDifficulty = ConvertToStyleDifficulty(chartPlayStyle, difficulty);

            auto &availability = music.GetChartAvailability(styleDifficulty, mActiveVersionIndex);
            if (availability.ChartAvailableStatus==ChartStatus::NotAvailable
                ||availability.ChartAvailableStatus==ChartStatus::Removed)
            {
                throw std::runtime_error("active chart is not available.");
            }

            auto* chartInfoPtr = mMusicDatabase.FindChartInfo(musicId, styleDifficulty, mActiveVersionIndex);
            if (!chartInfoPtr)
            {
                throw std::runtime_error("cannot find chart info");
            }

            auto &chartInfo = *chartInfoPtr;
            if (chartInfo.Note<=0)
            {
                std::cout << "[" << ToMusicIdString(musicId)
                          << "][" << mMusicDatabase.GetTitle(musicId)
                          << "][" << ToString(styleDifficulty)
                          << "] Note is non-positive\nLevel: " << chartInfo.Level
                          << ", Note: " << chartInfo.Note
                          << ".\n";
                continue;
            }

            auto sameChartVersions = music.FindSameChartVersions(styleDifficulty, mActiveVersionIndex);

            std::map<std::size_t, std::vector<ChartScoreRecord>> versionRecords;

            if (findScoreTable)
            {
                for (auto versionIndex : sameChartVersions)
                {
                    if (versionIndex!=mActiveVersionIndex)
                    {
                        auto& versionScoreTable = findScoreTable.value()->second;
                        auto& musicScores = versionScoreTable.GetMusicScores(versionIndex, chartPlayStyle);
                        if (!musicScores.empty())
                        {
                            auto bestMusicScoreIt = musicScores.rbegin();
                            if (auto* chartScorePtr = bestMusicScoreIt->second.GetChartScore(difficulty))
                            {
                                versionRecords[versionIndex] = {ChartScoreRecord{*chartScorePtr, versionIndex, bestMusicScoreIt->first}};
                            }
                        }
                    }
                    else
                    {
                        auto& versionScoreTable = findScoreTable.value()->second;
                        auto& musicScores = versionScoreTable.GetMusicScores(versionIndex, chartPlayStyle);
                        auto& records = versionRecords[versionIndex];
                        records.reserve(musicScores.size());
                        for (auto& [dateTime, musicScore] : musicScores)
                        {
                            if (auto* chartScorePtr = musicScore.GetChartScore(difficulty))
                            {
                                records.emplace_back(ChartScoreRecord{*chartScorePtr, versionIndex, dateTime});
                            }
                        }
                    }
                }
            }

            careerRecord.Add(chartId, versionRecords);

            ChartScore versionBestChartScore;
            auto* versionBestRecordPtr = careerRecord.GetRecord(chartId, BestType::VersionBest, RecordType::Score);
            if (versionBestRecordPtr)
            {
                versionBestChartScore = versionBestRecordPtr->ChartScoreProp;
            }

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

            std::vector<Statistics*> analysisStatsPtrVec
            {
                &analysis.StatisticsByStyle[static_cast<std::size_t>(chartPlayStyle)],
                &analysis.StatisticsByStyleLevel[static_cast<std::size_t>(chartPlayStyle)][chartInfo.Level],
                &analysis.StatisticsByStyleDifficulty[static_cast<std::size_t>(styleDifficulty)],
                &analysis.StatisticsByVersionStyle[versionIndex][static_cast<std::size_t>(chartPlayStyle)],
                &analysis.StatisticsByVersionStyleDifficulty[versionIndex][static_cast<std::size_t>(styleDifficulty)]
            };

            for (auto stats : analysisStatsPtrVec)
            {
                stats->ChartIdList.emplace(chartId);
                stats->ChartIdListByClearType[static_cast<std::size_t>(versionBestChartScore.ClearType)].emplace(chartId);
                if (versionBestChartScore.ClearType!=ClearType::NO_PLAY
                    &&versionBestChartScore.ExScore!=0)
                {
                    stats->ChartIdListByDjLevel[static_cast<std::size_t>(versionBestChartScore.DjLevel)].emplace(chartId);
                    stats->ChartIdListByScoreLevelCategory[static_cast<std::size_t>(category)].emplace(chartId);
                }
            }
        }
    }

    return analysis;
}

ActivityAnalysis
Analyzer::
AnalyzeVersionActivity(const PlayerScore &playerScore)
const
{
    auto &timeRange = GetVersionDateTimeRange(mActiveVersionIndex);
    return AnalyzeActivity(playerScore, timeRange.Get(ies::RangeSide::Begin), timeRange.Get(ies::RangeSide::End));
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

    auto findActiveVersionIndex = FindVersionIndexFromDateTime(beginDateTime);
    if (!findActiveVersionIndex)
    {
        return activityAnalysis;
    }
    auto activeVersionIndex = findActiveVersionIndex.value();

    auto* activeVersionPtr = mMusicDatabase.FindActiveVersion(activeVersionIndex);
    if (!activeVersionPtr)
    {
        throw std::runtime_error("invalid active version");
    }

    auto &activeVersion = *activeVersionPtr;
    auto &versionDateTimeRange = GetVersionDateTimeRange(activeVersionIndex);
    auto &versionBeginDateTime = versionDateTimeRange.Get(ies::RangeSide::Begin);

    for (auto chartId : activeVersion.GetChartIdList())
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

        auto findVersionScoreTable = ies::Find(playerScore.GetVersionScoreTables(), musicId);
        if (!findVersionScoreTable)
        {
            continue;
        }
        auto &versionScoreTable = findVersionScoreTable.value()->second;

        std::string previousDateTime;
        for (auto scoreVersionIndex : GetSupportScoreVersionRange())
        {
            for (auto &[dateTime, musicScore] : versionScoreTable.GetMusicScores(scoreVersionIndex, chartPlayStyle))
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
    }

    s2Time::Print<std::chrono::milliseconds>(s2Time::CountNs(begin), "AnalyzeActivity");
    return activityAnalysis;
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
    auto findVersionScoreTable = ies::Find(playerScore.GetVersionScoreTables(), musicId);
    if (!findVersionScoreTable)
    {
        return ChartScore{};
    }
    auto &versionScoreTable = findVersionScoreTable.value()->second;

    auto findVersionIndex = FindVersionIndexFromDateTime(dateTime);
    if (!findVersionIndex)
    {
        return ChartScore{};
    }
    auto versionIndex = findVersionIndex.value();

    //'' note: there's Copula music previewed in Pendual event. So +1 to guranteed invalid case.
    if ((musicId/1000)>versionIndex+1)
    {
        std::cout << "(musicId/1000)>versionIndex+1, Id [" << ToMusicIdString(musicId) << ", ver [" << ToVersionString(versionIndex) << "].\n";
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

    auto &versionDateTimeRange = GetVersionDateTimeRange(versionIndex);
    auto &versionBeginDateTime = versionDateTimeRange.Get(ies::RangeSide::Begin);
    for (auto scoreVersionIndex : containingAvailableVersionRange)
    {
        for (auto &[recordDateTime, musicScore] : versionScoreTable.GetMusicScores(scoreVersionIndex, playStyle))
        {
            auto* chartScorePtr = musicScore.GetChartScore(difficulty);
            if (!chartScorePtr) { continue; }

            if (recordDateTime<versionBeginDateTime)
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
    }

    return chartScore;
}

}
