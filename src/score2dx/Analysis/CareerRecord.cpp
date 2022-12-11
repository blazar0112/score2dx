#include "score2dx/Analysis/CareerRecord.hpp"

#include <iostream>
#include <numeric>

#include "ies/Common/IntegralRangeUsing.hpp"
#include "ies/StdUtil/Find.hxx"
#include "ies/Time/TimeUtilFormat.hxx"

#include "score2dx/Iidx/Version.hpp"

namespace score2dx
{

bool
IsBetterRecord(
    RecordType recordType,
    const ChartScore& lhs,
    const ChartScore& rhs)
{
    if (recordType==RecordType::Score)
    {
        return lhs.ExScore>rhs.ExScore;
    }
    else
    {
        lhs.MissCount && rhs.MissCount && lhs.MissCount<rhs.MissCount;
    }
    return false;
}

CareerRecord::
CareerRecord(std::size_t activeVersionIndex)
:   mActiveVersionIndex(activeVersionIndex)
{
}

void
CareerRecord::
Add(std::size_t chartId,
    const std::map<std::size_t, std::vector<ChartScoreRecord>>& versionRecords)
{
    auto findBestRecord = ies::Find(mBestRecordByChartId, chartId);
    if (findBestRecord)
    {
        throw std::runtime_error("Already setup best record of chart id.");
    }

    std::unique_ptr<ChartScoreRecord> versionBestRecord;
    if (auto findActiveRecords = ies::Find(versionRecords, mActiveVersionIndex))
    {
        auto& activeVersionRecords = findActiveRecords.value()->second;
        if (!activeVersionRecords.empty())
        {
            auto& latestRecord = activeVersionRecords.back();
            versionBestRecord = std::make_unique<ChartScoreRecord>(latestRecord);
        }
    }

    std::vector<const ChartScoreRecord*> recordPtrVector;
    auto recordCount = std::accumulate(
        versionRecords.begin(), versionRecords.end(),
        0u,
        [](std::size_t count, auto& pair)
        {
            return pair.second.size()+count;
        }
    );
    recordPtrVector.reserve(recordCount);

    for (auto& [versionIndex, records] : versionRecords)
    {
        for (auto &record : records)
        {
            recordPtrVector.emplace_back(&record);
        }
    }

    auto constexpr RecordTypeSize = RecordTypeSmartEnum::Size();

    std::array<const ChartScoreRecord*, RecordTypeSize> bestRecords{ nullptr, nullptr };
    std::array<const ChartScoreRecord*, RecordTypeSize> secondBestRecords{ nullptr, nullptr };

    for (auto* recordPtr : recordPtrVector)
    {
        auto& chartScoreRecord = *recordPtr;
        auto& chartScore = chartScoreRecord.ChartScoreProp;

        for (auto recordType : RecordTypeSmartEnum::ToRange())
        {
            auto recordTypeIndex = static_cast<std::size_t>(recordType);
            std::array<bool, RecordTypeSmartEnum::Size()> typeUpdated{false, false};

            if (!bestRecords[recordTypeIndex])
            {
                if (recordType!=RecordType::Miss || chartScore.MissCount)
                {
                    bestRecords[recordTypeIndex] = recordPtr;
                    typeUpdated[recordTypeIndex] = true;
                }
            }

            if (!typeUpdated[recordTypeIndex] && bestRecords[recordTypeIndex])
            {
                if (IsBetterRecord(recordType, chartScore, bestRecords[recordTypeIndex]->ChartScoreProp))
                {
                    secondBestRecords[recordTypeIndex] = bestRecords[recordTypeIndex];
                    bestRecords[recordTypeIndex] = recordPtr;
                    typeUpdated[recordTypeIndex] = true;
                }
            }

            if (!typeUpdated[recordTypeIndex] && !secondBestRecords[recordTypeIndex])
            {
                if (recordType!=RecordType::Miss || chartScore.MissCount)
                {
                    secondBestRecords[recordTypeIndex] = recordPtr;
                    typeUpdated[recordTypeIndex] = true;
                }
            }

            if (!typeUpdated[recordTypeIndex] && secondBestRecords[recordTypeIndex])
            {
                if (IsBetterRecord(recordType, chartScore, secondBestRecords[recordTypeIndex]->ChartScoreProp))
                {
                    secondBestRecords[recordTypeIndex] = recordPtr;
                    typeUpdated[recordTypeIndex] = true;
                }
            }
        }
    }

    auto& bestRecord = mBestRecordByChartId[chartId];
    if (versionBestRecord)
    {
        bestRecord.VersionBest = std::move(versionBestRecord);
    }

    for (auto recordType : RecordTypeSmartEnum::ToRange())
    {
        auto recordTypeIndex = static_cast<std::size_t>(recordType);
        if (!bestRecords[recordTypeIndex]) { continue; }
        if (bestRecords[recordTypeIndex]->VersionIndex==mActiveVersionIndex)
        {
            if (secondBestRecords[recordTypeIndex])
            {
                bestRecord.OtherBestByRecordType[recordTypeIndex] = std::make_unique<ChartScoreRecord>(*secondBestRecords[recordTypeIndex]);
            }
            bestRecord.CareerBestByRecordType[recordTypeIndex] = bestRecord.VersionBest.get();
        }
        else
        {
            bestRecord.OtherBestByRecordType[recordTypeIndex] = std::make_unique<ChartScoreRecord>(*bestRecords[recordTypeIndex]);
            bestRecord.CareerBestByRecordType[recordTypeIndex] = bestRecord.OtherBestByRecordType[recordTypeIndex].get();
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
    auto& bestRecord = GetBestRecord(chartId);
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
    auto& bestRecord = GetBestRecord(chartId);
    if (bestRecord.VersionBest)
    {
        return bestRecord.CareerBestByRecordType[static_cast<std::size_t>(recordType)]
               ==bestRecord.VersionBest.get();
    }
    return false;
}

BestRecord&
CareerRecord::
GetBestRecord(std::size_t chartId)
{
    return const_cast<BestRecord&>(std::as_const(*this).GetBestRecord(chartId));
}

const BestRecord&
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
