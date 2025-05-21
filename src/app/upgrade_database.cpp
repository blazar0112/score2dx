#include <fstream>
#include <iostream>
#include <string>

#include "ies/Time/ScopeTimePrinter.hxx"

#include "score2dx/Core/MusicDatabase.hpp"

int
main(int argc, char* argv[])
{
    (void)argc;
    (void)argv;

    try
    {
        ies::Time::ScopeTimePrinter<std::chrono::milliseconds> timePrinter{"experiment.exe"};

        score2dx::UpgradeMusicDatabase(
            R"(E:\projects\score2dx\table\MusicDatabase31_2024-10-02.json)",
            R"(E:\projects\score2dx\table\MusicDatabase32_2025-01-10.json)"
        );

        return 0;
    }
    catch (const std::exception &e)
    {
        std::cerr << "Exception:\n" << e.what() << std::endl;
        return 1;
    }
}
