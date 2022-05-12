#include "score2dx/Analysis/CareerRecord.hpp"

#include <gtest/gtest.h>

namespace score2dx
{

TEST(CareerRecord, Empty)
{
    CareerRecord careerRecord{29};
    careerRecord.Add(12345, {}, {});
    ASSERT_FALSE(careerRecord.IsVersionBestCareerBest(12345, RecordType::Score));
}

TEST(CareerRecord, VersionBest)
{
    CareerRecord careerRecord{29};
    careerRecord.Add(12345, {{"2022-04-01 13:59", {}}}, {});
    EXPECT_EQ(nullptr, careerRecord.GetRecord(12345, BestType::OtherBest, RecordType::Score));
    ASSERT_TRUE(careerRecord.IsVersionBestCareerBest(12345, RecordType::Score));
}

TEST(CareerRecord, OtherBest)
{
    CareerRecord careerRecord{29};
    careerRecord.Add(12345, {}, {{28, {{}, 28, "2022-04-01 13:59"}}});
    EXPECT_NE(nullptr, careerRecord.GetRecord(12345, BestType::OtherBest, RecordType::Score));
    EXPECT_EQ(careerRecord.GetRecord(12345, BestType::CareerBest, RecordType::Score),
              careerRecord.GetRecord(12345, BestType::OtherBest, RecordType::Score));
    ASSERT_FALSE(careerRecord.IsVersionBestCareerBest(12345, RecordType::Score));
}

}
