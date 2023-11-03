#include <fstream>
#include <iostream>
#include <string>

#include "ies/Common/IntegralRangeUsing.hpp"
#include "ies/StdUtil/Find.hxx"
#include "ies/Time/ScopeTimePrinter.hxx"
#include "ies/Time/TimeUtilFormat.hxx"

#include "score2dx/Core/Core.hpp"
#include "score2dx/Core/MusicDatabase.hpp"
#include "score2dx/Csv/Csv.hpp"
#include "score2dx/Csv/CsvColumn.hpp"
#include "score2dx/Iidx/Version.hpp"
#include "score2dx/Score/PlayerScore.hpp"

int
main(int argc, char* argv[])
{
    (void)argc;
    (void)argv;

    try
    {
        ies::Time::ScopeTimePrinter<std::chrono::milliseconds> timePrinter{"experiment.exe"};

        score2dx::Core core;

        //auto succeeded = core.LoadDirectory(R"(E:\project_document\score2dx\5483-7391)", false, true);
        auto succeeded = core.LoadDirectory(R"(E:\project_document\score2dx\5483-7391_31)", false, false);
        if (!succeeded)
        {
            std::cout << "Load directory failed.\n";
        }

        //score2dx::UpgradeMusicDatabase("table/MusicDatabase29_2022-11-29.json", "table/MusicDatabase30_2022-11-29.json");

        //core.LoadDirectory(R"(E:\project_document\score2dx\5483-7391\ME\5483-7391)", true, true);

        //core.AddIidxMeUser("blazar");

        //'' 04021: JIVE INTO THE NIGHT
        //'' 14000: 2hot2eat
//        std::size_t musicId = 29000;
//        auto context = core.GetMusicDatabase().GetDbMusicContext(musicId);

//        auto availableVersions = core.GetMusicDatabase().GetAvailableVersions(musicId);
//        std::cout << "Music [" << score2dx::ToMusicIdString(musicId) << "]\n"
//                  << "Title [" << context.Title << "]\n"
//                  << "AvailableVersions: " << score2dx::ToString(availableVersions) << "\n";
        //std::cout << "dbMusic " << dbMusic.Title << "\n";

//        Print(score2dx::ParseCsvLine("CastHour,Game Changers,HARD MIXTURE,Yuta Imai,6,0,0,0,0,---,NO PLAY,---,4,691,312,67,1,FULLCOMBO CLEAR,AAA,8,1305,562,181,11,EASY CLEAR,AA,11,2018,822,374,63,ASSIST CLEAR,A,0,0,0,0,---,NO PLAY,---,2021-11-27 16:03"));
//        Print(score2dx::ParseCsvLine("CastHour,Blue Bird feat. Kanae Asaba,LATIN FUSION POP,Nhato,4,0,0,0,0,---,NO PLAY,---,4,550,252,46,0,FULLCOMBO CLEAR,AAA,6,1131,515,101,1,FULLCOMBO CLEAR,AAA,8,1526,671,184,3,EASY CLEAR,AA,0,0,0,0,---,NO PLAY,---,2021-11-13 13:57"));
//        Print(score2dx::ParseCsvLine("CastHour,Brave Spirits,MELODIC RIDDIM,MK,3,0,0,0,0,---,NO PLAY,---,4,679,319,41,0,FULLCOMBO CLEAR,AAA,8,1298,588,122,1,FULLCOMBO CLEAR,AAA,10,1848,749,350,21,EASY CLEAR,A,0,0,0,0,---,NO PLAY,---,2021-12-04 15:39"));
//        Print(score2dx::ParseCsvLine("CastHour,DISPARATE,CYBERPUNK,BEMANI Sound Team \"Captain Sonic\",4,0,0,0,0,---,NO PLAY,---,5,980,443,94,1,FULLCOMBO CLEAR,AAA,9,1679,747,185,2,FULLCOMBO CLEAR,AA,11,1881,757,367,59,EASY CLEAR,A,0,0,0,0,---,NO PLAY,---,2021-12-04 15:33"));

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

        return 0;
    }
    catch (const std::exception &e)
    {
        std::cerr << "Exception:\n" << e.what() << std::endl;
        return 1;
    }
}
