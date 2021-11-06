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

        /*
        auto succeeded = core.LoadDirectory(R"(E:\project_document\score2dx\1067-6562)", true);
        if (!succeeded)
        {
            std::cout << "Load directory failed.\n";
        }
        */

        // List all DPL level = 10/11 in each active version.
        auto &database = core.GetMusicDatabase();
        for (auto &[versionIndex, activeVersion] : database.GetActiveVersions())
        {
            if (versionIndex<26) { continue; }

            std::cout << "ActiveVersion = " << versionIndex << "\n";
            for (auto level : IntRange{10, 12})
            {
                for (auto &[musicId, styleDifficultyList] : activeVersion.GetChartDifficultyList(level))
                {
                    if (icl_s2::Find(styleDifficultyList, score2dx::StyleDifficulty::DPL))
                    {
                        auto findChartInfo = activeVersion.FindChartInfo(musicId, score2dx::StyleDifficulty::DPL);
                        if (!findChartInfo) { throw std::runtime_error("should have chart info."); }

                        auto &title = database.GetLatestMusicInfo(musicId).GetField(score2dx::MusicInfoField::Title);

                        std::cout << "[" << score2dx::ToFormatted(musicId)
                                  << "][" << title
                                  << "][DPL] Level=" << findChartInfo->Level
                                  << ", Note=" << findChartInfo->Note
                                  << ".\n";
                    }
                }
            }
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
