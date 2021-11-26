#include "score2dx/Analysis/BestScoreData.hpp"

#include <iostream>

#include "icl_s2/Common/IntegralRangeUsing.hpp"
#include "icl_s2/StdUtil/Find.hxx"
#include "icl_s2/Time/TimeUtilFormat.hxx"

#include "score2dx/Iidx/Version.hpp"

namespace s2Time = icl_s2::Time;

namespace score2dx
{

BestScoreData::
BestScoreData(std::size_t activeVersionIndex,
              std::size_t musicId,
              PlayStyle playStyle)
:   mActiveVersionIndex(activeVersionIndex),
    mVersionBestMusicScore(musicId, playStyle, 0, "")
{
    for (auto bestScoreType : BestScoreTypeSmartEnum::ToRange())
    {
        mCareerBestRecords[bestScoreType];
    }
}

std::size_t
BestScoreData::
GetActiveVersion()
const
{
    return mActiveVersionIndex;
}

const MusicScore &
BestScoreData::
GetVersionBestMusicScore()
const
{
    return mVersionBestMusicScore;
}

void
BestScoreData::
InitializeVersionBeginChartScore(Difficulty difficulty,
                                 const ChartScore &chartScore)
{
    if (!mVersionBestMusicScore.FindChartScore(difficulty))
    {
        mVersionBestMusicScore.AddChartScore(difficulty, chartScore);
        mChartVersionPlayCounts[difficulty] = 0;
    }
}

std::string
BestScoreData::
UpdateChartScore(Difficulty difficulty,
                 const std::string &dateTime,
                 const ChartScore &chartScore,
                 std::size_t playCount)
{
    if (!mVersionBestMusicScore.FindChartScore(difficulty))
    {
        throw std::runtime_error("UpdateChartScore(): InitializeVersionBeginChartScore first.");
    }

    //'' cases:
    //'' S = chartScore.EXScore, S1 = BestExScore, S2 = SecondBestExScore
    //'' For Miss: only update if miss available, rests are same (just compare different)
    //''  S1  S2    process
    //''  N/A N/A   S1 = S
    //''  S1  N/A   S>S1 ? { S2=S1; S1=S } : S<S1 ? { S2=S } : {}
    //''  N/A S2    should not happen
    //''  S1  S2    assert(S1>S2)
    //''            S>S1 ? { S2=S1; S1=S } : S<S1&&S>S2 ? { S2=S } : S<S2 ? {} : {}

    auto versionIndex = FindVersionIndexFromDateTime(dateTime);
    auto isTrivial = chartScore.ExScore==0 && !chartScore.MissCount.has_value();

    if (!isTrivial)
    {
        auto* findBestScoreRecord = FindNonConstBestChartScoreRecord(BestScoreType::BestExScore, difficulty);
        auto* findSecondBestScoreRecord = FindNonConstBestChartScoreRecord(BestScoreType::SecondBestExScore, difficulty);
        if (findBestScoreRecord)
        {
            auto &bestChartScoreRecord = *findBestScoreRecord;
            if (chartScore.ExScore>bestChartScoreRecord.ChartScoreProp.ExScore)
            {
                mCareerBestRecords[BestScoreType::SecondBestExScore][difficulty] = bestChartScoreRecord;
                mCareerBestRecords[BestScoreType::BestExScore][difficulty] = ChartScoreRecord{chartScore, versionIndex, dateTime};
            }
            else if (chartScore.ExScore<bestChartScoreRecord.ChartScoreProp.ExScore)
            {
                if (!findSecondBestScoreRecord || chartScore.ExScore>findSecondBestScoreRecord->ChartScoreProp.ExScore)
                {
                    mCareerBestRecords[BestScoreType::SecondBestExScore][difficulty] = ChartScoreRecord{chartScore, versionIndex, dateTime};
                }
            }
        }
        else
        {
            mCareerBestRecords[BestScoreType::BestExScore][difficulty] = ChartScoreRecord{chartScore, versionIndex, dateTime};
        }
    }

    if (!isTrivial && chartScore.MissCount.has_value())
    {
        auto* findBestMissRecord = FindNonConstBestChartScoreRecord(BestScoreType::BestMiss, difficulty);
        auto* findSecondBestMissRecord = FindNonConstBestChartScoreRecord(BestScoreType::SecondBestMiss, difficulty);
        if (findBestMissRecord)
        {
            auto &bestMissChartScoreRecord = *findBestMissRecord;
            if (chartScore.MissCount<bestMissChartScoreRecord.ChartScoreProp.MissCount)
            {
                mCareerBestRecords[BestScoreType::SecondBestMiss][difficulty] = bestMissChartScoreRecord;
                mCareerBestRecords[BestScoreType::BestMiss][difficulty] = ChartScoreRecord{chartScore, versionIndex, dateTime};
            }
            else if (chartScore.MissCount>bestMissChartScoreRecord.ChartScoreProp.MissCount)
            {
                if (!findSecondBestMissRecord || chartScore.MissCount<findSecondBestMissRecord->ChartScoreProp.MissCount)
                {
                    mCareerBestRecords[BestScoreType::SecondBestMiss][difficulty] = ChartScoreRecord{chartScore, versionIndex, dateTime};
                }
            }
        }
        else
        {
            mCareerBestRecords[BestScoreType::BestMiss][difficulty] = ChartScoreRecord{chartScore, versionIndex, dateTime};
        }
    }

    std::string inconsistency;
    auto versionDateTimeRange = GetVersionDateTimeRange(mActiveVersionIndex);
    if (dateTime>=versionDateTimeRange.at(icl_s2::RangeSide::Begin)
        &&(mActiveVersionIndex==GetLatestVersionIndex()
           ||dateTime<=versionDateTimeRange.at(icl_s2::RangeSide::End)))
    {
        if (playCount<mChartVersionPlayCounts.at(difficulty))
        {
            throw std::runtime_error("chart play count is not incremental.");
        }

        if (playCount>mVersionBestMusicScore.GetPlayCount())
        {
            mVersionBestMusicScore.SetPlayCount(playCount);
        }

        mChartVersionPlayCounts[difficulty] = playCount;

        auto* verBestChartScorePtr = mVersionBestMusicScore.FindChartScore(difficulty);
        if (!verBestChartScorePtr) { throw std::runtime_error("verBestChartScorePtr is nullptr"); }
        auto &versionBestChartScore = *verBestChartScorePtr;

        /*
        if (icl_s2::Find(mVersionUpdatedDifficultySet, difficulty))
        {

        }
        */

        if (versionBestChartScore.ClearType>chartScore.ClearType
            ||versionBestChartScore.DjLevel>chartScore.DjLevel
            ||versionBestChartScore.ExScore>chartScore.ExScore
            ||(versionBestChartScore.MissCount.has_value()&&chartScore.MissCount.has_value()
               &&versionBestChartScore.MissCount.value()<chartScore.MissCount.value()))
        {
            inconsistency = "Chart score is not incrementally better within a version\n";
            inconsistency += "Ver ["+ToVersionString(mActiveVersionIndex)+"\n";
            inconsistency += "VerDateTimeRange ["+versionDateTimeRange.at(icl_s2::RangeSide::Begin)
                             +", "+versionDateTimeRange.at(icl_s2::RangeSide::End)+"]\n";
            inconsistency += "Previous "+ToString(versionBestChartScore)+"\n";
            inconsistency += "Current "+ToString(chartScore)+"\n";
        }

        mVersionBestMusicScore.AddChartScore(difficulty, chartScore);
        mVersionUpdatedDifficultySet.emplace(difficulty);
    }

    return inconsistency;
}

const ChartScoreRecord*
BestScoreData::
FindBestChartScoreRecord(BestScoreType bestScoreType,
                         Difficulty difficulty)
const
{
    auto findRecord = icl_s2::Find(mCareerBestRecords.at(bestScoreType), difficulty);
    if (!findRecord) { return nullptr; }
    return &(findRecord.value()->second);
}

const ChartScoreRecord*
BestScoreData::
FindDiffableChartScoreRecord(DiffableBestScoreType diffableBestScoreType,
                             Difficulty difficulty)
const
{
    auto* verBestChartScorePtr = GetVersionBestMusicScore().FindChartScore(difficulty);
    if (!verBestChartScorePtr) { throw std::runtime_error("verBestChartScorePtr is nullptr"); }
    auto &versionBestChartScore = *verBestChartScorePtr;

    if (diffableBestScoreType==DiffableBestScoreType::ExScore)
    {
        auto* bestScoreRecord = FindBestChartScoreRecord(BestScoreType::BestExScore, difficulty);
        if (!bestScoreRecord) { return nullptr; }
        if (versionBestChartScore.ExScore>bestScoreRecord->ChartScoreProp.ExScore)
        {
            throw std::runtime_error("versionBestChartScore.ExScore>bestScoreRecord->ChartScoreProp.ExScore.");
        }

        if (versionBestChartScore.ExScore==bestScoreRecord->ChartScoreProp.ExScore)
        {
            return FindBestChartScoreRecord(BestScoreType::SecondBestExScore, difficulty);
        }

        return bestScoreRecord;
    }

    if (diffableBestScoreType==DiffableBestScoreType::Miss)
    {
        if (!versionBestChartScore.MissCount.has_value())
        {
            return nullptr;
        }

        auto* bestMissRecord = FindBestChartScoreRecord(BestScoreType::BestMiss, difficulty);
        if (!bestMissRecord) { return nullptr; }
        if (versionBestChartScore.MissCount<bestMissRecord->ChartScoreProp.MissCount)
        {
            throw std::runtime_error("versionBestChartScore.MissCount<bestMissRecord->ChartScoreProp.MissCount.");
        }

        if (versionBestChartScore.MissCount==bestMissRecord->ChartScoreProp.MissCount)
        {
            return FindBestChartScoreRecord(BestScoreType::SecondBestMiss, difficulty);
        }

        return bestMissRecord;
    }

    return nullptr;
}

ChartScoreRecord*
BestScoreData::
FindNonConstBestChartScoreRecord(BestScoreType bestScoreType,
                                 Difficulty difficulty)
{
    return const_cast<ChartScoreRecord*>(std::as_const(*this).FindBestChartScoreRecord(bestScoreType, difficulty));
}

}
