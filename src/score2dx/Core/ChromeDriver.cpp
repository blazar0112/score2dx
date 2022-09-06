#include "score2dx/Core/ChromeDriver.hpp"

#include <cstdio>
#include <cstdlib>

#include <iostream>
#include <fstream>
#include <filesystem>
#include <memory>

#include "ies/StdUtil/Find.hxx"
#include "ies/String/SplitString.hpp"

namespace fs = std::filesystem;

namespace score2dx
{

int
CheckChromeDriverVersion(const std::string &driverPath)
{
    auto command = driverPath+" -v";
    std::string logFilename = "chromedriver.log";
    auto sysCommand = command+" >"+logFilename;
    std::system(sysCommand.c_str());

    std::ifstream logFile{logFilename};

    if (!logFile)
    {
        std::cerr << "Open log file [" << logFilename << "] failed." << std::endl;
        return 0;
    }

    std::string output;
    std::getline(logFile, output);

    if (output.empty())
    {
        return 0;
    }

    auto tokens = ies::SplitString(" ", output);
    if (tokens.size()!=3)
    {
        std::cerr << "Incorrect chrome driver -v result:\n" << output << std::endl;
        return 0;
    }

    auto &versionString = tokens[1];

    auto versions = ies::SplitString(".", versionString);
    if (versions.size()!=4)
    {
        std::cerr << "Incorrect chrome driver -v version string:\n" << versionString << std::endl;
        return 0;
    }

    try
    {
        auto version = std::stoi(versions[0]);
        return version;
    }
    catch (const std::exception &)
    {
        std::cerr << "Parse chrome driver -v version string failed:\n" << versionString << std::endl;
        return 0;
    }
}

int
CheckChromeBrowserVersion()
{
    auto* programFilesEnv = std::getenv("PROGRAMFILES(X86)");
    if (!programFilesEnv)
    {
        std::cerr << "Cannot read env ProgramFiles." << std::endl;
        return 0;
    }

    fs::path programFilesPath{programFilesEnv};
    auto chromeBrowserPath = programFilesPath / R"(Google\Chrome\Application)";

    if (!fs::is_directory(chromeBrowserPath))
    {
        std::cerr << "chromeBrowserPath " << chromeBrowserPath << " is not a directory." << std::endl;
        return 0;
    }

    for (auto &entry : fs::directory_iterator(chromeBrowserPath))
    {
        if (fs::is_directory(entry))
        {
            auto versionDirectory = entry.path().filename().string();
            if (ies::Find(versionDirectory, "."))
            {
                auto versions = ies::SplitString(".", versionDirectory);
                if (versions.size()!=4)
                {
                    std::cerr << "incorrect browser version format: " << versionDirectory << std::endl;
                    return 0;
                }

                try
                {
                    auto version = std::stoi(versions[0]);
                    return version;
                }
                catch (const std::exception &)
                {
                    std::cerr << "Parse chrome browser version string failed:\n" << versionDirectory << std::endl;
                    return 0;
                }

                break;
            }
        }
    }

    std::cerr << "cannot find version directory in chromeBrowserPath " << chromeBrowserPath << std::endl;
    return 0;
}

}
