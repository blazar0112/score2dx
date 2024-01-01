#include "score2dx/Score/AllVersionScoreTable.hpp"

#include <gtest/gtest.h>

namespace score2dx
{

TEST(AllVersionScoreTable, Empty)
{
    Music music{0, ""};
    AllVersionScoreTable table{music};

    ASSERT_EQ(nullptr, table.GetBestChartScore(0, Difficulty::Normal));
}

}
