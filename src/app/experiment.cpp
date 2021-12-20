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

        auto succeeded = core.LoadDirectory(R"(E:\project_document\score2dx\5483-7391_test_29)", false, true);
        if (!succeeded)
        {
            std::cout << "Load directory failed.\n";
        }

        //core.CheckIidxMeDataTable();
        //core.GetMusicDatabase().CheckValidity();

        //core.ExportIidxMeData("delmitz");

        /*
        core.AnalyzeActivity("5483-7391", "2019-10-13 00:00", "2019-10-13 23:59");
        auto* findAnalyzeActivity = core.FindActivityAnalysis("5483-7391");
        if (!findAnalyzeActivity)
        {
            std::cout << "not findAnalyzeActivity\n";
        }

        auto &activityAnalysis = *findAnalyzeActivity;
        std::cout << "Activity Analysis:\n"
                  << "BeginDateTime [" << activityAnalysis.DateTimeRange.at(icl_s2::RangeSide::Begin) << "]: "
                  << activityAnalysis.BeginSnapshot.at(score2dx::PlayStyle::DoublePlay).size() << "\n";

        for (auto &[dateTime, activities] : activityAnalysis.ActivityByDateTime.at(score2dx::PlayStyle::DoublePlay))
        {
            std::cout << "DateTime [" << dateTime << "]: "
                      << activities.size() << "\n";
        }
        */

        /*
        for (auto activeVersionIndex : ReverseIndexRange{score2dx::GetFirstDateTimeAvailableVersionIndex(), score2dx::GetLatestVersionIndex()+1})
        {
            std::cout << "Active Version set to [" << score2dx::ToVersionString(activeVersionIndex) << "].\n";
            core.SetActiveVersionIndex(activeVersionIndex);
            core.Analyze("5483-7391");
        }
        */

        //core.GetMusicDatabase().CheckValidity();

        s2Time::Print<std::chrono::milliseconds>(s2Time::CountNs(begin), "experiment.exe");
        return 0;
    }
    catch (const std::exception &e)
    {
        std::cerr << "Exception:\n" << e.what() << std::endl;
        return 1;
    }
}
