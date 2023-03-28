#include <fstream>
#include <iostream>
#include <string>

#include "fmt/format.h"

#include "ies/Common/IntegralRangeUsing.hpp"
#include "ies/StdUtil/Find.hxx"
#include "ies/String/SplitString.hpp"
#include "ies/Time/TimeUtilFormat.hxx"

#include "score2dx/Core/Core.hpp"
#include "score2dx/Csv/Csv.hpp"
#include "score2dx/Iidx/Version.hpp"
#include "score2dx/Score/PlayerScore.hpp"

namespace s2Time = ies::Time;

void
PrintHelp()
{
    std::cout   << "Interactive commands:\n"
                << "h: print this help.\n"
                << "q: quit interactive interface.\n"
                << "pv: print version list with format [versionIndex] versionName.\n"
                << "pm <versionIndex>: print verion music list with format [musicIndex] title.\n"
                << "pi <musicId>: print music info of musicId.\n"
                << "ps <musicId> <styleDifficulty>: print scores of musicId.\n"
                << "pr <musicId> <styleDifficulty>: print analyzed record of musicId.\n"
                << "\n"
                << "Note: musicId is <versionIndex><musicIndex> e.g. musicId 28001, versionIndex=28, musicIndex=001.\n"
                << "styleDifficulty format is <SP|DP><B|N|H|A|L>.\n";
}

int
main(int argc, char* argv[])
{
    (void)argc;
    (void)argv;

    try
    {
        score2dx::Core core;
        auto succeeded = core.LoadDirectory(R"(E:\project_document\score2dx\5483-7391)");
        if (!succeeded)
        {
            std::cout << "Load directory failed.\n";
            return 1;
        }

        PrintHelp();

        auto &musicDatabase = core.GetMusicDatabase();

        bool quit = false;
        while (!quit)
        {
            std::string command;
            std::cout << "> ";
            std::getline(std::cin, command);

            if (command=="h")
            {
                PrintHelp();
                continue;
            }

            if (command=="q")
            {
                quit = true;
                continue;
            }

            if (command=="pv")
            {
                for (auto i : IndexRange{0, score2dx::VersionNames.size()})
                {
                    std::cout << "["+fmt::format("{:02}", i)+"] " << score2dx::VersionNames[i] << "\n";
                }
                continue;
            }

            if (command.starts_with("pm"))
            {
                auto tokens = ies::SplitString(" ", command);
                if (tokens.size()>=2)
                {
                    auto versionIndex = std::stoull(tokens[1]);
                    auto &versionMusics = musicDatabase.GetAllTimeMusics().at(versionIndex);
                    for (auto i : IndexRange{0, versionMusics.size()})
                    {
                        std::cout << "["+fmt::format("{:03}", i)+"] " << versionMusics[i].GetMusicInfo().GetField(score2dx::MusicInfoField::Title) << "\n";
                    }
                }
                else
                {
                    std::cout << "incorrect command format: "+command+"\n";
                }
                continue;
            }

            if (command.starts_with("pi"))
            {
                auto tokens = ies::SplitString(" ", command);
                if (tokens.size()>=2)
                {
                    auto musicId = std::stoull(tokens[1]);
                    musicDatabase.GetMusic(musicId).GetMusicInfo().Print();
                }
                else
                {
                    std::cout << "incorrect command format: "+command+"\n";
                }
                continue;
            }

            if (command.starts_with("ps"))
            {
                auto tokens = ies::SplitString(" ", command);
                if (tokens.size()>=3)
                {
                    auto musicId = std::stoull(tokens[1]);
                    auto styleDifficulty = score2dx::ToStyleDifficulty(tokens[2]);
                    auto [playStyle, difficulty] = score2dx::Split(styleDifficulty);
                    auto &playerScore = core.GetPlayerScores().at("5483-7391");
                    std::cout << "Music ["+musicDatabase.GetTitle(musicId)+"]:\n";

                    auto findVersionScoreTable = ies::Find(playerScore.GetVersionScoreTables(), musicId);
                    if (!findVersionScoreTable)
                    {
                        std::cout << "No score available.\n";
                        continue;
                    }

                    auto &versionScoreTable = findVersionScoreTable.value()->second;
                    for (auto scoreVersionIndex : score2dx::GetSupportScoreVersionRange())
                    {
                        for (auto &[dateTime, musicScore] : versionScoreTable.GetMusicScores(scoreVersionIndex, playStyle))
                        {
                            auto* chartScorePtr = musicScore.GetChartScore(difficulty);
                            if (!chartScorePtr) { continue; }

                            auto &chartScore = *chartScorePtr;
                            std::cout   << "["+dateTime+"] Clear: "+ToString(chartScore.ClearType)
                                        << ", DJ Level: "+ToString(chartScore.DjLevel)
                                        << ", EX Score: " << chartScore.ExScore
                                        << ", MissCount: ";
                            if (chartScore.MissCount)
                            {
                                std::cout << chartScore.MissCount.value();
                            }
                            else
                            {
                                std::cout << "N/A";
                            }
                            std::cout << std::endl;
                        }
                    }
                }
                else
                {
                    std::cout << "incorrect command format: "+command+"\n";
                }
                continue;
            }

            if (command.starts_with("pr"))
            {
                auto tokens = ies::SplitString(" ", command);
                if (tokens.size()<3)
                {
                    std::cout << "incorrect command format: "+command+"\n";
                    continue;
                }

                auto* analysisPtr = core.FindAnalysis("5483-7391");
                if (!analysisPtr)
                {
                    std::cout << "no analysis for 5483-7391\n";
                    continue;
                }

                auto& analysis = *analysisPtr;
                if (!analysis.CareerRecordPtr)
                {
                    std::cout << "no carreer record analysis for 5483-7391\n";
                    continue;
                }

                auto& careerRecord = *analysis.CareerRecordPtr;
                auto musicId = std::stoull(tokens[1]);
                auto styleDifficulty = score2dx::ToStyleDifficulty(tokens[2]);
                auto chartId = score2dx::ToChartId(musicId, styleDifficulty);
                for (auto recordType : score2dx::RecordTypeSmartEnum::ToRange())
                {
                    std::cout << score2dx::RecordTypeSmartEnum::GetName() << ": [" << ToString(recordType) << "]\n";
                    for (auto bestType : score2dx::BestTypeSmartEnum::ToRange())
                    {
                        std::cout << "    " << ToString(bestType) << ": ";
                        auto* recordPtr = careerRecord.GetRecord(chartId, bestType, recordType);
                        if (!recordPtr)
                        {
                            std::cout << "(nullptr)";
                        }
                        else
                        {
                            auto& record = *recordPtr;
                            std::cout << ToString(record);
                        }
                        std::cout << "\n";
                    }
                }

                continue;
            }
        }

        return 0;
    }
    catch (const std::exception &e)
    {
        std::cerr << "Exception:\n" << e.what() << std::endl;
        return 1;
    }
}
