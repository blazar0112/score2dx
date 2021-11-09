#include "score2dx/Iidx/Version.hpp"

#include <gtest/gtest.h>

namespace score2dx
{

TEST(Version, IsValidVersion)
{
    EXPECT_FALSE(IsValidVersion("abc"));
    EXPECT_FALSE(IsValidVersion("c9"));
    EXPECT_TRUE(IsValidVersion("00"));
    EXPECT_FALSE(IsValidVersion("30"));
    ASSERT_TRUE(IsValidVersion("29"));
}

TEST(Version, GetLatestVersionIndex)
{
    ASSERT_EQ(29u, GetLatestVersionIndex());
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
    ASSERT_EQ(17u, FindVersionIndexFromDateTime("1999-12-31 23:59"));
}

}
