#include "score2dx/Score/ScoreLevel.hpp"

#include <gtest/gtest.h>

#include "ies/Common/IntegralRangeUsing.hpp"

namespace score2dx
{

TEST(ScoreLevel, KeyScore)
{
    //! e.g. note=2000:
    //!                  AA             AAA
    //! KeyScore        3112           3556
    //! HalfKeyScore            3334 <- choose to be HalfKeyScore of AAA
    //!                  [   AA+ )[  AAA- )

    constexpr int n = 2000;

    EXPECT_EQ(IntRange(3334, 3556), IntRange(FindHalfKeyScore(n, ScoreLevel::AAA), FindKeyScore(n, ScoreLevel::AAA)));
    ASSERT_EQ(IntRange(3778, 4000), IntRange(FindHalfKeyScore(n, ScoreLevel::Max), FindKeyScore(n, ScoreLevel::Max)));
}

TEST(ScoreLevel, FindScoreLevelDiff)
{
    EXPECT_ANY_THROW(
        FindScoreLevelDiff(-1, -1);
    );

    using SLRD = std::pair<ScoreLevelRange, int>;

    constexpr int n = 2000;

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

    EXPECT_EQ(SLRD({ScoreLevel::Max, ScoreRange::AtLevel}, 0), FindScoreLevelDiff(n, 4000));
    EXPECT_EQ(SLRD({ScoreLevel::Max, ScoreRange::LevelMinus}, 100), FindScoreLevelDiff(n, 3900));
    EXPECT_EQ(SLRD({ScoreLevel::Max, ScoreRange::LevelMinus}, 200), FindScoreLevelDiff(n, 3800));
    EXPECT_EQ(SLRD({ScoreLevel::Max, ScoreRange::LevelMinus}, 222), FindScoreLevelDiff(n, 3778));
    EXPECT_EQ(SLRD({ScoreLevel::AAA, ScoreRange::LevelPlus}, 221), FindScoreLevelDiff(n, 3777));
    EXPECT_EQ(SLRD({ScoreLevel::AAA, ScoreRange::LevelPlus}, 144), FindScoreLevelDiff(n, 3700));
    EXPECT_EQ(SLRD({ScoreLevel::AAA, ScoreRange::LevelPlus}, 1), FindScoreLevelDiff(n, 3557));
    EXPECT_EQ(SLRD({ScoreLevel::AAA, ScoreRange::AtLevel}, 0), FindScoreLevelDiff(n, 3556));
    EXPECT_EQ(SLRD({ScoreLevel::AAA, ScoreRange::LevelMinus}, 1), FindScoreLevelDiff(n, 3555));
    EXPECT_EQ(SLRD({ScoreLevel::AAA, ScoreRange::LevelMinus}, 222), FindScoreLevelDiff(n, 3334));
    EXPECT_EQ(SLRD({ScoreLevel::AA, ScoreRange::LevelPlus}, 221), FindScoreLevelDiff(n, 3333));

    EXPECT_EQ(SLRD({ScoreLevel::E, ScoreRange::LevelMinus}, 222), FindScoreLevelDiff(n, 667));
    EXPECT_EQ(SLRD({ScoreLevel::F, ScoreRange::LevelPlus}, 221), FindScoreLevelDiff(n, 666));
    EXPECT_EQ(SLRD({ScoreLevel::F, ScoreRange::LevelPlus}, 1), FindScoreLevelDiff(n, 446));
    EXPECT_EQ(SLRD({ScoreLevel::F, ScoreRange::AtLevel}, 0), FindScoreLevelDiff(n, 445));
    EXPECT_EQ(SLRD({ScoreLevel::F, ScoreRange::LevelMinus}, 1), FindScoreLevelDiff(n, 444));
    EXPECT_EQ(SLRD({ScoreLevel::F, ScoreRange::LevelMinus}, 222), FindScoreLevelDiff(n, 223));
    EXPECT_EQ(SLRD({ScoreLevel::F, ScoreRange::LevelMinus}, 223), FindScoreLevelDiff(n, 222));

    ASSERT_EQ(SLRD({ScoreLevel::F, ScoreRange::LevelMinus}, 445), FindScoreLevelDiff(n, 0));
}

TEST(ScoreLevel, ToScoreLevelDiffString)
{
    constexpr int n = 2000;

    EXPECT_EQ("MAX+0", ToScoreLevelDiffString(n, 4000));
    EXPECT_EQ("MAX-100", ToScoreLevelDiffString(n, 3900));
    EXPECT_EQ("MAX-200", ToScoreLevelDiffString(n, 3800));
    EXPECT_EQ("MAX-222", ToScoreLevelDiffString(n, 3778));
    EXPECT_EQ("AAA+221", ToScoreLevelDiffString(n, 3777));
    EXPECT_EQ("AAA+144", ToScoreLevelDiffString(n, 3700));
    EXPECT_EQ("AAA+1", ToScoreLevelDiffString(n, 3557));
    EXPECT_EQ("AAA+0", ToScoreLevelDiffString(n, 3556));
    EXPECT_EQ("AAA-1", ToScoreLevelDiffString(n, 3555));
    EXPECT_EQ("AAA-222", ToScoreLevelDiffString(n, 3334));
    EXPECT_EQ("AA+221", ToScoreLevelDiffString(n, 3333));

    EXPECT_EQ("E-222", ToScoreLevelDiffString(n, 667));
    EXPECT_EQ("F+221", ToScoreLevelDiffString(n, 666));
    EXPECT_EQ("F+1", ToScoreLevelDiffString(n, 446));
    EXPECT_EQ("F+0", ToScoreLevelDiffString(n, 445));
    EXPECT_EQ("F-1", ToScoreLevelDiffString(n, 444));
    EXPECT_EQ("F-222", ToScoreLevelDiffString(n, 223));
    EXPECT_EQ("F-223", ToScoreLevelDiffString(n, 222));

    ASSERT_EQ("F-445", ToScoreLevelDiffString(n, 0));
}

TEST(ScoreLevel, FindDjLevel)
{
    EXPECT_EQ(DjLevel::F, FindDjLevel({ScoreLevel::F, ScoreRange::LevelMinus}));
    EXPECT_EQ(DjLevel::F, FindDjLevel({ScoreLevel::F, ScoreRange::AtLevel}));
    EXPECT_EQ(DjLevel::F, FindDjLevel({ScoreLevel::E, ScoreRange::LevelMinus}));

    EXPECT_EQ(DjLevel::B, FindDjLevel({ScoreLevel::A, ScoreRange::LevelMinus}));
    EXPECT_EQ(DjLevel::A, FindDjLevel({ScoreLevel::A, ScoreRange::AtLevel}));
    EXPECT_EQ(DjLevel::A, FindDjLevel({ScoreLevel::A, ScoreRange::LevelPlus}));
    EXPECT_EQ(DjLevel::A, FindDjLevel({ScoreLevel::AA, ScoreRange::LevelMinus}));
    EXPECT_EQ(DjLevel::AA, FindDjLevel({ScoreLevel::AA, ScoreRange::AtLevel}));
    EXPECT_EQ(DjLevel::AA, FindDjLevel({ScoreLevel::AA, ScoreRange::LevelPlus}));
    EXPECT_EQ(DjLevel::AA, FindDjLevel({ScoreLevel::AAA, ScoreRange::LevelMinus}));
    EXPECT_EQ(DjLevel::AAA, FindDjLevel({ScoreLevel::AAA, ScoreRange::AtLevel}));
    EXPECT_EQ(DjLevel::AAA, FindDjLevel({ScoreLevel::AAA, ScoreRange::LevelPlus}));
    EXPECT_EQ(DjLevel::AAA, FindDjLevel({ScoreLevel::Max, ScoreRange::LevelMinus}));
    ASSERT_EQ(DjLevel::AAA, FindDjLevel({ScoreLevel::Max, ScoreRange::AtLevel}));
}

}
