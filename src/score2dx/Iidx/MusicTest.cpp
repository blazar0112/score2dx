#include "score2dx/Iidx/Music.hpp"

#include <gtest/gtest.h>

namespace score2dx
{

/*
"BRILLIANT 2U": {
    "availableVersions": "01-05, 10-25",
    "difficulty": {
        "DPH": {
            "01-04": {
                "level": 4,
                "note": 363
            },
            "05, 10": {
                "level": 6,
                "note": 363
            },
            "11-25": {
                "level": 5,
                "note": 363
            }
        },
*/

TEST(Music, AddAvailability)
{
    Music music{1000, "BRILLIANT 2U"};
    //'' modified to test more case
    std::map<std::string, ChartInfo> dphChartInfos
    {
        {"01-04", {4, 363}},
        {"05, 10", {6, 363}},
        {"11-24", {5, 363}},
        {"26", {5, 400}},
        {"28", {5, 363}}
    };
    music.AddAvailability(StyleDifficulty::DPH, dphChartInfos);

    auto &ver00 = music.GetChartAvailability(StyleDifficulty::DPH, 0);
    EXPECT_EQ(ChartStatus::NotAvailable, ver00.ChartAvailableStatus);

    auto &ver01 = music.GetChartAvailability(StyleDifficulty::DPH, 1);
    EXPECT_EQ(ChartStatus::BeginAvailable, ver01.ChartAvailableStatus);
    EXPECT_EQ(ChartInfo(4, 363), ver01.ChartInfoProp);
    EXPECT_EQ(0u, ver01.ChartIndex);

    auto &ver02 = music.GetChartAvailability(StyleDifficulty::DPH, 2);
    EXPECT_EQ(ChartStatus::Available, ver02.ChartAvailableStatus);
    EXPECT_EQ(ChartInfo(4, 363), ver02.ChartInfoProp);
    EXPECT_EQ(0u, ver01.ChartIndex);

    auto &ver04 = music.GetChartAvailability(StyleDifficulty::DPH, 4);
    EXPECT_EQ(ChartStatus::Available, ver04.ChartAvailableStatus);
    EXPECT_EQ(ChartInfo(4, 363), ver04.ChartInfoProp);
    EXPECT_EQ(0u, ver04.ChartIndex);

    auto &ver05 = music.GetChartAvailability(StyleDifficulty::DPH, 5);
    EXPECT_EQ(ChartStatus::Available, ver05.ChartAvailableStatus);
    EXPECT_EQ(ChartInfo(6, 363), ver05.ChartInfoProp);
    EXPECT_EQ(0u, ver05.ChartIndex);

    auto &ver06 = music.GetChartAvailability(StyleDifficulty::DPH, 6);
    EXPECT_EQ(ChartStatus::Removed, ver06.ChartAvailableStatus);

    auto &ver09 = music.GetChartAvailability(StyleDifficulty::DPH, 9);
    EXPECT_EQ(ChartStatus::Removed, ver09.ChartAvailableStatus);

    auto &ver10 = music.GetChartAvailability(StyleDifficulty::DPH, 10);
    EXPECT_EQ(ChartStatus::BeginAvailable, ver10.ChartAvailableStatus);
    EXPECT_EQ(ChartInfo(6, 363), ver10.ChartInfoProp);
    EXPECT_EQ(0u, ver10.ChartIndex);

    auto &ver11 = music.GetChartAvailability(StyleDifficulty::DPH, 11);
    EXPECT_EQ(ChartStatus::Available, ver11.ChartAvailableStatus);
    EXPECT_EQ(ChartInfo(5, 363), ver11.ChartInfoProp);
    EXPECT_EQ(0u, ver10.ChartIndex);

    auto &ver25 = music.GetChartAvailability(StyleDifficulty::DPH, 25);
    EXPECT_EQ(ChartStatus::Removed, ver25.ChartAvailableStatus);

    auto &ver26 = music.GetChartAvailability(StyleDifficulty::DPH, 26);
    EXPECT_EQ(ChartStatus::BeginAvailable, ver26.ChartAvailableStatus);
    EXPECT_EQ(ChartInfo(5, 400), ver26.ChartInfoProp);
    EXPECT_EQ(1u, ver26.ChartIndex);

    auto &ver27 = music.GetChartAvailability(StyleDifficulty::DPH, 27);
    EXPECT_EQ(ChartStatus::Removed, ver27.ChartAvailableStatus);

    auto &ver28 = music.GetChartAvailability(StyleDifficulty::DPH, 28);
    EXPECT_EQ(ChartStatus::BeginAvailable, ver28.ChartAvailableStatus);
    EXPECT_EQ(ChartInfo(5, 363), ver28.ChartInfoProp);
    EXPECT_EQ(0u, ver28.ChartIndex);

    auto &ver29 = music.GetChartAvailability(StyleDifficulty::DPH, 29);
    ASSERT_EQ(ChartStatus::Removed, ver29.ChartAvailableStatus);
}

}
