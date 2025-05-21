#include "score2dx/Core/MusicDatabase.hpp"

#include <filesystem>
#include <fstream>
#include <iostream>

#include "ies/Common/AdjacentArrayRange.hxx"
#include "ies/Common/IntegralRangeUsing.hpp"
#include "ies/StdUtil/Find.hxx"
#include "ies/String/RecursiveReplace.hpp"
#include "ies/Time/ScopeTimePrinter.hxx"
#include "ies/Time/TimeUtilFormat.hxx"

#include "score2dx/Iidx/Version.hpp"

namespace fs = std::filesystem;

namespace score2dx
{

MusicDatabase::
MusicDatabase()
{
    try
    {
        using MsTimer = ies::Time::ScopeTimePrinter<std::chrono::milliseconds>;
        MsTimer ctorTimer{"Load music database"};

        [this]()
        {
            MsTimer readJsonTimer{"MusicDatabase::ReadUsingDbToJson"};

            const std::string usingDbFilename{"table/usingDB.txt"};
            std::ifstream usingDbFile{usingDbFilename};
            if (usingDbFile)
            {
                std::string userDb;
                usingDbFile >> userDb;
                auto userDbPath = "table/"+userDb;
                if (!fs::exists(userDbPath)||!fs::is_regular_file(userDbPath))
                {
                    std::cout << "Not exist user music DB [" << userDbPath << "], use default instead.\n";
                }
                else
                {
                    mDatabaseFilename = userDbPath;
                    fmt::print("Using DB from usingDB config file: {}\n", mDatabaseFilename);
                }
            }
            else
            {
                fmt::print("Using default DB: {}\n", mDatabaseFilename);
            }

            std::ifstream databaseFile{mDatabaseFilename};
            if (!databaseFile)
            {
                throw std::runtime_error("cannot find music table: "+mDatabaseFilename);
            }
            databaseFile >> mDatabase;
        }();

        [this]()
        {
            MsTimer initializeTimer{"MusicDatabase::InitializeDbMusic"};

            auto versionCount = VersionNames.size();
            mAllTimeMusics.resize(versionCount);
            mTitleMusicIndexByVersion.resize(versionCount);

            auto &dbAllTimeMusics = mDatabase.at("version");
            for (auto versionIndex : IndexRange{0, versionCount})
            {
                auto version = ToVersionString(versionIndex);
                auto &dbVersionMusics = dbAllTimeMusics.at(version);

                auto &verMusicTable = mAllTimeMusics[versionIndex];
                verMusicTable.reserve(dbVersionMusics.size());

                auto &musicIndexMap = mTitleMusicIndexByVersion[versionIndex];

                std::size_t musicIndex = 0;
                for (auto &item : dbVersionMusics.items())
                {
                    auto& title = item.value();
                    auto musicId = ToMusicId(versionIndex, musicIndex);
                    verMusicTable.emplace_back(musicId, title);
                    auto& music = verMusicTable.back();

                    auto* findDbMusic = FindDbMusic(versionIndex, title);
                    if (!findDbMusic)
                    {
                        std::cout << "version ["+ToVersionString(versionIndex)+"] title ["+std::string{title}+"].\n";
                        throw std::runtime_error("cannot find music in main table and cs table.");
                    }

                    auto& dbMusic = *findDbMusic;

                    music.SetMusicInfoField(MusicInfoField::Genre, dbMusic["info"]["genre"]["latest"]);
                    music.SetMusicInfoField(MusicInfoField::Artist, dbMusic["info"]["artist"]["latest"]);
                    if (ies::Find(dbMusic["info"], "displayTitle"))
                    {
                        music.SetMusicInfoField(MusicInfoField::DisplayTitle, dbMusic["info"]["displayTitle"]);
                    }

                    for (auto &[styleDiffStr, diffInfo] : dbMusic.at("difficulty").items())
                    {
                        auto styleDifficulty = ToStyleDifficulty(styleDiffStr);

                        std::map<std::string, ChartInfo> chartInfoByChartVersions;
                        for (auto &[chartVersions, dbChartInfo] : diffInfo.items())
                        {
                            const int level = dbChartInfo["level"];
                            const int note = dbChartInfo["note"];
                            chartInfoByChartVersions.emplace(chartVersions, ChartInfo{level, note});
                        }

                        music.AddAvailability(styleDifficulty, chartInfoByChartVersions);
                    }

                    if (versionIndex==0||versionIndex==1)
                    {
                        m1stSubVersionIndexMap[title] = versionIndex;
                    }
                    musicIndexMap[title] = musicIndex;
                    ++musicIndex;
                }
            }

            const std::string countString = mDatabase.at("#meta").at("count");
            auto count = std::stoull(countString);
            auto latestVersionIndex = count/1000;
            auto latestVersionCount = count%1000;

            if (latestVersionIndex>=versionCount)
            {
                std::cout << "DB ID [" << countString
                          << "] cannot find latest version " << ToVersionString(latestVersionIndex)
                          << " musics.\n";
            }

            auto &latestMusics = mAllTimeMusics[latestVersionIndex];
            if (latestVersionCount!=latestMusics.size())
            {
                std::cout << "DB ID [" << countString
                          << "] latest version " << ToVersionString(latestVersionIndex)
                          << " musics count " << latestMusics.size()
                          << " is not same as ID.\n";
            }
        }();

        [this]()
        {
            MsTimer genActiveVersTimer{"MusicDatabase::GenerateSupportedActiveVersions"};
            GenerateActiveVersions(GetFirstSupportDateTimeVersionIndex());
        }();
    }
    catch (const std::exception &e)
    {
        throw std::runtime_error("MusicDatabase::MusicDatabase(): exception:\n    "+std::string{e.what()});
    }
}

const std::string &
MusicDatabase::
GetFilename()
const
{
    return mDatabaseFilename;
}

const std::vector<std::vector<Music>> &
MusicDatabase::
GetAllTimeMusics()
const
{
    return mAllTimeMusics;
}

const Music &
MusicDatabase::
GetMusic(std::size_t musicId)
const
{
    auto versionIndex = musicId/1000;
    if (versionIndex>=mAllTimeMusics.size())
    {
        throw std::runtime_error("versionIndex out of range in musicId.");
    }

    auto musicIndex = musicId%1000;
    auto &versionMusics = mAllTimeMusics[versionIndex];
    if (musicIndex>=versionMusics.size())
    {
        throw std::runtime_error("musicIndex out of range in musicId.");
    }

    return versionMusics[musicIndex];
}

const std::string &
MusicDatabase::
GetTitle(std::size_t musicId)
const
{
    return GetMusic(musicId).GetMusicInfo().GetField(MusicInfoField::Title);
}

std::optional<std::size_t>
MusicDatabase::
FindMusicId(std::size_t versionIndex, const std::string &dbTitle)
const
{
    if (versionIndex>=mTitleMusicIndexByVersion.size())
    {
        throw std::runtime_error("versionIndex out of range.");
    }

    if (auto findIndex = ies::Find(mTitleMusicIndexByVersion[versionIndex], dbTitle))
    {
        return ToMusicId(versionIndex, findIndex.value()->second);
    }

    return std::nullopt;
}

std::optional<std::string>
MusicDatabase::
FindDbTitle(const std::string &title)
const
{
    for (auto &section : {"csv", "display", "wiki"})
    {
        auto it = mDatabase["titleMapping"][section].find(title);
        if (it!=mDatabase["titleMapping"][section].cend())
        {
            return {it.value()};
        }
    }

    return std::nullopt;
}

std::optional<std::string>
MusicDatabase::
FindCsvDbTitle(const std::string &title)
const
{
    auto &csvMapping = mDatabase["titleMapping"]["csv"];
    auto it = csvMapping.find(title);
    if (it!=csvMapping.cend())
    {
        return {it.value()};
    }

    return std::nullopt;
}

std::optional<std::string>
MusicDatabase::
FindDbTitleMappingSection(const std::string &title)
const
{
    for (auto &section : {"csv", "display", "wiki"})
    {
        auto it = mDatabase["titleMapping"][section].find(title);
        if (it!=mDatabase["titleMapping"][section].cend())
        {
            return section;
        }
    }

    return std::nullopt;
}

std::optional<std::size_t>
MusicDatabase::
Find1stSubVersionIndex(const std::string &dbTitle)
const
{
    auto find1stSubMusicVersionIndex = ies::Find(m1stSubVersionIndexMap, dbTitle);
    if (find1stSubMusicVersionIndex)
    {
        return find1stSubMusicVersionIndex.value()->second;
    }

    return std::nullopt;
}

std::pair<std::size_t, std::size_t>
MusicDatabase::
FindIndexes(const std::string &versionName, const std::string &dbTitle)
const
{
    std::size_t versionIndex = 0;

    std::optional<std::size_t> findVersionIndex;
    if (versionName==Official1stSubVersionName)
    {
        findVersionIndex = Find1stSubVersionIndex(dbTitle);
    }
    else
    {
        findVersionIndex = FindVersionIndex(versionName);
    }

    if (!findVersionIndex)
    {
        throw std::runtime_error("cannot find "+versionName+" title "+dbTitle+" version index.");
    }

    versionIndex = findVersionIndex.value();

    auto &versionTitleMap = mTitleMusicIndexByVersion[versionIndex];
    auto findTitle = ies::Find(versionTitleMap, dbTitle);
    if (!findTitle)
    {
        throw std::runtime_error("cannot find "+versionName+" title "+dbTitle+" music index mapping.");
    }

    return {versionIndex, findTitle.value()->second};
}

bool
MusicDatabase::
IsCsMusic(std::size_t musicId)
const
{
    auto versionIndex = musicId/1000;
    auto &title = GetTitle(musicId);

    auto version = ToVersionString(versionIndex);
    auto findVersion = ies::Find(mDatabase["csMusicTable"], version);
    if (!findVersion)
    {
        return false;
    }

    auto findMusic = ies::Find(findVersion.value().value(), title);
    return findMusic.has_value();
}

const Json*
MusicDatabase::
FindDbMusic(std::size_t versionIndex, const std::string &title)
const
{
    auto version = ToVersionString(versionIndex);
    auto findMusic = ies::Find(mDatabase["musicTable"][version], title);
    if (findMusic)
    {
        return &(findMusic.value().value());
    }

    auto findCsMusic = ies::Find(mDatabase["csMusicTable"][version], title);
    if (findCsMusic)
    {
        return &(findCsMusic.value().value());
    }

    return nullptr;
}

const std::map<std::size_t, ActiveVersion> &
MusicDatabase::
GetActiveVersions()
const
{
    return mActiveVersions;
}

const ActiveVersion*
MusicDatabase::
FindActiveVersion(std::size_t activeVersionIndex)
const
{
    auto findActiveVersion = ies::Find(mActiveVersions, activeVersionIndex);
    if (!findActiveVersion) { return nullptr; }

    return &(findActiveVersion.value()->second);
}

const ChartInfo*
MusicDatabase::
FindChartInfo(std::size_t musicId,
              StyleDifficulty styleDifficulty,
              std::size_t availableVersionIndex)
const
{
    auto &music = GetMusic(musicId);
    auto &availability = music.GetChartAvailability(styleDifficulty, availableVersionIndex);

    switch (availability.ChartAvailableStatus)
    {
        case ChartStatus::BeginAvailable:
        case ChartStatus::Available:
            return &availability.ChartInfoProp;
        default:
            return nullptr;
    }
}

std::optional<ies::IndexRange>
MusicDatabase::
FindContainingAvailableVersionRange(std::size_t musicId,
                                    StyleDifficulty styleDifficulty,
                                    std::size_t containingVersionIndex)
const
{
    auto &music = GetMusic(musicId);
    auto &availability = music.GetChartAvailability(styleDifficulty, containingVersionIndex);

    if (availability.ChartAvailableStatus==ChartStatus::NotAvailable
        ||availability.ChartAvailableStatus==ChartStatus::Removed)
    {
        return std::nullopt;
    }

    auto beginContainingVerIndex = containingVersionIndex;
    auto endContainingVerIndex = containingVersionIndex+1;

    if (availability.ChartAvailableStatus==ChartStatus::Available)
    {
        for (auto i : ReverseIndexRange{0, containingVersionIndex})
        {
            if (music.GetChartAvailability(styleDifficulty, i).ChartAvailableStatus==ChartStatus::BeginAvailable)
            {
                beginContainingVerIndex = i;
                break;
            }
        }
    }

    if (containingVersionIndex!=GetLatestVersionIndex())
    {
        for (auto i : IndexRange{containingVersionIndex+1, VersionNames.size()})
        {
            if (music.GetChartAvailability(styleDifficulty, i).ChartAvailableStatus==ChartStatus::Removed)
            {
                endContainingVerIndex = i;
                break;
            }
        }
    }

    return IndexRange{beginContainingVerIndex, endContainingVerIndex};
}

void
MusicDatabase::
CheckValidity()
const
{
    ies::Time::ScopeTimePrinter<std::chrono::milliseconds> timePrinter{"Check DB validity"};
    std::cout << "Check DB validity...\n";

    IntRange levelRange{1, MaxLevel+1};

    auto &musicTable = mDatabase["musicTable"];
    for (auto &[version, versionMusics] : musicTable.items())
    {
        for (auto &[title, titleData] : versionMusics.items())
        {
            if (titleData.size()!=3)
            {
                std::cout << "[" << version << "][" << title << "] incorrect element size [" << titleData.size() << "].\n";
                continue;
            }
            for (auto &key : {"availableVersions", "difficulty", "info"})
            {
                if (!ies::Find(titleData, key))
                {
                    std::cout << "[" << version << "][" << title << "] lack key [" << key << "].\n";
                    continue;
                }
            }

            auto musicAvailableRangeList = ToRangeList(titleData["availableVersions"]);
            std::set<std::size_t> musicAvailableVersions;
            for (auto &range : musicAvailableRangeList.GetRanges())
            {
                for (auto v : range)
                {
                    musicAvailableVersions.emplace(v);
                }
            }

            for (auto &[styleDifficulty, diffData] : titleData["difficulty"].items())
            {
                if (!StyleDifficultySmartEnum::Has(styleDifficulty))
                {
                    std::cout << "[" << version << "][" << title << "] invalid difficulty [" << styleDifficulty << "].\n";
                    continue;
                }

                //'' check chart version format:
                auto isDiffDataValid = true;
                for (auto &[chartVersion, chartData] : diffData.items())
                {
                    std::string versionRangeStr = chartVersion;
                    //'' remove cs prefix.
                    ies::RecursiveReplace(versionRangeStr, "cs", "");
                    //'' make sure comma only follow by one space.
                    ies::RecursiveReplace(versionRangeStr, ", ", "|");
                    auto versionRanges = ies::SplitString("|", versionRangeStr);
                    auto isValid = true;
                    std::string reason;
                    for (auto &versionRange : versionRanges)
                    {
                        if (versionRange.size()!=2&&versionRange.size()!=5)
                        {
                            reason = "incorrect size";
                            isValid = false;
                            break;
                        }
                        if (versionRange.size()==5&&versionRange[2]!='-')
                        {
                            reason = "range not separated by -";
                            isValid = false;
                            break;
                        }

                        for (auto i : IndexRange{0, versionRange.size()})
                        {
                            if (i==2) { continue; }
                            if (!std::isdigit(static_cast<unsigned char>(versionRange[i])))
                            {
                                reason = "!isdigit";
                                isValid = false;
                                break;
                            }
                        }

                        if (isValid)
                        {
                            auto maxVersion = GetLatestVersionIndex();

                            auto versions = ies::SplitString("-", versionRange);
                            auto beginVersion = std::stoull(versions[0]);
                            auto endVersion = beginVersion;
                            if (versions.size()==2)
                            {
                                endVersion = std::stoull(versions[1]);
                                if (beginVersion>=endVersion)
                                {
                                    reason = "begin>=end";
                                    isValid = false;
                                }
                                if (beginVersion>=maxVersion)
                                {
                                    reason = "begin>=max";
                                    isValid = false;
                                }
                            }

                            if (endVersion>maxVersion)
                            {
                                reason = "end>max";
                                isValid = false;
                            }
                        }
                    }

                    if (!isValid)
                    {
                        isDiffDataValid = false;
                        std::cout   << "[" << version << "][" << title << "][" << styleDifficulty
                                    << "] invalid chartVersionRange [" << chartVersion << "].\n"
                                    << "Reason: " << reason << ".\n";
                    }
                }

                if (!isDiffDataValid) { continue; }

                //'' check chart version range is valid:
                //'' to make sure order of chart version is correct, not depend on lexicographic order of key string.
                std::map<std::size_t, std::string> chartVersionByFirstVer;
                std::set<std::size_t> difficultyAvailableVersions;
                for (auto &[chartVersion, chartData] : diffData.items())
                {
                    if (ies::Find(chartVersion, "cs")) { continue; }

                    if (chartData["level"]==0)
                    {
                        isDiffDataValid = false;
                        std::cout << "[" << version << "][" << title << "][" << styleDifficulty
                                  << "] has 0 level at [" << chartVersion << "].\n";
                        break;
                    }

                    if (!ies::Find(levelRange, chartData["level"]))
                    {
                        isDiffDataValid = false;
                        std::cout << "[" << version << "][" << title << "][" << styleDifficulty
                                  << "] has invalid level [" << chartData["level"]
                                  << "] at [" << chartVersion << "].\n";
                        break;
                    }

                    if (chartData["note"]==0)
                    {
                        isDiffDataValid = false;
                        std::cout << "[" << version << "][" << title << "][" << styleDifficulty
                                  << "] has 0 note at [" << chartVersion << "].\n";
                        break;
                    }

                    auto chartVerRangeList = ToRangeList(chartVersion);
                    auto chartVerRanges = chartVerRangeList.GetRanges();
                    if (chartVerRanges.empty())
                    {
                        isDiffDataValid = false;
                        std::cout << "[" << version << "][" << title << "][" << styleDifficulty
                                  << "] has invalid chart version [" << chartVersion << "].\n";
                        break;
                    }

                    auto isChartVersionValid = true;
                    for (auto &verRange : chartVerRanges)
                    {
                        for (auto v : verRange)
                        {
                            if (!ies::Find(musicAvailableVersions, v))
                            {
                                isChartVersionValid = false;
                                break;
                            }

                            if (ies::Find(difficultyAvailableVersions, v))
                            {
                                isChartVersionValid = false;
                                break;
                            }

                            difficultyAvailableVersions.emplace(v);
                        }

                        if (!isChartVersionValid) { break; }
                    }
                    if (!isChartVersionValid)
                    {
                        isDiffDataValid = false;
                        std::cout << "[" << version << "][" << title << "][" << styleDifficulty
                                  << "] has invalid chart version [" << chartVersion << "].\n";
                        break;
                    }

                    auto firstVer = chartVerRangeList.GetRanges().begin()->GetMin();
                    chartVersionByFirstVer[firstVer] = chartVersion;
                }

                if (!isDiffDataValid) { continue; }

                //'' check adjacent chart infos are not same.

                for (auto itArray : ies::MakeAdjacentArrayRange<2>(chartVersionByFirstVer))
                {
                    auto &previousChartVer = itArray[0]->second;
                    auto &nextChartVer = itArray[1]->second;
                    if (diffData[previousChartVer]==diffData[nextChartVer])
                    {
                        std::cout << "[" << version << "][" << title << "][" << styleDifficulty
                                  << "] has same chart info for [" << previousChartVer
                                  << "] and [" << nextChartVer << "].\n";
                        break;
                    }
                }
            }
        }
    }


    std::cout << "Check DB validity done.\n";
}

void
MusicDatabase::
GenerateActiveVersions(std::size_t beginVersionIndex)
{
    mActiveVersions.clear();
    for (auto versionIndex : IndexRange{beginVersionIndex, VersionNames.size()})
    {
        mActiveVersions.emplace(versionIndex, versionIndex);
    }

    for (auto versionIndex : IndexRange{0, mAllTimeMusics.size()})
    {
        const auto &versionMusics = mAllTimeMusics[versionIndex];
        for (auto musicIndex : IndexRange{0, versionMusics.size()})
        {
            auto &music = versionMusics[musicIndex];

            for (auto &[activeVersionIndex, activeVersion] : mActiveVersions)
            {
                for (auto styleDifficulty : StyleDifficultySmartEnum::ToRange())
                {
                    auto &availability = music.GetChartAvailability(styleDifficulty, activeVersionIndex);
                    if (availability.ChartAvailableStatus==ChartStatus::BeginAvailable
                        ||availability.ChartAvailableStatus==ChartStatus::Available)
                    {
                        activeVersion.AddDifficulty(music.GetMusicId(), styleDifficulty, availability.ChartInfoProp);
                    }
                }
            }
        }
    }
}

void
UpgradeMusicDatabase(const std::string &currentFilename,
                     const std::string &newFilename)
{
    std::cout << "Loading current music table from " << currentFilename << "\n";

    Json inputDb;
    {
        std::ifstream databaseFile{currentFilename};
        if (!databaseFile)
        {
            throw std::runtime_error("cannot find music table: "+currentFilename);
        }
        databaseFile >> inputDb;
    }

    const auto &db = inputDb;

    std::string versionStr = db["#meta"]["version"];
    auto version = std::stoull(versionStr);
    auto nextVersion = version+1;
    auto nextVersionStr = ToVersionString(nextVersion);

    std::cout << "Version: " << versionStr << "\n"
              << "NextVersion: " << nextVersionStr << "\n";

    Json nextDb;
    nextDb["#meta"] = db["#meta"];
    nextDb["#meta"]["count"] = ToMusicIdString(ToMusicId(nextVersion, 0));
    nextDb["#meta"]["version"] = nextVersionStr;

    nextDb["csMusicTable"] = db["csMusicTable"];
    nextDb["titleMapping"] = db["titleMapping"];
    nextDb["version"] = db["version"];
    std::vector<std::string> versionMusic;
    nextDb["version"][nextVersionStr] = versionMusic;
    versionMusic = nextDb["version"][versionStr].get<decltype(versionMusic)>();
    std::sort(versionMusic.begin(), versionMusic.end());
    nextDb["version"][versionStr] = versionMusic;

    nextDb["musicTable"] = db["musicTable"];

    auto ReplaceString = [](std::string &s, const std::string &from, const std::string &to)
    {
        auto pos = s.find(from, 0);
        if (pos!=std::string::npos)
        {
            s.replace(pos, from.size(), to);
        }
    };

    auto UpdateVersionRangeList =
        [&versionStr, &nextVersionStr, &ReplaceString]
        (std::string versionRangeList)
        -> std::string
    {
        if (ies::Find(versionRangeList, versionStr))
        {
            //'' e.g. [start-29] to [start-30]
            if (ies::Find(versionRangeList, "-"+versionStr))
            {
                ReplaceString(versionRangeList, versionStr, nextVersionStr);
            }
            //'' e.g. [29] to [29-30], [..., 29] to [..., 29-30]
            else
            {
                ReplaceString(versionRangeList, versionStr, versionStr+"-"+nextVersionStr);
            }
        }
        return versionRangeList;
    };

    for (auto &[verStr, verMusicInfoMap] : nextDb["musicTable"].items())
    {
        for (auto &[title, musicInfo] : verMusicInfoMap.items())
        {
            auto updatedAvailableVersions = UpdateVersionRangeList(musicInfo["availableVersions"]);
            musicInfo["availableVersions"] = updatedAvailableVersions;
            Json updatedDifficulty;
            for (auto &[chartDifficulty, history] : musicInfo["difficulty"].items())
            {
                auto &updatedHistory = updatedDifficulty[chartDifficulty];
                updatedHistory = Json::object();
                for (auto &[versionRangeList, chartInfo] : history.items())
                {
                    auto updatedVersionRangeList = UpdateVersionRangeList(versionRangeList);
                    updatedHistory[updatedVersionRangeList] = chartInfo;
                }
            }

            musicInfo["difficulty"] = updatedDifficulty;
        }
    }

    nextDb["musicTable"][nextVersionStr] = Json::object();

    std::ofstream outputFile{newFilename};
    if (!outputFile)
    {
        throw std::runtime_error("cannot create new music database file: "+newFilename);
    }

    outputFile << std::setw(4) << nextDb;
}

}
