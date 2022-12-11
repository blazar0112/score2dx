#include "score2dx/Iidx/Version.hpp"

#include <gtest/gtest.h>

namespace score2dx
{

TEST(Version, IsValidVersion)
{
    EXPECT_FALSE(IsValidVersion("abc"));
    EXPECT_FALSE(IsValidVersion("c9"));
    EXPECT_TRUE(IsValidVersion("00"));
    EXPECT_FALSE(IsValidVersion("31"));
    ASSERT_TRUE(IsValidVersion("30"));
}

TEST(Version, GetLatestVersionIndex)
{
    ASSERT_EQ(30u, GetLatestVersionIndex());
}

TEST(Version, FindVersionIndex)
{
    auto findInvalidVersion = FindVersionIndex("st style");
    EXPECT_FALSE(findInvalidVersion.has_value());
    EXPECT_EQ(0u, FindVersionIndex("1st style"));
    EXPECT_EQ(11u, FindVersionIndex("IIDX RED"));
    EXPECT_EQ(28u, FindVersionIndex("BISTROVER"));
    ASSERT_EQ(1u, FindVersionIndex("substream"));
}

TEST(Version, FindVersionIndexFromDateTime)
{
    EXPECT_EQ(29u, FindVersionIndexFromDateTime("2021-11-10 00:00"));
    EXPECT_EQ(29u, FindVersionIndexFromDateTime("2021-10-13 00:00"));
    EXPECT_EQ(18u, FindVersionIndexFromDateTime("2010-12-31 00:00"));
    EXPECT_EQ(18u, FindVersionIndexFromDateTime("2010-09-15 00:00"));
    EXPECT_EQ(17u, FindVersionIndexFromDateTime("2010-09-14 23:59"));
    EXPECT_EQ(17u, FindVersionIndexFromDateTime("2009-10-21 00:00"));
    ASSERT_EQ(std::nullopt, FindVersionIndexFromDateTime("1999-12-31 23:59"));
}

TEST(Version, FindVersionDateType)
{
    /*
        {29, {"2021-10-13 00:00", ""}},
        {28, {"2020-10-28 00:00", "2021-10-12 23:59"}},
    */
    EXPECT_EQ(VersionDateType::VersionBegin, FindVersionDateType("2020-10-28 12:34"));
    EXPECT_EQ(VersionDateType::VersionEnd, FindVersionDateType("2021-10-12 00:00"));
    EXPECT_EQ(VersionDateType::None, FindVersionDateType("2021-10-11 23:59"));
    EXPECT_EQ(VersionDateType::VersionBegin, FindVersionDateType("2021-10-13 13:57"));
    EXPECT_EQ(VersionDateType::VersionEnd, FindVersionDateType(""));
    ASSERT_EQ(VersionDateType::None, FindVersionDateType("2022-09-12 23:59"));
}

}
