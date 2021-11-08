#include "score2dx/Score/ChartScore.hpp"

namespace score2dx
{

std::string
ToString(const ChartScore &chartScore)
{
    std::string s{"ChartScore {"};

    s += "Clear: "+ToString(chartScore.ClearType);
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

}
