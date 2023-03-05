#include "score2dx/Analysis/ChartScoreRecord.hpp"

#include "score2dx/Iidx/Version.hpp"

namespace score2dx
{

std::string
ToString(const ChartScoreRecord& chartScoreRecord)
{
    std::string s = "["+ToVersionString(chartScoreRecord.VersionIndex)+"]";
    s += "["+chartScoreRecord.DateTime+"]: ";
    s += ToString(chartScoreRecord.ChartScoreProp);
    return s;
}

}
