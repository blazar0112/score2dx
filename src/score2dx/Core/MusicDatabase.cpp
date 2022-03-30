#include "score2dx/Core/MusicDatabase.hpp"

#include <filesystem>
#include <fstream>
#include <iostream>

#include "ies/Common/AdjacentArrayRange.hxx"
#include "ies/Common/IntegralRangeUsing.hpp"
#include "ies/StdUtil/Find.hxx"
#include "ies/StdUtil/FormatString.hxx"
#include "ies/String/RecursiveReplace.hpp"
#include "ies/Time/TimeUtilFormat.hxx"

#include "score2dx/Core/CheckedParse.hxx"
#include "score2dx/Iidx/Version.hpp"

namespace fs = std::filesystem;
namespace s2Time = ies::Time;

namespace
{

/*
bool
IsActive(std::size_t activeVersionIndex, const std::string &availableVersions)
{
    auto rangeList = ToRangeList(availableVersions);
    return rangeList.HasRange(activeVersionIndex);
}
*/

}

namespace score2dx
{

MusicDatabase::
MusicDatabase()
{
    try
    {
        auto begin = s2Time::Now();

        {
            std::string usingDbFilename{"table/usingDB.txt"};
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
                }
            }
        }

        std::cout << "Loading music table from " << mDatabaseFilename << "\n";
        std::ifstream databaseFile{mDatabaseFilename};
        if (!databaseFile)
        {
            throw std::runtime_error("cannot find music table: "+mDatabaseFilename);
        }
        databaseFile >> mDatabase;
        s2Time::Print<std::chrono::milliseconds>(s2Time::CountNs(begin), "Read Json");

        auto versionCount = GetLatestVersionIndex()+1;
        mAllTimeMusicContexts.resize(versionCount);
        auto &dbAllTimeMusics = mDatabase.at("version");
        for (auto versionIndex : IndexRange{0, versionCount})
        {
            auto version = ToVersionString(versionIndex);
            auto &dbVersionMusics = dbAllTimeMusics.at(version);
            mAllTimeMusicContexts[versionIndex].resize(dbVersionMusics.size());
            auto &musicIndexMap = mVersionMusicIndexMap[versionIndex];
            std::size_t musicIndex = 0;
            for (auto &item : dbVersionMusics.items())
            {
                auto &title = item.value();
                auto &context = mAllTimeMusicContexts[versionIndex][musicIndex];
                context.MusicId = ToMusicId(versionIndex, musicIndex);
                context.Title = title;

                auto findDbMusic = FindDbMusic(versionIndex, title);
                if (!findDbMusic)
                {
                    std::cout << "version ["+ToVersionString(versionIndex)+"] title ["+context.Title+"].\n";
                    throw std::runtime_error("cannot find music in main table and cs table.");
                }

                context.Data = findDbMusic;

                if (versionIndex==0||versionIndex==1)
                {
                    m1stSubVersionIndexMap[title] = versionIndex;
                }
                musicIndexMap[title] = musicIndex;
                ++musicIndex;
            }
        }

        std::string countString = mDatabase.at("#meta").at("count");
        auto count = std::stoull(countString);
        auto latestVersionIndex = count/1000;
        auto latestVersionCount = count%1000;

        if (latestVersionIndex>=versionCount)
        {
            std::cout << "DB ID [" << countString
                      << "] cannot find latest version " << ToVersionString(latestVersionIndex)
                      << " musics.\n";
        }

        auto &latestMusics = mAllTimeMusicContexts[latestVersionIndex];
        if (latestVersionCount!=latestMusics.size())
        {
            std::cout << "DB ID [" << countString
                      << "] latest version " << ToVersionString(latestVersionIndex)
                      << " musics count " << latestMusics.size()
                      << " is not same as ID.\n";
        }

        auto stageBegin = s2Time::Now();
        GenerateActiveVersions(GetFirstDateTimeAvailableVersionIndex());
        s2Time::Print<std::chrono::milliseconds>(s2Time::CountNs(stageBegin), "GenerateActiveVersions");

        s2Time::Print<std::chrono::milliseconds>(s2Time::CountNs(begin), "Load music database");
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

const std::vector<std::vector<DbMusicContext>> &
MusicDatabase::
GetAllTimeMusicContexts()
const
{
    return mAllTimeMusicContexts;
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

    auto findContext = FindDbMusicContext(versionIndex, dbTitle);
    if (!findContext)
    {
        throw std::runtime_error("cannot find "+versionName+" title "+dbTitle+" music context.");
    }

    auto &context = *findContext;

    return {versionIndex, GetMusicIndex(context.MusicId)};
}

MusicInfo
MusicDatabase::
GetLatestMusicInfo(std::size_t musicId)
const
{
    auto dbMusicContext = GetDbMusicContext(musicId);
    auto &dbMusic = *dbMusicContext.Data;
    auto &title = dbMusicContext.Title;

    MusicInfo musicInfo{musicId};
    musicInfo.AddField(MusicInfoField::Title, title);
    musicInfo.AddField(MusicInfoField::Genre, dbMusic["info"]["genre"]["latest"]);
    musicInfo.AddField(MusicInfoField::Artist, dbMusic["info"]["artist"]["latest"]);
    std::string displayTitle;
    if (ies::Find(dbMusic["info"], "displayTitle"))
    {
        displayTitle = dbMusic["info"]["displayTitle"];
    }
    musicInfo.AddField(MusicInfoField::DisplayTitle, displayTitle);

    auto &dbDiffList = dbMusic["difficulty"];
    for (auto &[diff, diffData] : dbDiffList.items())
    {
        auto styleDifficulty = ToStyleDifficulty(diff);
        auto [playStyle, difficulty] = Split(styleDifficulty);
        auto &latestChartData = diffData.back();
        int level = latestChartData["level"];
        int note = latestChartData["note"];
        musicInfo.AddChartInfo(playStyle, difficulty, {level, note});
    }

    return musicInfo;
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
              std::size_t activeVersionIndex)
const
{
    auto &context = GetDbMusicContext(musicId);

    auto findActiveVersion = FindActiveVersion(activeVersionIndex);
    if (!findActiveVersion)
    {
        return nullptr;
    }

    auto &activeVersion = *findActiveVersion;

    auto findChartInfo = activeVersion.FindChartInfo(context.MusicId, styleDifficulty);
    if (findChartInfo)
    {
        return findChartInfo;
    }

    return nullptr;
}

bool
MusicDatabase::
IsAvailable(std::size_t musicId,
            StyleDifficulty styleDifficulty,
            std::size_t versionIndex)
const
{
    auto findActiveVersion = ies::Find(mActiveVersions, versionIndex);
    if (!findActiveVersion)
    {
        return false;
    }

    auto &activeVersion = findActiveVersion.value()->second;
    return activeVersion.FindChartInfo(musicId, styleDifficulty);
}

std::optional<ies::IndexRange>
MusicDatabase::
FindContainingAvailableVersionRange(std::size_t musicId,
                                    StyleDifficulty styleDifficulty,
                                    std::size_t containingVersionIndex)
const
{
    auto &context = GetDbMusicContext(musicId);
    auto &dbMusic = *context.Data;

    auto findDiffInfo = ies::Find(dbMusic.at("difficulty"), ToString(styleDifficulty));
    if (!findDiffInfo)
    {
        std::cout << "music id ["+ToMusicIdString(musicId)+"] has not difficulty ["+ToString(styleDifficulty)+"].\n";
        throw std::runtime_error("cannot difficulty.");
    }

    auto &diffInfo = findDiffInfo.value().value();

    auto containingVersion = ToVersionString(containingVersionIndex);

    for (auto &[chartVersions, chartInfo] : diffInfo.items())
    {
        std::string_view versionRangesView{chartVersions};
        if (versionRangesView.size()==2)
        {
            if (containingVersion==versionRangesView)
            {
                return IndexRange{containingVersionIndex, containingVersionIndex+1};
            }
            continue;
        }

        std::size_t firstVersionIndex = 0;
        std::size_t lastVersionIndex = 0;
        CheckedParse(versionRangesView.substr(0, 2), firstVersionIndex, "versionRanges[firstVersionIndex]");
        CheckedParse(versionRangesView.substr(versionRangesView.size()-2, 2), lastVersionIndex, "versionRanges[lastVersionIndex]");
        IndexRange maxVersionRange{firstVersionIndex, lastVersionIndex+1};
        if (!maxVersionRange.IsInRange(containingVersionIndex))
        {
            continue;
        }

        auto rangeList = ToRangeList(chartVersions);
        for (auto &range : rangeList.GetRanges())
        {
            if (range.IsInRange(containingVersionIndex))
            {
                return range;
            }
        }
    }

    return std::nullopt;
}

ies::IntegralRangeList<std::size_t>
MusicDatabase::
GetAvailableVersions(std::size_t musicId)
const
{
    auto context = GetDbMusicContext(musicId);
    auto &versions = context.Data->at("availableVersions");
    auto availableVersions = ToRangeList(versions);
    return availableVersions;
}

void
MusicDatabase::
CheckValidity()
const
{
    std::cout << "Check DB validity...\n";

    auto begin = s2Time::Now();

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
                            if (i==2) continue;
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

    s2Time::Print<std::chrono::milliseconds>(s2Time::CountNs(begin), "Check DB validity");
    std::cout << "Check DB validity done.\n";
}

const DbMusicContext*
MusicDatabase::
FindDbMusicContext(std::size_t versionIndex, const std::string &dbTitle)
const
{
    if (versionIndex>=mAllTimeMusicContexts.size())
    {
        throw std::runtime_error("versionIndex out of range in musicId.");
    }

    auto &versionMusics = mAllTimeMusicContexts[versionIndex];
    for (auto musicIndex : IndexRange{0, versionMusics.size()})
    {
        auto &context = versionMusics[musicIndex];
        if (context.Title==dbTitle)
        {
            return &context;
        }
    }

    return nullptr;
}

const DbMusicContext &
MusicDatabase::
GetDbMusicContext(std::size_t musicId)
const
{
    auto versionIndex = musicId/1000;
    if (versionIndex>=mAllTimeMusicContexts.size())
    {
        throw std::runtime_error("versionIndex out of range in musicId.");
    }

    auto musicIndex = musicId%1000;
    auto &versionMusics = mAllTimeMusicContexts[versionIndex];
    if (musicIndex>=versionMusics.size())
    {
        throw std::runtime_error("musicIndex out of range in musicId.");
    }

    return mAllTimeMusicContexts[versionIndex][musicIndex];
}

void
MusicDatabase::
GenerateActiveVersions(std::size_t beginVersionIndex)
{
    mActiveVersions.clear();
    for (auto versionIndex : IndexRange{beginVersionIndex, GetLatestVersionIndex()+1})
    {
        mActiveVersions.emplace(versionIndex, versionIndex);
    }

    for (auto versionIndex : IndexRange{0, mAllTimeMusicContexts.size()})
    {
        const auto &versionMusics = mAllTimeMusicContexts[versionIndex];
        for (auto musicIndex : IndexRange{0, versionMusics.size()})
        {
            auto musicId = ToMusicId(versionIndex, musicIndex);
            auto &context = versionMusics[musicIndex];
            auto &musicInfo = *context.Data;

            for (auto &[styleDiffStr, diffInfo] : musicInfo.at("difficulty").items())
            {
                auto styleDifficulty = ToStyleDifficulty(styleDiffStr);
                for (auto &[chartVersions, chartInfo] : diffInfo.items())
                {
                    auto chartAvailableRangeList = ToRangeList(chartVersions);
                    for (auto &[activeVersionIndex, activeVersion] : mActiveVersions)
                    {
                        if (chartAvailableRangeList.HasRange(activeVersionIndex))
                        {
                            int level = chartInfo.at("level");
                            int note = chartInfo.at("note");
                            activeVersion.AddDifficulty(musicId, styleDifficulty, {level, note});
                        }
                    }
                }
            }
        }
    }
}

const std::string &
MusicDatabase::
GetTitle(std::size_t musicId)
const
{
    auto versionIndex = musicId/1000;
    if (versionIndex>=mAllTimeMusicContexts.size())
    {
        throw std::runtime_error("versionIndex out of range in musicId.");
    }

    auto musicIndex = musicId%1000;
    auto &versionMusics = mAllTimeMusicContexts[versionIndex];
    if (musicIndex>=versionMusics.size())
    {
        throw std::runtime_error("musicIndex out of range in musicId.");
    }

    return versionMusics[musicIndex].Title;
}

std::string
ToString(const ies::IntegralRangeList<std::size_t> &availableVersions)
{
    std::string s;
    auto ranges = availableVersions.GetRanges();
    auto isFirst = true;
    for (auto &range : ranges)
    {
        if (!isFirst) s += ", ";

        if (range.size()==1)
        {
            s += ToVersionString(range.GetMin());
        }
        else
        {
            s += ToVersionString(range.GetMin())+"-"+ToVersionString(range.GetMax());
        }

        isFirst = false;
    }

    return s;
}

ies::IntegralRangeList<std::size_t>
ToRangeList(const std::string &availableVersions)
{
    ies::IntegralRangeList<std::size_t> rangeList;

    if (ies::Find(availableVersions, "cs"))
    {
        return rangeList;
    }

    std::size_t start = 0;
    std::size_t end = 0;

    std::string_view view{availableVersions};

    while ((start = view.find_first_not_of(", ", end))!=std::string_view::npos)
    {
        end = view.find(',', start+1);
        if (end==std::string_view::npos)
        {
            end = view.length();
        }

        auto versionRange = view.substr(start, end-start);
        if (versionRange.size()==2)
        {
            std::size_t versionIndex = 0;
            CheckedParse(versionRange, versionIndex, "versionRange[SingleVersionIndex]");
            rangeList.AddRange({versionIndex, versionIndex+1});
        }
        //'' for 00-29 like case, note it's range [00, 29], not [00, 29).
        else if (versionRange.size()==5)
        {
            std::size_t beginVersionIndex = 0;
            std::size_t endVersionIndex = 0;
            CheckedParse(versionRange.substr(0, 2), beginVersionIndex, "versionRange[beginVersionIndex]");
            CheckedParse(versionRange.substr(3, 2), endVersionIndex, "versionRange[endVersionIndex]");
            rangeList.AddRange({beginVersionIndex, endVersionIndex+1});
        }
        else
        {
            std::cout << versionRange << std::endl;
            throw std::runtime_error("incorrect availableVersions "+availableVersions);
        }
    }

    return rangeList;
}

}
