#include "score2dx/Iidx/Version.hpp"

#include <gtest/gtest.h>

namespace score2dx
{

TEST(Version, FindVersionIndex)
{
    auto findInvalidVersion = FindVersionIndex("st style");
    EXPECT_FALSE(findInvalidVersion.has_value());
    EXPECT_EQ(0u, FindVersionIndex("1st style"));
    EXPECT_EQ(11u, FindVersionIndex("IIDX RED"));
    EXPECT_EQ(28u, FindVersionIndex("BISTROVER"));
    ASSERT_EQ(1u, FindVersionIndex("substream"));
}

}
