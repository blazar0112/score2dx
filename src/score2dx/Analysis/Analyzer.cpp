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
            std::cout << "[" << ToFormatted(musicId)
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

}
