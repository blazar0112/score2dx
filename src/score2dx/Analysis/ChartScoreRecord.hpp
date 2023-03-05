#pragma once

#include "ies/Common/SmartEnum.hxx"

#include "score2dx/Score/ChartScore.hpp"

namespace score2dx
{

IES_SMART_ENUM(DiffableBestScoreType,
    ExScore,
    Miss
);

IES_SMART_ENUM(BestScoreType,
    BestExScore,
    SecondBestExScore,
    BestMiss,
    SecondBestMiss
);

struct ChartScoreRecord
{
    ChartScore ChartScoreProp;
    std::size_t VersionIndex{0};
    std::string DateTime;

        ChartScoreRecord() = default;
        ChartScoreRecord(const ChartScore &chartScore,
                         std::size_t versionIndex,
                         const std::string &dateTime)
        :   ChartScoreProp(chartScore),
            VersionIndex(versionIndex),
            DateTime(dateTime)
        {}
};

std::string
ToString(const ChartScoreRecord& chartScoreRecord);

}
