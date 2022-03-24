#include <fstream>
#include <iostream>
#include <string>

#include "ies/Common/IntegralRangeUsing.hpp"
#include "ies/StdUtil/Find.hxx"
#include "ies/Time/TimeUtilFormat.hxx"

#include "score2dx/Core/Core.hpp"
#include "score2dx/Csv/Csv.hpp"
#include "score2dx/Iidx/Version.hpp"
#include "score2dx/Score/PlayerScore.hpp"

namespace s2Time = ies::Time;

int
main(int argc, char* argv[])
{
    (void)argc;
    (void)argv;

    try
    {
        auto begin = s2Time::Now();

        score2dx::Core core;

        auto succeeded = core.LoadDirectory(R"(E:\project_document\score2dx\5483-7391)", false, true);
        if (!succeeded)
        {
            std::cout << "Load directory failed.\n";
        }

        core.LoadDirectory(R"(E:\project_document\score2dx\5483-7391\ME\5483-7391)", true, true);

        //core.AddIidxMeUser("blazar");

        //auto dbMusic = core.GetMusicDatabase().GetDbMusic(19026);
        //std::cout << "dbMusic " << dbMusic.Title << "\n";

        /*
        core.AnalyzeActivity("5483-7391", "2019-10-13 00:00", "2019-10-13 23:59");
        auto* findAnalyzeActivity = core.FindActivityAnalysis("5483-7391");
        if (!findAnalyzeActivity)
        {
            std::cout << "not findAnalyzeActivity\n";
        }

        auto &activityAnalysis = *findAnalyzeActivity;
        std::cout << "Activity Analysis:\n"
                  << "BeginDateTime [" << activityAnalysis.DateTimeRange.at(ies::RangeSide::Begin) << "]: "
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
