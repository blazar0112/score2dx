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

std::string
ToPrettyString(StatisticScoreLevelRange statisticScoreLevelRange)
{
    static const std::array<std::string, StatisticScoreLevelRangeSmartEnum::Size()> prettyStrings
    {
        "A-",
        "A+",
        "AA-",
        "AA+",
        "AAA-",
        "AAA+",
        "MAX-",
        "MAX"
    };

    return prettyStrings[static_cast<std::size_t>(statisticScoreLevelRange)];
}

StatisticScoreLevelRange
FindStatisticScoreLevelRange(int note, int exScore)
{
    return FindStatisticScoreLevelRange(FindScoreLevelRange(note, exScore));
}

StatisticScoreLevelRange
FindStatisticScoreLevelRange(ScoreLevelRange scoreLevelRange)
{
    auto [scoreLevel, scoreRange] = scoreLevelRange;

    auto statsScoreLevel = StatisticScoreLevelRange::AMinus;
    if (scoreLevel>=ScoreLevel::A)
    {
        if (scoreLevel==ScoreLevel::AA) { statsScoreLevel = StatisticScoreLevelRange::AAMinus; }
        if (scoreLevel==ScoreLevel::AAA) { statsScoreLevel = StatisticScoreLevelRange::AAAMinus; }
        if (scoreLevel==ScoreLevel::Max) { statsScoreLevel = StatisticScoreLevelRange::MaxMinus; }
        if (scoreRange!=ScoreRange::LevelMinus)
        {
            statsScoreLevel = static_cast<StatisticScoreLevelRange>(static_cast<int>(statsScoreLevel)+1);
        }
    }

    return statsScoreLevel;
}

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

    for (auto statisticScoreLevelRange : StatisticScoreLevelRangeSmartEnum::ToRange())
    {
        ChartIdListByScoreLevelRange[statisticScoreLevelRange];
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

        auto &bestScoreData = analysis.MusicBestScoreData[musicId].at(chartPlayStyle);
        bestScoreData.RegisterActiveChart(difficulty);

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
            }
        }

        auto* verBestChartScorePtr = bestScoreData.GetVersionBestMusicScore().FindChartScore(difficulty);
        if (!verBestChartScorePtr) { throw std::runtime_error("verBestChartScorePtr is nullptr"); }
        auto &versionBestChartScore = *verBestChartScorePtr;

        auto [scoreLevel, scoreRange] = FindScoreLevelRange(chartInfo.Note, versionBestChartScore.ExScore);
        auto statsScoreLevel = StatisticScoreLevelRange::AMinus;
        if (scoreLevel>=ScoreLevel::A)
        {
            if (scoreLevel==ScoreLevel::AA) { statsScoreLevel = StatisticScoreLevelRange::AAMinus; }
            if (scoreLevel==ScoreLevel::AAA) { statsScoreLevel = StatisticScoreLevelRange::AAAMinus; }
            if (scoreLevel==ScoreLevel::Max) { statsScoreLevel = StatisticScoreLevelRange::MaxMinus; }
            if (scoreRange!=ScoreRange::LevelMinus)
            {
                statsScoreLevel = static_cast<StatisticScoreLevelRange>(static_cast<int>(statsScoreLevel)+1);
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
                stats->ChartIdListByScoreLevelRange[statsScoreLevel].emplace(chartId);
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
    }
    activityAnalysis.BeginDateTime = beginDateTime;

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

        auto findChartScoreAtTime = FindChartScoreAtTime(playerScore, musicId, chartPlayStyle, difficulty, activityAnalysis.BeginDateTime);
        if (!findChartScoreAtTime) { continue; }

        if (!icl_s2::Find(snapshotMusicScores, musicId))
        {
            snapshotMusicScores.emplace(
                std::piecewise_construct,
                std::forward_as_tuple(musicId),
                std::forward_as_tuple(musicId, chartPlayStyle, 0, activityAnalysis.BeginDateTime)
            );
        }

        auto &snapshotMusicScore = snapshotMusicScores.at(musicId);

        snapshotMusicScore.AddChartScore(difficulty, findChartScoreAtTime.value());

        auto findMusicScores = icl_s2::Find(playerScore.GetMusicScores(chartPlayStyle), musicId);
        if (!findMusicScores) { continue; }

        for (auto &[dateTime, musicScore] : findMusicScores.value()->second)
        {
            if (dateTime>=versionBeginDateTime && dateTime<=beginDateTime)
            {
                snapshotMusicScore.SetPlayCount(musicScore.GetPlayCount());
            }

            if (dateTime>=beginDateTime
                && (endDateTime.empty() || dateTime<=endDateTime))
            {
                auto* findChartScore = musicScore.FindChartScore(difficulty);
                if (findChartScore)
                {
                    auto &activityMusicScoreById = activityAnalysis.ActivityByDateTime[chartPlayStyle][dateTime];
                    if (!icl_s2::Find(activityMusicScoreById, musicId))
                    {
                        activityMusicScoreById.emplace(musicId, musicScore);
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
        return std::nullopt;
    }

    auto versionIndex = FindVersionIndexFromDateTime(dateTime);
    if ((musicId/1000)>versionIndex)
    {
        std::cout << "(musicId/1000)>versionIndex, Id [" << ToMusicIdString(musicId) << ", ver [" << ToVersionString(versionIndex) << "].\n";
        return std::nullopt;
    }

    auto* activeVersionPtr = mMusicDatabase.FindActiveVersion(mActiveVersionIndex);
    if (!activeVersionPtr)
    {
        std::cout << "No Active Version [" << ToVersionString(versionIndex) << "].\n";
        return std::nullopt;
    }

    auto &activeVersion = *activeVersionPtr;
    auto styleDifficulty = ConvertToStyleDifficulty(playStyle, difficulty);
    if (!activeVersion.FindChartInfo(musicId, styleDifficulty))
    {
        return std::nullopt;
    }

    //'' note: version index should >= 17.
    auto previousVersionIndex = versionIndex-1;
    if (!mMusicDatabase.IsAvailable(musicId, styleDifficulty, previousVersionIndex))
    {
        return ChartScore{};
    }

    ChartScore chartScore;
    auto &musicScoreById = findMusicScoresById.value()->second;
    auto versionDateTimeRange = GetVersionDateTimeRange(versionIndex);
    auto &versionBeginDateTime = versionDateTimeRange.at(icl_s2::RangeSide::Begin);
    for (auto &[recordDateTime, musicScore] : musicScoreById)
    {
        auto* findChartScore = musicScore.FindChartScore(difficulty);
        if (!findChartScore) { continue; }

        if (recordDateTime<versionBeginDateTime)
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
