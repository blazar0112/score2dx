#include <fstream>
#include <iostream>
#include <string>

#include "icl_s2/Common/IntegralRangeUsing.hpp"
#include "icl_s2/StdUtil/Find.hxx"
#include "icl_s2/Time/TimeUtilFormat.hxx"

#include "score2dx/Core/Core.hpp"
#include "score2dx/Csv/Csv.hpp"
#include "score2dx/Iidx/Version.hpp"
#include "score2dx/Score/PlayerScore.hpp"

namespace s2Time = icl_s2::Time;

int
main(int argc, char* argv[])
{
    (void)argc;
    (void)argv;

    try
    {
        auto begin = s2Time::Now();

        score2dx::Core core;
        core.SetActiveVersionIndex(28);

        auto succeeded = core.LoadDirectory(R"(E:\project_document\score2dx\1067-6562)", true);
        if (!succeeded)
        {
            std::cout << "Load directory failed.\n";
        }

        auto* scoreAnalysisPtr = core.FindAnalysis("1067-6562");
        if (!scoreAnalysisPtr) { throw std::runtime_error("scoreAnalysisPtr is nullptr."); }
        auto &scoreAnalysis = *scoreAnalysisPtr;
        std::cout << "scoreAnalysis.StatisticsByVersionStyleDifficulty.size() = " << scoreAnalysis.StatisticsByVersionStyleDifficulty.size() << std::endl;

        auto lastIt = scoreAnalysis.StatisticsByVersionStyleDifficulty.rbegin();
        auto lastVer = lastIt->first;
        auto &verStyleDiffStats = lastIt->second;
        std::cout << "Last analyzed version [" << lastVer << "]\n";
        for (auto &[styleDifficulty, statistics] : verStyleDiffStats)
        {
            std::cout << "[" << ToString(styleDifficulty) << "] " << statistics.ChartIdList.size() << " chart ids.\n";
        }

        auto &dpaStats = verStyleDiffStats.at(score2dx::StyleDifficulty::DPA);
        std::cout << "DPA stats:\n";
        std::cout << "--------\n";
        for (auto &[clearType, chartIds] : dpaStats.ChartIdListByClearType)
        {
            std::cout << ToString(clearType) << ": " << chartIds.size() << "\n";
        }
        std::cout << "--------\n";
        for (auto &[djLevel, chartIds] : dpaStats.ChartIdListByDjLevel)
        {
            std::cout << ToString(djLevel) << ": " << chartIds.size() << "\n";
        }
        std::cout << "--------\n";
        for (auto &[scoreLevel, chartIds] : dpaStats.ChartIdListByScoreLevelRange)
        {
            std::cout << ToString(scoreLevel) << ": " << chartIds.size() << "\n";
        }

        s2Time::Print<std::chrono::milliseconds>(s2Time::CountNs(begin), "experiment.exe");
        return 0;
    }
    catch (const std::exception &e)
    {
        std::cerr << "Exception:\n" << e.what() << std::endl;
        return 1;
    }
}
