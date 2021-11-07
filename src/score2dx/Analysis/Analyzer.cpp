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

    for (auto statisticScoreLevelRange : StatisticScoreLevelRangeSmartEnum::ToRange())
    {
        ChartIdListByScoreLevelRange[statisticScoreLevelRange];
    }
}

BestScoreData::
BestScoreData(std::size_t musicId,
              PlayStyle playStyle)
:   VersionBestMusicScore(musicId, playStyle, 0, "")
{
}

Analyzer::
Analyzer(const MusicDatabase &musicDatabase)
:   mMusicDatabase(musicDatabase)
{
    SetActiveVersion(mMusicDatabase.GetLatestVersionIndex());
}

void
Analyzer::
SetActiveVersion(std::size_t activeVersionIndex)
{
    if (!icl_s2::Find(mMusicDatabase.GetActiveVersions(), activeVersionIndex))
    {
        throw std::runtime_error("ActiveVersion "+std::to_string(activeVersionIndex)+" is not supported.");
    }

    mActiverVersionIndex = activeVersionIndex;
}

ScoreAnalysis
Analyzer::
Analyze(const PlayerScore &playerScore)
const
{
    auto begin = s2Time::Now();

    ScoreAnalysis analysis;

    for (auto styleDifficulty : StyleDifficultySmartEnum::ToRange())
    {
        analysis.StatisticsByStyleDifficulty[styleDifficulty];
    }

    for (auto versionIndex : IndexRange{0, mActiverVersionIndex+1})
    {
        for (auto styleDifficulty : StyleDifficultySmartEnum::ToRange())
        {
            analysis.StatisticsByVersionStyleDifficulty[versionIndex][styleDifficulty];
        }
    }

    auto* activeVersionPtr = mMusicDatabase.FindActiveVersion(mActiverVersionIndex);
    if (!activeVersionPtr)
    {
        throw std::runtime_error("invalid active version");
    }

    auto &activeVersion = *activeVersionPtr;

    auto versionDateTimeRange = GetVersionDateTimeRange(mActiverVersionIndex);

    for (auto &[chartId, chartInfo] : activeVersion.GetChartInfos())
    {
        auto [musicId, chartPlayStyle, difficulty] = ToMusicStyleDiffculty(chartId);
        for (auto playStyle : PlayStyleSmartEnum::ToRange())
        {
            analysis.MusicBestScoreData[musicId].emplace(
                std::piecewise_construct,
                std::forward_as_tuple(playStyle),
                std::forward_as_tuple(musicId, playStyle)
            );
        }

        auto &bestScoreData = analysis.MusicBestScoreData[musicId].at(chartPlayStyle);
        auto &versionBestMusicScore = bestScoreData.VersionBestMusicScore;
        versionBestMusicScore.AddChartScore(difficulty, ChartScore{});
        auto &careerBestChartScore = bestScoreData.CareerBestChartScores[difficulty];

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
                if (chartScore.ExScore>careerBestChartScore.BestChartScore.ExScore)
                {
                    careerBestChartScore.BestChartScore = chartScore;
                }

                if (dateTime>=versionDateTimeRange.at(icl_s2::RangeSide::Begin)
                    &&( mActiverVersionIndex==mMusicDatabase.GetLatestVersionIndex()
                        ||dateTime<=versionDateTimeRange.at(icl_s2::RangeSide::End)))
                {
                    if (musicScore.GetPlayCount()<versionBestMusicScore.GetPlayCount())
                    {
                        throw std::runtime_error("musicScore.GetPlayCount()<bestMusicScore.GetPlayCount()");
                    }
                    if (musicScore.GetPlayCount()>versionBestMusicScore.GetPlayCount())
                    {
                        versionBestMusicScore.SetPlayCount(musicScore.GetPlayCount());
                    }

                    auto* verBestChartScorePtr = versionBestMusicScore.FindChartScore(difficulty);
                    if (!verBestChartScorePtr) { throw std::runtime_error("verBestChartScorePtr is nullptr"); }
                    auto &previousChartScore = *verBestChartScorePtr;

                    if (previousChartScore.ClearType>chartScore.ClearType
                        ||previousChartScore.DjLevel>chartScore.DjLevel
                        ||previousChartScore.ExScore>chartScore.ExScore
                        ||(previousChartScore.MissCount.has_value()&&chartScore.MissCount.has_value()
                           &&previousChartScore.MissCount.value()<chartScore.MissCount.value()))
                    {
                        std::cout << "Previous " << ToString(previousChartScore) << "\n"
                                  << "Current " << ToString(chartScore) << "\n";
                        throw std::runtime_error("chart score is not incrementally better within a version");
                    }

                    versionBestMusicScore.AddChartScore(difficulty, chartScore);
                }
            }
        }

        auto* verBestChartScorePtr = versionBestMusicScore.FindChartScore(difficulty);
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

        auto styleDifficulty = ConvertToStyleDifficulty(chartPlayStyle, difficulty);
        auto versionIndex = ToIndexes(musicId).first;
        if (versionIndex>mActiverVersionIndex)
        {
            throw std::runtime_error("versionIndex>mActiverVersionIndex");
        }

        std::set<Statistics*> analysisStatsList
        {
            &analysis.StatisticsByLevel[chartInfo.Level],
            &analysis.StatisticsByStyleDifficulty[styleDifficulty],
            &analysis.StatisticsByVersionStyleDifficulty[versionIndex][styleDifficulty]
        };

        for (auto stats : analysisStatsList)
        {
            stats->ChartIdList.emplace(chartId);
            stats->ChartIdListByClearType[versionBestChartScore.ClearType].emplace(chartId);
            stats->ChartIdListByDjLevel[versionBestChartScore.DjLevel].emplace(chartId);
            stats->ChartIdListByScoreLevelRange[statsScoreLevel].emplace(chartId);
        }
    }

    s2Time::Print<std::chrono::milliseconds>(s2Time::CountNs(begin), "Analyze");

    return analysis;
}

}
