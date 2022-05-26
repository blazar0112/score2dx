#include "score2dx/Score/ChartScore.hpp"

namespace score2dx
{

std::string
ToString(const ChartScore &chartScore)
{
    std::string s{"ChartScore {"};

    s += "Level: "+std::to_string(chartScore.Level);
    s += ", Clear: "+ToString(chartScore.ClearType);
    s += ", DjLevel: "+ToString(chartScore.DjLevel);
    s += ", Score: "+std::to_string(chartScore.ExScore);
    s += " ("+std::to_string(chartScore.PGreatCount);
    s += "/"+std::to_string(chartScore.GreatCount);
    s += "), Miss: ";
    if (chartScore.MissCount)
    {
        s += std::to_string(chartScore.MissCount.value());
    }
    else
    {
        s += "N/A";
    }
    s += "}";

    return s;
}

bool
IsDefault(const ChartScore &chartScore)
{
    static const ChartScore defaultChartScore;
    return chartScore==defaultChartScore;
}

bool
IsTrivial(const ChartScore &chartScore)
{
    static const ChartScore defaultChartScore;
    auto isTrivial =
    (
        chartScore.ClearType==defaultChartScore.ClearType
        && chartScore.DjLevel==defaultChartScore.DjLevel
        && chartScore.ExScore==defaultChartScore.ExScore
        && chartScore.PGreatCount==defaultChartScore.PGreatCount
        && chartScore.GreatCount==defaultChartScore.GreatCount
        && !chartScore.MissCount.has_value()
    );
    return isTrivial;
}

bool
IsValueEqual(const ChartScore &lhs, const ChartScore &rhs)
{
    auto isValueEqual =
    (
        lhs.ExScore==rhs.ExScore
        &&lhs.PGreatCount==rhs.PGreatCount
        &&lhs.GreatCount==rhs.GreatCount
        &&lhs.MissCount==rhs.MissCount
    );
    return isValueEqual;
}

}
