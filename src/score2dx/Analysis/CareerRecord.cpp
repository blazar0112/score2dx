#include "score2dx/Analysis/CareerRecord.hpp"

#include <iostream>
#include <numeric>

#include "ies/StdUtil/Find.hxx"

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

    return lhs.MissCount && rhs.MissCount && lhs.MissCount<rhs.MissCount;
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

    constexpr auto RecordTypeSize = RecordTypeSmartEnum::Size();

    std::array<const ChartScoreRecord*, RecordTypeSize> bestRecords{ nullptr, nullptr };
    std::array<const ChartScoreRecord*, RecordTypeSize> secondBestRecords{ nullptr, nullptr };

    for (auto* recordPtr : recordPtrVector)
    {
        auto& chartScoreRecord = *recordPtr;
        auto& chartScore = chartScoreRecord.ChartScoreProp;

        for (auto recordType : RecordTypeSmartEnum::ToRange())
        {
            auto recordTypeIndex = static_cast<std::size_t>(recordType);
            auto recordUpdated = false;

            if (!bestRecords[recordTypeIndex])
            {
                if (recordType!=RecordType::Miss || chartScore.MissCount)
                {
                    bestRecords[recordTypeIndex] = recordPtr;
                    recordUpdated = true;
                }
            }

            if (!recordUpdated && bestRecords[recordTypeIndex])
            {
                if (IsBetterRecord(recordType, chartScore, bestRecords[recordTypeIndex]->ChartScoreProp))
                {
                    secondBestRecords[recordTypeIndex] = bestRecords[recordTypeIndex];
                    bestRecords[recordTypeIndex] = recordPtr;
                    recordUpdated = true;
                }
            }

            if (!recordUpdated && !secondBestRecords[recordTypeIndex])
            {
                if (recordType!=RecordType::Miss || chartScore.MissCount)
                {
                    secondBestRecords[recordTypeIndex] = recordPtr;
                    recordUpdated = true;
                }
            }

            if (!recordUpdated && secondBestRecords[recordTypeIndex])
            {
                if (IsBetterRecord(recordType, chartScore, secondBestRecords[recordTypeIndex]->ChartScoreProp))
                {
                    secondBestRecords[recordTypeIndex] = recordPtr;
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
    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-const-cast)
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
