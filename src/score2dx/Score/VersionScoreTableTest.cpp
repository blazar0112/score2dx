#include "score2dx/Score/VersionScoreTable.hpp"

#include <gtest/gtest.h>

namespace score2dx
{

TEST(VersionScoreTable, Empty)
{
    Music music{0, ""};
    VersionScoreTable table{music};

    ASSERT_EQ(nullptr, table.GetBestChartScore(0, PlayStyle::SinglePlay, Difficulty::Normal));
}

}
