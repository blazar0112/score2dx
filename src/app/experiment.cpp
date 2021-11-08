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

        auto succeeded = core.LoadDirectory(R"(E:\project_document\score2dx\5483-7391)", true);
        if (!succeeded)
        {
            std::cout << "Load directory failed.\n";
        }

        for (auto activeVersionIndex : IndexRange{27, 27+1})
        {
            core.SetActiveVersionIndex(activeVersionIndex);
            core.Analyze("5483-7391");

            std::cout << "ActiveVersion set to [" << activeVersionIndex << "]\n";

            auto* scoreAnalysisPtr = core.FindAnalysis("5483-7391");
            if (!scoreAnalysisPtr) { throw std::runtime_error("scoreAnalysisPtr is nullptr."); }
            auto &scoreAnalysis = *scoreAnalysisPtr;
            std::cout << "scoreAnalysis.StatisticsByVersionStyleDifficulty.size() = " << scoreAnalysis.StatisticsByVersionStyleDifficulty.size() << std::endl;

            auto &ver27Stats = scoreAnalysis.StatisticsByVersionStyleDifficulty[27];
            for (auto &[styleDifficulty, statistics] : ver27Stats)
            {
                std::cout << "[" << ToString(styleDifficulty) << "] " << statistics.ChartIdList.size() << " chart ids.\n";
            }

            auto &diffStats = ver27Stats.at(score2dx::StyleDifficulty::DPN);
            std::cout << "DPN stats:\n";
            std::cout << "--------\n";
            for (auto &[clearType, chartIds] : diffStats.ChartIdListByClearType)
            {
                std::cout << ToString(clearType) << ": " << chartIds.size() << "\n";
            }
            std::cout << "--------\n";
            for (auto &[djLevel, chartIds] : diffStats.ChartIdListByDjLevel)
            {
                std::cout << ToString(djLevel) << ": " << chartIds.size() << "\n";
            }
            std::cout << "--------\n";
            for (auto &[scoreLevel, chartIds] : diffStats.ChartIdListByScoreLevelRange)
            {
                std::cout << ToPrettyString(scoreLevel) << ": " << chartIds.size() << "\n";
            }
        }

        core.SetActiveVersionIndex(29);
        core.Analyze("5483-7391");

        auto* scoreAnalysisPtr = core.FindAnalysis("5483-7391");
        if (!scoreAnalysisPtr) { throw std::runtime_error("scoreAnalysisPtr is nullptr."); }
        auto &scoreAnalysis = *scoreAnalysisPtr;
        std::cout << "scoreAnalysis.StatisticsByVersionStyleDifficulty.size() = " << scoreAnalysis.StatisticsByVersionStyleDifficulty.size() << std::endl;

        auto &ver29Stats = scoreAnalysis.StatisticsByVersionStyleDifficulty[29];
        for (auto &[styleDifficulty, statistics] : ver29Stats)
        {
            std::cout << "[" << ToString(styleDifficulty) << "] " << statistics.ChartIdList.size() << " chart ids.\n";
        }

        auto &diffStats = ver29Stats.at(score2dx::StyleDifficulty::DPN);
        std::cout << "DPN stats:\n";
        std::cout << "--------\n";
        for (auto &[clearType, chartIds] : diffStats.ChartIdListByClearType)
        {
            std::cout << ToString(clearType) << ": " << chartIds.size() << "\n";
        }
        std::cout << "--------\n";
        for (auto &[djLevel, chartIds] : diffStats.ChartIdListByDjLevel)
        {
            std::cout << ToString(djLevel) << ": " << chartIds.size() << "\n";
        }
        std::cout << "--------\n";
        for (auto &[scoreLevel, chartIds] : diffStats.ChartIdListByScoreLevelRange)
        {
            std::cout << ToPrettyString(scoreLevel) << ": " << chartIds.size() << "\n";
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
