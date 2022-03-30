#include "score2dx/Core/MusicDatabase.hpp"

#include <gtest/gtest.h>

#include "ies/Common/IntegralRangeUsing.hpp"
#include "ies/StdUtil/FormatString.hxx"

namespace score2dx
{

TEST(MusicDatabase, ToRangeList)
{
    //'' gentle stress [modified]
    //'' 01-05, 09-16, 24, 27-29
    ies::IntegralRangeList<std::size_t> expectRangeList;
    expectRangeList.AddRange({1, 5+1});
    expectRangeList.AddRange({9, 16+1});
    expectRangeList.AddRange({24, 24+1});
    expectRangeList.AddRange({27, 29+1});

    ASSERT_EQ(expectRangeList.GetRanges(), ToRangeList("01-05, 09-16, 24, 27-29").GetRanges());
}

}
