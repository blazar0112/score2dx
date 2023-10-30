#include <cstdlib>

#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>

#include "score2dx/Core/Core.hpp"
#include "score2dx/Core/ChromeDriver.hpp"
#include "score2dx/Core/ScopeProfiler.hxx"

namespace fs = std::filesystem;

bool
TestMusicDatabase()
{
    score2dx::ScopeProfiler<std::chrono::milliseconds> profiler{"TestMusicDatabase"};
    try
    {
        score2dx::Core core;
        core.GetMusicDatabase().CheckValidity();
        return true;
    }
    catch (const std::exception &e)
    {
        std::cerr << "TestMusicDatabase exception:\n" << e.what() << std::endl;
        return false;
    }
}

bool
TestLoadDirectory()
{
    score2dx::ScopeProfiler<std::chrono::milliseconds> profiler{"TestLoadDirectory"};
    try
    {
        score2dx::Core core;
        auto succeeded = core.LoadDirectory(R"(E:\project_document\score2dx\5483-7391)", false, true);
        if (!succeeded)
        {
            std::cout << "Load directory failed." << std::endl;
            return false;
        }
        return true;
    }
    catch (const std::exception &e)
    {
        std::cerr << "TestLoadDirectory exception:\n" << e.what() << std::endl;
        return false;
    }
}

bool
TestChrome()
{
    score2dx::ScopeProfiler<std::chrono::milliseconds> profiler{"TestChrome"};
    try
    {
        //'' intended to use gui path in case forget update gui.
        const std::string chromeDriverPath = R"(D:\build\score2dx-gui\qtc-Release\src\chromedriver.exe)";
        auto driverVersion = score2dx::CheckChromeDriverVersion(chromeDriverPath);
        if (driverVersion==0)
        {
            std::cout << "cannot find chrome driver version.\n";
            return false;
        }

        std::cout << "chrome driver version = " << driverVersion << "\n";

        auto browserVersion = score2dx::CheckChromeBrowserVersion();
        if (browserVersion==0)
        {
            std::cout << "cannot find chrome browser version.\n";
            return false;
        }

        std::cout << "chrome browser version = " << browserVersion << "\n";

        if (driverVersion!=browserVersion)
        {
            std::cout << "chrome driver and browser version not match.\n";
            return false;
        }

        return true;
    }
    catch (const std::exception &e)
    {
        std::cerr << "TestChrome exception:\n" << e.what() << std::endl;
        return false;
    }
}

bool
TestIst()
{
    score2dx::ScopeProfiler<std::chrono::seconds> profiler{"TestIst"};
    try
    {
        score2dx::Core core;
        std::cout << "TestIst(): about 30secs, patience...\n";
        fs::remove_all("IST/5483-7391");

        {
            std::ofstream configFile{"config.txt"};

            configFile << "id = 5483-7391\n";
            configFile << "version = [28]\n";
            configFile << "style = [SP]\n";
        }

        std::cout << std::flush;
        std::string command = R"(C:\Programs\Python\python.exe E:\projects\score2dx\script\ist_scraper.py)";
        auto ret = std::system(command.c_str());
        if (ret!=0)
        {
            std::cout << "Execute ist_scraper.py failed.\n";
            return false;
        }

        core.LoadDirectory("IST/5483-7391");

        return true;
    }
    catch (const std::exception &e)
    {
        std::cerr << "TestIst exception:\n" << e.what() << std::endl;
        return false;
    }
}

bool
TestMeUser()
{
    score2dx::ScopeProfiler<std::chrono::milliseconds> profiler{"TestMeUser"};
    try
    {
        score2dx::Core core;
        auto iidxId = core.AddIidxMeUser("blazar");

        if (iidxId!="5483-7391")
        {
            std::cout << "Incorrect IIDX ID for ME user.\n";
            return false;
        }

        return true;
    }
    catch (const std::exception &e)
    {
        std::cerr << "TestMeUser exception:\n" << e.what() << std::endl;
        return false;
    }
}

bool
TestMeExport()
{
    score2dx::ScopeProfiler<std::chrono::seconds> profiler{"TestMeExport"};
    try
    {
        score2dx::Core core;
        auto iidxId = core.AddIidxMeUser("blazar");
        fs::remove_all("ME/"+iidxId);
        //'' 3rd style: first style that SP has null data in beginning and non-null later.
        core.ExportIidxMeData("blazar", 4);
        core.LoadDirectory("ME/"+iidxId);
        return true;
    }
    catch (const std::exception &e)
    {
        std::cerr << "TestMeExport exception:\n" << e.what() << std::endl;
        return false;
    }
}

int
main(int argc, char* argv[])
{
    (void)argc;
    (void)argv;

    score2dx::ScopeProfiler<std::chrono::seconds> profiler{"regression"};

    try
    {
        if (!TestMusicDatabase())
        {
            std::cerr << "TestMusicDatabase failed." << std::endl;
            return 1;
        }

        if (!TestLoadDirectory())
        {
            std::cerr << "TestLoadDirectory failed." << std::endl;
            return 1;
        }

        if (!TestChrome())
        {
            std::cerr << "TestChrome failed." << std::endl;
            return 1;
        }

        if (!TestIst())
        {
            std::cerr << "TestIst failed." << std::endl;
            return 1;
        }

        if (!TestMeUser())
        {
            std::cerr << "TestMeUser failed." << std::endl;
            return 1;
        }

        if (!TestMeExport())
        {
            std::cerr << "TestMeExport failed." << std::endl;
            return 1;
        }

        return 0;
    }
    catch (const std::exception &e)
    {
        std::cerr << "Exception:\n" << e.what() << std::endl;
        return 1;
    }
}
