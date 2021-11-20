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

        auto succeeded = core.LoadDirectory(R"(E:\project_document\score2dx\5483-7391)", false);
        if (!succeeded)
        {
            std::cout << "Load directory failed.\n";
        }

        //auto activityAnalysis = core.GetAnalyzer().AnalyzeVersionActivity(core.GetPlayerScore("5483-7391"));
        auto activityAnalysis = core.GetAnalyzer().AnalyzeActivity(core.GetPlayerScore("5483-7391"), "2021-11-20 00:00", "2021-11-20 23:59");
        std::cout << "ActivityAnalysis:\n"
                  << "Begin Time: " << activityAnalysis.BeginDateTime << "\n";
        std::size_t beginPlayCount = 0;
        for (auto &[musicId, musicScore] : activityAnalysis.BeginSnapshot.at(score2dx::PlayStyle::DoublePlay))
        {
            beginPlayCount += musicScore.GetPlayCount();
        }
        std::cout << "Begin Snapshot: Musics = " << activityAnalysis.BeginSnapshot.at(score2dx::PlayStyle::DoublePlay).size()
                  << ", PlayCount = " << beginPlayCount
                  << "\n";
        for (auto &[dateTime, musicScoreById] : activityAnalysis.ActivityByDateTime.at(score2dx::PlayStyle::DoublePlay))
        {
            std::size_t playCount = 0;
            for (auto &[musicId, musicScore] : musicScoreById)
            {
                playCount += musicScore.GetPlayCount();
            }
            std::cout << "Activity [" << dateTime
                      << "], Musics: " << musicScoreById.size()
                      << ", Total PlayCount = " << playCount
                      << "\n";
        }

        /*
        auto firstActiveVersion = core.GetMusicDatabase().GetActiveVersions().begin()->first;

        for (auto activeVersionIndex : ReverseIndexRange{firstActiveVersion, score2dx::GetLatestVersionIndex()+1})
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
