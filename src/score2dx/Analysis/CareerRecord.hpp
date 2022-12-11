#pragma once

#include <array>
#include <map>
#include <memory>
#include <string>

#include "ies/Common/SmartEnum.hxx"

#include "score2dx/Analysis/ChartScoreRecord.hpp"
#include "score2dx/Iidx/Definition.hpp"
#include "score2dx/Score/ChartScore.hpp"

namespace score2dx
{

IES_SMART_ENUM(RecordType,
    Score,
    Miss
);

//! @brief Check if lhs is better than rhs chart score of type.
//! @note For miss, must exist to compare, return false if miss not exist.
bool
IsBetterRecord(
    RecordType recordType,
    const ChartScore& lhs,
    const ChartScore& rhs);

//! @brief
//! Active version maybe not latest! can switch to activate previous version.
//! All record can be null.
//! VersionBest: best record of current active version
//! OtherBest: best record other than Version best, can be Version 2nd best or other version best.
//! CareerBest: best between VersionBest and OtherBest.
IES_SMART_ENUM(BestType,
    VersionBest,
    OtherBest,
    CareerBest
);

struct BestRecord
{
    std::unique_ptr<ChartScoreRecord> VersionBest;
    std::array<std::unique_ptr<ChartScoreRecord>, RecordTypeSmartEnum::Size()> OtherBestByRecordType;
    std::array<ChartScoreRecord*, RecordTypeSmartEnum::Size()> CareerBestByRecordType{nullptr, nullptr};
};

//! @brief Stores all active chart's career best/other-best score for analysis.
class CareerRecord
{
public:
        explicit CareerRecord(std::size_t activeVersionIndex);

    //! @brief Provide best chart score of each versions (of same ChartIndex).
    //! @note Compare and only leave best/second-best here.
        void
        Add(std::size_t chartId,
            const std::map<std::size_t, std::vector<ChartScoreRecord>>& versionRecords);

        const ChartScoreRecord*
        GetRecord(std::size_t chartId,
                  BestType bestType,
                  RecordType recordType)
        const;

        bool
        IsVersionBestCareerBest(std::size_t chartId,
                                RecordType recordType)
        const;

private:
    std::size_t mActiveVersionIndex;
    //! @brief Map of {ChartId, BestRecord}.
    std::map<std::size_t, BestRecord> mBestRecordByChartId;

        BestRecord&
        GetBestRecord(std::size_t chartId);

        const BestRecord&
        GetBestRecord(std::size_t chartId)
        const;
};

}
