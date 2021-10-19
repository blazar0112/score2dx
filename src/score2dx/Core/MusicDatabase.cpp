#include "score2dx/Core/MusicDatabase.hpp"

#include <fstream>
#include <iostream>

#include "icl_s2/Common/IntegralRangeUsing.hpp"
#include "icl_s2/StdUtil/Find.hxx"
#include "icl_s2/Time/TimeUtilFormat.hxx"

#include "score2dx/Iidx/Version.hpp"

namespace s2Time = icl_s2::Time;

namespace score2dx
{

MusicDatabase::
MusicDatabase()
{
    auto begin = s2Time::Now();
    std::ifstream databaseFile{"table/MusicDatabase29_2021-10-19.json"};
    databaseFile >> mDatabase;

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

}
