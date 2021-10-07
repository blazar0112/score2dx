#include "score2dx/Score/ScoreLevel.hpp"

#include <gtest/gtest.h>

#include "icl_s2/Common/IntegralRangeUsing.hpp"
#include "icl_s2/StdUtil/FormatString.hxx"
#include "icl_s2/Time/TimeUtilFormat.hxx"

namespace s2Time = icl_s2::Time;

namespace score2dx
{

TEST(ScoreLevel, KeyScore)
{
    //! e.g. note=2000:
    //!                  AA             AAA
    //! KeyScore        3112           3556
    //! HalfKeyScore            3334 <- choose to be HalfKeyScore of AAA
    //!                  [   AA+ )[  AAA- )

    int n = 2000;

    EXPECT_EQ(IntRange(3334, 3556), IntRange(FindHalfKeyScore(n, ScoreLevel::AAA), FindKeyScore(n, ScoreLevel::AAA)));
    ASSERT_EQ(IntRange(3778, 4000), IntRange(FindHalfKeyScore(n, ScoreLevel::Max), FindKeyScore(n, ScoreLevel::Max)));
}

TEST(ScoreLevel, FindScoreLevelRangeDiff)
{
    EXPECT_ANY_THROW(
        FindScoreLevelRangeDiff(-1, -1);
    );

    using SLRD = std::pair<ScoreLevelRange, int>;

    int n = 2000;

    //'' KeyScores:
    //'' index ScoreLevelRange KeyScore(Center)
    //''    18             Max      4000
    //''    17            Max-      3778
    //''    16              3A      3556
    //''    15             3A-      3334
    //''    14              2A      3112
    //''    13             2A-      2889
    //''    12               A      2667
    //''    11              A-      2445
    //''    10               B      2223
    //''     9              B-      2000
    //''     8               C      1778
    //''     7              C-      1556
    //''     6               D      1334
    //''     5              D-      1112
    //''     4               E       889
    //''     3              E-       667
    //''     2               F       445
    //''     1              F-       223
    //''     0             Min         0

    EXPECT_EQ(SLRD({ScoreLevel::Max, ScoreRange::AtLevel}, 0), FindScoreLevelRangeDiff(n, 4000));
    EXPECT_EQ(SLRD({ScoreLevel::Max, ScoreRange::LevelMinus}, 100), FindScoreLevelRangeDiff(n, 3900));
    EXPECT_EQ(SLRD({ScoreLevel::Max, ScoreRange::LevelMinus}, 200), FindScoreLevelRangeDiff(n, 3800));
    EXPECT_EQ(SLRD({ScoreLevel::Max, ScoreRange::LevelMinus}, 222), FindScoreLevelRangeDiff(n, 3778));
    EXPECT_EQ(SLRD({ScoreLevel::AAA, ScoreRange::LevelPlus}, 221), FindScoreLevelRangeDiff(n, 3777));
    EXPECT_EQ(SLRD({ScoreLevel::AAA, ScoreRange::LevelPlus}, 144), FindScoreLevelRangeDiff(n, 3700));
    EXPECT_EQ(SLRD({ScoreLevel::AAA, ScoreRange::LevelPlus}, 1), FindScoreLevelRangeDiff(n, 3557));
    EXPECT_EQ(SLRD({ScoreLevel::AAA, ScoreRange::AtLevel}, 0), FindScoreLevelRangeDiff(n, 3556));
    EXPECT_EQ(SLRD({ScoreLevel::AAA, ScoreRange::LevelMinus}, 1), FindScoreLevelRangeDiff(n, 3555));
    EXPECT_EQ(SLRD({ScoreLevel::AAA, ScoreRange::LevelMinus}, 222), FindScoreLevelRangeDiff(n, 3334));
    EXPECT_EQ(SLRD({ScoreLevel::AA, ScoreRange::LevelPlus}, 221), FindScoreLevelRangeDiff(n, 3333));

    EXPECT_EQ(SLRD({ScoreLevel::E, ScoreRange::LevelMinus}, 222), FindScoreLevelRangeDiff(n, 667));
    EXPECT_EQ(SLRD({ScoreLevel::F, ScoreRange::LevelPlus}, 221), FindScoreLevelRangeDiff(n, 666));
    EXPECT_EQ(SLRD({ScoreLevel::F, ScoreRange::LevelPlus}, 1), FindScoreLevelRangeDiff(n, 446));
    EXPECT_EQ(SLRD({ScoreLevel::F, ScoreRange::AtLevel}, 0), FindScoreLevelRangeDiff(n, 445));
    EXPECT_EQ(SLRD({ScoreLevel::F, ScoreRange::LevelMinus}, 1), FindScoreLevelRangeDiff(n, 444));
    EXPECT_EQ(SLRD({ScoreLevel::F, ScoreRange::LevelMinus}, 222), FindScoreLevelRangeDiff(n, 223));
    EXPECT_EQ(SLRD({ScoreLevel::F, ScoreRange::LevelMinus}, 223), FindScoreLevelRangeDiff(n, 222));

    ASSERT_EQ(SLRD({ScoreLevel::F, ScoreRange::LevelMinus}, 445), FindScoreLevelRangeDiff(n, 0));
}

TEST(ScoreLevel, ToScoreLevelRangeDiffString)
{
    int n = 2000;

    EXPECT_EQ("MAX+0", ToScoreLevelRangeDiffString(n, 4000));
    EXPECT_EQ("MAX-100", ToScoreLevelRangeDiffString(n, 3900));
    EXPECT_EQ("MAX-200", ToScoreLevelRangeDiffString(n, 3800));
    EXPECT_EQ("MAX-222", ToScoreLevelRangeDiffString(n, 3778));
    EXPECT_EQ("AAA+221", ToScoreLevelRangeDiffString(n, 3777));
    EXPECT_EQ("AAA+144", ToScoreLevelRangeDiffString(n, 3700));
    EXPECT_EQ("AAA+1", ToScoreLevelRangeDiffString(n, 3557));
    EXPECT_EQ("AAA+0", ToScoreLevelRangeDiffString(n, 3556));
    EXPECT_EQ("AAA-1", ToScoreLevelRangeDiffString(n, 3555));
    EXPECT_EQ("AAA-222", ToScoreLevelRangeDiffString(n, 3334));
    EXPECT_EQ("AA+221", ToScoreLevelRangeDiffString(n, 3333));

    EXPECT_EQ("E-222", ToScoreLevelRangeDiffString(n, 667));
    EXPECT_EQ("F+221", ToScoreLevelRangeDiffString(n, 666));
    EXPECT_EQ("F+1", ToScoreLevelRangeDiffString(n, 446));
    EXPECT_EQ("F+0", ToScoreLevelRangeDiffString(n, 445));
    EXPECT_EQ("F-1", ToScoreLevelRangeDiffString(n, 444));
    EXPECT_EQ("F-222", ToScoreLevelRangeDiffString(n, 223));
    EXPECT_EQ("F-223", ToScoreLevelRangeDiffString(n, 222));

    ASSERT_EQ("F-445", ToScoreLevelRangeDiffString(n, 0));
}

}
