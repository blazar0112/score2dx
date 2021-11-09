#include "score2dx/Iidx/Definition.hpp"

#include <gtest/gtest.h>

namespace score2dx
{

TEST(IidxDefinition, StyleDifficulty)
{
    auto styleDifficulty = ConvertToStyleDifficulty(PlayStyle::DoublePlay, Difficulty::Another);
    EXPECT_EQ("DPA", ToString(styleDifficulty));
    auto [playStyle, difficulty] = Split(styleDifficulty);
    EXPECT_EQ(PlayStyle::DoublePlay, playStyle);
    ASSERT_EQ(Difficulty::Another, difficulty);
}

TEST(IidxDefinition, IsStyle)
{
    EXPECT_FALSE(IsStyle(StyleDifficulty::SPB, PlayStyle::DoublePlay));
    ASSERT_TRUE(IsStyle(StyleDifficulty::SPA, PlayStyle::SinglePlay));
}

TEST(IidxDefinition, IsIidxId)
{
    EXPECT_FALSE(IsIidxId(""));
    EXPECT_FALSE(IsIidxId("1234 5678"));
    EXPECT_FALSE(IsIidxId("1234_5678"));
    EXPECT_FALSE(IsIidxId("12345678"));
    EXPECT_FALSE(IsIidxId("abcd-efgh"));
    ASSERT_TRUE(IsIidxId("1234-5678"));
}

TEST(IidxDefinition, PlayStyleCast)
{
    auto a = PlayStyleAcronym::DP;
    auto cast = static_cast<PlayStyle>(a);
    ASSERT_EQ(PlayStyle::DoublePlay, cast);
}

}
