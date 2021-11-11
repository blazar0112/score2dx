#include "score2dx/Core/MusicDatabase.hpp"

#include <fstream>
#include <iostream>

#include "icl_s2/Common/AdjacentArrayRange.hxx"
#include "icl_s2/Common/IntegralRangeUsing.hpp"
#include "icl_s2/Common/IntegralRangeList.hxx"
#include "icl_s2/StdUtil/Find.hxx"
#include "icl_s2/StdUtil/FormatString.hxx"
#include "icl_s2/String/RecursiveReplace.hpp"
#include "icl_s2/Time/TimeUtilFormat.hxx"

#include "score2dx/Iidx/Version.hpp"

namespace s2Time = icl_s2::Time;

namespace
{

//! @brief Convert non-CS availableVersions to range list.
icl_s2::IntegralRangeList<std::size_t>
ToRangeList(const std::string &availableVersions)
{
    icl_s2::IntegralRangeList<std::size_t> rangeList;

    if (icl_s2::Find(availableVersions, "cs"))
    {
        return rangeList;
    }

    auto tokens = icl_s2::SplitString(", ", availableVersions);
    for (auto &token : tokens)
    {
        if (token.size()==2)
        {
            auto versionIndex = std::stoull(token);
            rangeList.AddRange({versionIndex, versionIndex+1});
        }
        //'' for 00-29 like case, note it's range [00, 29], not [00, 29).
        else if (token.size()==5)
        {
            auto beginVersionIndex = std::stoull(token.substr(0, 2));
            auto endVersionIndex = std::stoull(token.substr(3, 2));
            rangeList.AddRange({beginVersionIndex, endVersionIndex+1});
        }
        else
        {
            std::cout << icl_s2::FormatString(tokens) << std::endl;
            throw std::runtime_error("incorrect availableVersions "+availableVersions);
        }
    }

    return rangeList;
}

bool
IsActive(std::size_t activeVersionIndex, const std::string &availableVersions)
{
    auto rangeList = ToRangeList(availableVersions);
    return rangeList.HasRange(activeVersionIndex);
}

}

namespace score2dx
{

MusicDatabase::
MusicDatabase()
{
    auto begin = s2Time::Now();
    std::ifstream databaseFile{"table/MusicDatabase29_2021-11-12.json"};
    databaseFile >> mDatabase;
    s2Time::Print<std::chrono::milliseconds>(s2Time::CountNs(begin), "Read Json");

    mAllTimeMusics.resize(VersionNames.size());
    auto &dbAllTimeMusics = mDatabase.at("version");
    for (auto versionIndex : IndexRange{0, VersionNames.size()})
    {
        auto version = ToVersionString(versionIndex);
        auto &dbVersionMusics = dbAllTimeMusics.at(version);
        mAllTimeMusics[versionIndex].reserve(dbVersionMusics.size());
        auto &musicIndexMap = mVersionMusicIndexMap[versionIndex];
        std::size_t musicIndex = 0;
        for (auto &item : dbVersionMusics.items())
        {
            auto &title = item.value();
            mAllTimeMusics[versionIndex].emplace_back(title);
            if (versionIndex==0||versionIndex==1)
            {
                m1stSubVersionIndexMap[title] = versionIndex;
            }
            musicIndexMap[title] = musicIndex;
            ++musicIndex;
        }
    }

    auto stageBegin = s2Time::Now();
    GenerateActiveVersions(GetFirstDateTimeAvailableVersionIndex());
    s2Time::Print<std::chrono::milliseconds>(s2Time::CountNs(stageBegin), "GenerateActiveVersions");

    s2Time::Print<std::chrono::milliseconds>(s2Time::CountNs(begin), "Load music database");
}

const std::vector<std::vector<std::string>> &
MusicDatabase::
GetAllTimeMusics()
const
{
    return mAllTimeMusics;
}

std::optional<std::string>
MusicDatabase::
FindDbTitle(const std::string &title)
const
{
    for (auto &section : {"csv", "display", "wiki"})
    {
        if (auto findMapping = icl_s2::Find(mDatabase["titleMapping"][section], title))
        {
            return findMapping.value().value();
        }
    }

    return std::nullopt;
}

std::optional<std::size_t>
MusicDatabase::
Find1stSubVersionIndex(const std::string &dbTitle)
const
{
    auto find1stSubMusicVersionIndex = icl_s2::Find(m1stSubVersionIndexMap, dbTitle);
    if (find1stSubMusicVersionIndex)
    {
        return find1stSubMusicVersionIndex.value()->second;
    }

    return std::nullopt;
}

std::optional<std::size_t>
MusicDatabase::
FindMusicIndex(std::size_t versionIndex, const std::string &dbTitle)
const
{
    auto findVersion = icl_s2::Find(mVersionMusicIndexMap, versionIndex);
    if (!findVersion)
    {
        throw std::runtime_error("FindMusicIndex: invalid versionIndex.");
    }

    auto findMusicIndex = icl_s2::Find(findVersion.value()->second, dbTitle);
    if (findMusicIndex)
    {
        return findMusicIndex.value()->second;
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

    auto findMusicIndex = FindMusicIndex(versionIndex, dbTitle);
    if (!findMusicIndex)
    {
        throw std::runtime_error("cannot find "+versionName+" title "+dbTitle+" music index.");
    }

    auto musicIndex = findMusicIndex.value();
    return {versionIndex, musicIndex};
}

MusicInfo
MusicDatabase::
GetLatestMusicInfo(std::size_t musicId)
const
{
    auto versionIndex = musicId/1000;
    if (versionIndex>=mAllTimeMusics.size())
    {
        throw std::runtime_error("versionIndex out of range in musicId.");
    }

    auto musicIndex = musicId%1000;
    auto &versionMusics = mAllTimeMusics.at(versionIndex);
    if (musicIndex>=versionMusics.size())
    {
        throw std::runtime_error("musicIndex out of range in musicId.");
    }

    auto &title = versionMusics[musicIndex];

    auto findDbMusic = FindDbMusic(versionIndex, title);
    if (!findDbMusic)
    {
        std::cout << "version ["+ToVersionString(versionIndex)+"] title ["+title+"].\n";
        throw std::runtime_error("cannot find music in main table and cs table.");
    }

    auto &dbMusic = *findDbMusic;

    MusicInfo musicInfo{musicId};
    musicInfo.AddField(MusicInfoField::Title, title);
    musicInfo.AddField(MusicInfoField::Genre, dbMusic["info"]["genre"]["latest"]);
    musicInfo.AddField(MusicInfoField::Artist, dbMusic["info"]["artist"]["latest"]);

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
    if (versionIndex>=mAllTimeMusics.size())
    {
        throw std::runtime_error("versionIndex out of range in musicId.");
    }

    auto musicIndex = musicId%1000;
    auto &versionMusics = mAllTimeMusics.at(versionIndex);
    if (musicIndex>=versionMusics.size())
    {
        throw std::runtime_error("musicIndex out of range in musicId.");
    }

    auto &title = versionMusics[musicIndex];

    auto version = ToVersionString(versionIndex);
    auto findVersion = icl_s2::Find(mDatabase["csMusicTable"], version);
    if (!findVersion)
    {
        return false;
    }

    auto findMusic = icl_s2::Find(findVersion.value().value(), title);
    return findMusic.has_value();
}

const Json*
MusicDatabase::
FindDbMusic(std::size_t versionIndex, const std::string &title)
const
{
    auto version = ToVersionString(versionIndex);
    auto findMusic = icl_s2::Find(mDatabase["musicTable"][version], title);
    if (findMusic)
    {
        return &(findMusic.value().value());
    }

    auto findCsMusic = icl_s2::Find(mDatabase["csMusicTable"][version], title);
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
    auto findActiveVersion = icl_s2::Find(mActiveVersions, activeVersionIndex);
    if (!findActiveVersion) { return nullptr; }

    return &(findActiveVersion.value()->second);
}

std::optional<ChartInfo>
MusicDatabase::
FindChartInfo(std::size_t titleVersionIndex,
              const std::string &dbTitle,
              StyleDifficulty styleDifficulty,
              std::size_t activeVersionIndex)
const
{
    auto version = ToVersionString(titleVersionIndex);
    auto findMusic = icl_s2::Find(mDatabase["musicTable"][version], dbTitle);
    if (!findMusic)
    {
        throw std::runtime_error("FindChartInfo: cannot find title ["+dbTitle+"] in version ["+version+"].");
    }

    auto &musicInfo = findMusic.value().value();
    if (!IsActive(activeVersionIndex, musicInfo["availableVersions"]))
    {
        return std::nullopt;
    }

    auto findDifficulty = icl_s2::Find(musicInfo["difficulty"], ToString(styleDifficulty));
    if (!findDifficulty)
    {
        return std::nullopt;
    }

    for (auto &[chartVersions, chartInfo] : findDifficulty.value().value().items())
    {
        if (IsActive(activeVersionIndex, chartVersions))
        {
            int level = chartInfo.at("level");
            int note = chartInfo.at("note");
            return {{level, note}};
        }
    }

    return std::nullopt;
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
                if (!icl_s2::Find(titleData, key))
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
                    icl_s2::RecursiveReplace(versionRangeStr, "cs", "");
                    //'' make sure comma only follow by one space.
                    icl_s2::RecursiveReplace(versionRangeStr, ", ", "|");
                    auto versionRanges = icl_s2::SplitString("|", versionRangeStr);
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

                            auto versions = icl_s2::SplitString("-", versionRange);
                            auto beginVersion = std::stoull(versions[0]);
                            auto endVersion = beginVersion;
                            if (versions.size()==2)
                            {
                                auto endVersion = std::stoull(versions[1]);
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
                    if (icl_s2::Find(chartVersion, "cs")) { continue; }

                    if (chartData["level"]==0)
                    {
                        isDiffDataValid = false;
                        std::cout << "[" << version << "][" << title << "][" << styleDifficulty
                                  << "] has 0 level at [" << chartVersion << "].\n";
                        break;
                    }

                    if (!icl_s2::Find(levelRange, chartData["level"]))
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
                            if (!icl_s2::Find(musicAvailableVersions, v))
                            {
                                isChartVersionValid = false;
                                break;
                            }

                            if (icl_s2::Find(difficultyAvailableVersions, v))
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

                for (auto itArray : icl_s2::MakeAdjacentArrayRange<2>(chartVersionByFirstVer))
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

void
MusicDatabase::
GenerateActiveVersions(std::size_t beginVersionIndex)
{
    const auto &musicTable = mDatabase.at("musicTable");

    mActiveVersions.clear();
    for (auto versionIndex : IndexRange{beginVersionIndex, GetLatestVersionIndex()+1})
    {
        mActiveVersions.emplace(versionIndex, versionIndex);
    }

    for (const auto &[version, versionMusics] : mDatabase.at("version").items())
    {
        auto versionIndex = std::stoull(version);
        for (auto musicIndex : IndexRange{0, versionMusics.size()})
        {
            auto findMusic = icl_s2::Find(musicTable.at(version), versionMusics.at(musicIndex));
            //'' cs musics.
            if (!findMusic) { continue; }

            auto &musicInfo = findMusic.value().value();

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
                            auto musicId = ToMusicId(versionIndex, musicIndex);
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

}
