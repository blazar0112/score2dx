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
    for (auto i : IndexRange{0, VersionNames.size()})
    {
        auto version = ToVersionString(i);
        auto &dbVersionMusics = dbAllTimeMusics.at(version);
        mAllTimeMusics[i].reserve(dbVersionMusics.size());
        for (auto &item : dbVersionMusics.items())
        {
            mAllTimeMusics[i].emplace_back(item.value());
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

std::pair<std::size_t, std::size_t>
MusicDatabase::
FindIndexes(const std::string &officialVersionName, const std::string &dbTitle)
const
{
    std::size_t versionIndex = 0;
    std::size_t musicIndex = 0;

    if (officialVersionName!="1st&substream")
    {
        auto findVersionIndex = FindVersionIndex(officialVersionName);
        if (!findVersionIndex)
        {
            throw std::runtime_error("cannot find version index of version ["+officialVersionName+"].");
        }

        versionIndex = findVersionIndex.value();

        if (auto findMusicIndex = FindMusicIndex(versionIndex, dbTitle))
        {
            musicIndex = findMusicIndex.value();
        }
        else
        {
            throw std::runtime_error("cannot find indexes of title ["+dbTitle+"] in version ["+officialVersionName+"].");
        }
    }
    else
    {
        if (auto findMusicIndex = FindMusicIndex(0, dbTitle))
        {
            versionIndex = 0;
            musicIndex = findMusicIndex.value();
        }
        else if (auto findMusicIndex = FindMusicIndex(1, dbTitle))
        {
            versionIndex = 1;
            musicIndex = findMusicIndex.value();
        }
        else
        {
            throw std::runtime_error("cannot find version and music indexes of title ["+dbTitle+"].");
        }
    }

    return {versionIndex, musicIndex};
}

MusicInfo
MusicDatabase::
GetMusicInfo(std::size_t musicId)
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

    auto &dbChartInfos = dbMusic["difficulty"];
    for (auto &[key, value] : dbChartInfos.items())
    {
        auto styleDifficulty = ToStyleDifficulty(key);
        auto [playStyle, difficulty] = Split(styleDifficulty);
        int level = value["latest"]["level"];
        int note = value["latest"]["note"];
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

std::optional<std::size_t>
MusicDatabase::
FindMusicIndex(std::size_t versionIndex, const std::string &dbTitle)
const
{
    auto &versionMusics = mAllTimeMusics.at(versionIndex);
    if (auto findMusic = icl_s2::Find(versionMusics, dbTitle))
    {
        auto musicIndex = static_cast<std::size_t>(std::distance(versionMusics.begin(), findMusic.value()));
        return musicIndex;
    }

    return std::nullopt;
}

}
