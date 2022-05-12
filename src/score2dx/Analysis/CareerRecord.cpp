#include "score2dx/Analysis/CareerRecord.hpp"

#include <iostream>

#include "ies/Common/IntegralRangeUsing.hpp"
#include "ies/StdUtil/Find.hxx"
#include "ies/Time/TimeUtilFormat.hxx"

#include "score2dx/Iidx/Version.hpp"

namespace score2dx
{

CareerRecord::
CareerRecord(std::size_t activeVersionIndex)
:   mActiveVersionIndex(activeVersionIndex)
{
}

void
CareerRecord::
Add(std::size_t chartId,
    const std::map<std::string, ChartScore> &activeVersionScores,
    const std::map<std::size_t, ChartScoreRecord> &nonActiveVersionRecords)
{
    auto findBestRecord = ies::Find(mBestRecordByChartId, chartId);
    if (findBestRecord)
    {
        throw std::runtime_error("Already setup best record of chart id.");
    }

    std::unique_ptr<ChartScoreRecord> versionBestRecord;
    if (!activeVersionScores.empty())
    {
        auto &[dateTime, chartScore] = *activeVersionScores.rbegin();
        versionBestRecord = std::make_unique<ChartScoreRecord>(chartScore, mActiveVersionIndex, dateTime);
    }

    std::map<std::size_t, const ChartScoreRecord*> versionRecords;
    for (auto &[versionIndex, record] : nonActiveVersionRecords)
    {
        versionRecords[versionIndex] = &record;
    }
    if (versionBestRecord)
    {
        versionRecords[mActiveVersionIndex] = versionBestRecord.get();
    }

    std::array<const ChartScoreRecord*, RecordTypeSmartEnum::Size()> careerBestRecordByType
    {
        nullptr, nullptr
    };

    std::array<const ChartScoreRecord*, RecordTypeSmartEnum::Size()> careerSecondBestRecordByType
    {
        nullptr, nullptr
    };

    auto scoreIndex = static_cast<std::size_t>(RecordType::Score);
    auto missIndex = static_cast<std::size_t>(RecordType::Miss);

    for (auto &[versionIndex, recordPtr] : versionRecords)
    {
        auto &record = *recordPtr;
        auto &chartScore = record.ChartScoreProp;

        auto scoreUpdated = false;
        auto missUpdated = false;

        if (!careerBestRecordByType[scoreIndex])
        {
            careerBestRecordByType[scoreIndex] = recordPtr;
            scoreUpdated = true;
        }

        if (!careerBestRecordByType[missIndex] && chartScore.MissCount)
        {
            careerBestRecordByType[missIndex] = recordPtr;
            missUpdated = true;
        }

        if (!scoreUpdated && careerBestRecordByType[scoreIndex])
        {
            if (chartScore.ExScore>careerBestRecordByType[scoreIndex]->ChartScoreProp.ExScore)
            {
                careerSecondBestRecordByType[scoreIndex] = careerBestRecordByType[scoreIndex];
                careerBestRecordByType[scoreIndex] = recordPtr;
                scoreUpdated = true;
            }
        }

        if (!missUpdated && careerBestRecordByType[missIndex])
        {
            if (chartScore.MissCount && chartScore.MissCount<careerBestRecordByType[missIndex]->ChartScoreProp.MissCount)
            {
                careerSecondBestRecordByType[missIndex] = careerBestRecordByType[missIndex];
                careerBestRecordByType[missIndex] = recordPtr;
                missUpdated = true;
            }
        }

        if (!scoreUpdated && !careerSecondBestRecordByType[scoreIndex])
        {
            careerSecondBestRecordByType[scoreIndex] = recordPtr;
            scoreUpdated = true;
        }

        if (!missUpdated && !careerSecondBestRecordByType[missIndex] && chartScore.MissCount)
        {
            careerSecondBestRecordByType[missIndex] = recordPtr;
            missUpdated = true;
        }

        if (!scoreUpdated && careerSecondBestRecordByType[scoreIndex])
        {
            if (chartScore.ExScore>careerSecondBestRecordByType[scoreIndex]->ChartScoreProp.ExScore)
            {
                careerSecondBestRecordByType[scoreIndex] = recordPtr;
                scoreUpdated = true;
            }
        }

        if (!missUpdated && careerSecondBestRecordByType[missIndex])
        {
            if (chartScore.MissCount && chartScore.MissCount<careerSecondBestRecordByType[missIndex]->ChartScoreProp.MissCount)
            {
                careerSecondBestRecordByType[missIndex] = recordPtr;
                missUpdated = true;
            }
        }
    }

    auto &bestRecord = mBestRecordByChartId[chartId];
    if (versionBestRecord)
    {
        bestRecord.VersionBest = std::make_unique<ChartScoreRecord>(*versionBestRecord);
    }

    std::array<std::unique_ptr<ChartScoreRecord>, RecordTypeSmartEnum::Size()> versionSecondBestRecordByType;

    if (careerBestRecordByType[scoreIndex])
    {
        if (careerBestRecordByType[scoreIndex]->VersionIndex==mActiveVersionIndex)
        {
            bestRecord.CareerBestByRecordType[scoreIndex] = bestRecord.VersionBest.get();

            for (auto &[dateTime, chartScore] : activeVersionScores)
            {
                if (dateTime==bestRecord.VersionBest->DateTime) break;

                if (!careerSecondBestRecordByType[scoreIndex] && !versionSecondBestRecordByType[scoreIndex])
                {
                    versionSecondBestRecordByType[scoreIndex] = std::make_unique<ChartScoreRecord>(chartScore, mActiveVersionIndex, dateTime);
                    continue;
                }

                if (!careerSecondBestRecordByType[scoreIndex])
                {
                    if (chartScore.ExScore>versionSecondBestRecordByType[scoreIndex]->ChartScoreProp.ExScore)
                    {
                        versionSecondBestRecordByType[scoreIndex] = std::make_unique<ChartScoreRecord>(chartScore, mActiveVersionIndex, dateTime);
                    }
                    continue;
                }

                if (chartScore.ExScore>careerSecondBestRecordByType[scoreIndex]->ChartScoreProp.ExScore)
                {
                    if (!versionSecondBestRecordByType[scoreIndex])
                    {
                        versionSecondBestRecordByType[scoreIndex] = std::make_unique<ChartScoreRecord>(chartScore, mActiveVersionIndex, dateTime);
                        continue;
                    }

                    if (chartScore.ExScore>versionSecondBestRecordByType[scoreIndex]->ChartScoreProp.ExScore)
                    {
                        versionSecondBestRecordByType[scoreIndex] = std::make_unique<ChartScoreRecord>(chartScore, mActiveVersionIndex, dateTime);
                    }
                }
            }

            if (versionSecondBestRecordByType[scoreIndex])
            {
                bestRecord.OtherBestByRecordType[scoreIndex] = std::make_unique<ChartScoreRecord>(*versionSecondBestRecordByType[scoreIndex]);
            }
        }
        else
        {
            bestRecord.OtherBestByRecordType[scoreIndex] = std::make_unique<ChartScoreRecord>(*careerBestRecordByType[scoreIndex]);
            bestRecord.CareerBestByRecordType[scoreIndex] = bestRecord.OtherBestByRecordType[scoreIndex].get();
        }
    }

    if (careerBestRecordByType[missIndex])
    {
        if (careerBestRecordByType[missIndex]->VersionIndex==mActiveVersionIndex)
        {
            bestRecord.CareerBestByRecordType[missIndex] = bestRecord.VersionBest.get();

            for (auto &[dateTime, chartScore] : activeVersionScores)
            {
                if (dateTime==bestRecord.VersionBest->DateTime) break;

                if (!careerSecondBestRecordByType[missIndex] && !versionSecondBestRecordByType[missIndex])
                {
                    if (chartScore.MissCount)
                    {
                        versionSecondBestRecordByType[missIndex] = std::make_unique<ChartScoreRecord>(chartScore, mActiveVersionIndex, dateTime);
                    }
                    continue;
                }

                if (!careerSecondBestRecordByType[missIndex])
                {
                    if (chartScore.MissCount && chartScore.MissCount<versionSecondBestRecordByType[missIndex]->ChartScoreProp.MissCount)
                    {
                        versionSecondBestRecordByType[missIndex] = std::make_unique<ChartScoreRecord>(chartScore, mActiveVersionIndex, dateTime);
                    }
                    continue;
                }

                if (chartScore.MissCount && chartScore.MissCount<careerSecondBestRecordByType[missIndex]->ChartScoreProp.MissCount)
                {
                    if (!versionSecondBestRecordByType[missIndex])
                    {
                        versionSecondBestRecordByType[missIndex] = std::make_unique<ChartScoreRecord>(chartScore, mActiveVersionIndex, dateTime);
                        continue;
                    }

                    if (chartScore.MissCount<versionSecondBestRecordByType[missIndex]->ChartScoreProp.MissCount)
                    {
                        versionSecondBestRecordByType[missIndex] = std::make_unique<ChartScoreRecord>(chartScore, mActiveVersionIndex, dateTime);
                    }
                }
            }

            if (versionSecondBestRecordByType[missIndex])
            {
                bestRecord.OtherBestByRecordType[missIndex] = std::make_unique<ChartScoreRecord>(*versionSecondBestRecordByType[missIndex]);
            }
        }
        else
        {
            bestRecord.OtherBestByRecordType[missIndex] = std::make_unique<ChartScoreRecord>(*careerBestRecordByType[missIndex]);
            bestRecord.CareerBestByRecordType[missIndex] = bestRecord.OtherBestByRecordType[missIndex].get();
        }
    }
}

const ChartScoreRecord*
CareerRecord::
GetRecord(std::size_t chartId,
          BestType bestType,
          RecordType recordType)
const
{
    auto &bestRecord = GetBestRecord(chartId);
    if (bestType==BestType::VersionBest)
    {
        return bestRecord.VersionBest.get();
    }

    auto typeIndex = static_cast<std::size_t>(recordType);
    if (bestType==BestType::OtherBest)
    {
        return bestRecord.OtherBestByRecordType[typeIndex].get();
    }

    return bestRecord.CareerBestByRecordType[typeIndex];
}

bool
CareerRecord::
IsVersionBestCareerBest(std::size_t chartId,
                        RecordType recordType)
const
{
    auto &bestRecord = GetBestRecord(chartId);
    if (bestRecord.VersionBest)
    {
        return bestRecord.CareerBestByRecordType[static_cast<std::size_t>(recordType)]
               ==bestRecord.VersionBest.get();
    }
    return false;
}

BestRecord &
CareerRecord::
GetBestRecord(std::size_t chartId)
{
    return const_cast<BestRecord &>(std::as_const(*this).GetBestRecord(chartId));
}

const BestRecord &
CareerRecord::
GetBestRecord(std::size_t chartId)
const
{
    auto findBestRecord = ies::Find(mBestRecordByChartId, chartId);
    if (!findBestRecord)
    {
        throw std::runtime_error("Best record of chart id not exist.");
    }

    return findBestRecord.value()->second;
}

}
