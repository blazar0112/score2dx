#include "score2dx/Core/Core.hpp"

#include <ctime>
#include <filesystem>
#include <fstream>

#include "fmt/chrono.h"

#include "icl_s2/Common/IntegralRangeUsing.hpp"
#include "icl_s2/StdUtil/Find.hxx"
#include "icl_s2/StdUtil/FormatString.hxx"
#include "icl_s2/Time/TimeUtilFormat.hxx"

#include "score2dx/Iidx/Version.hpp"

namespace fs = std::filesystem;
namespace s2Time = icl_s2::Time;

namespace score2dx
{

Core::
Core()
{
    auto jsonBegin = s2Time::Now();
    std::ifstream databaseFile{"table/MusicDatabase28_20211006.json"};
    databaseFile >> mMusicDatabase;

    mAllVersionMusics.resize(VersionNames.size());
    auto &dbAllVersionMusics = mMusicDatabase.at("version");
    for (auto i : IndexRange{0, VersionNames.size()})
    {
        auto versionIndex = fmt::format("{:02}", i);
        auto &dbVersionMusics = dbAllVersionMusics.at(versionIndex);
        mAllVersionMusics[i].reserve(dbVersionMusics.size());
        for (auto &item : dbVersionMusics.items())
        {
            mAllVersionMusics[i].emplace_back(item.value());
        }
    }

    s2Time::Print<std::chrono::milliseconds>(s2Time::CountNs(jsonBegin), "Load music database");
}

void
Core::
AddPlayer(const std::string &iidxId)
{
    if (!IsIidxId(iidxId))
    {
        return;
    }

    if (!icl_s2::Find(mPlayerScores, iidxId))
    {
        CreatePlayer(iidxId);
    }
}

std::optional<std::string>
Core::
FindDbTitle(const std::string &title)
const
{
    for (auto &section : {"csv", "display", "wiki"})
    {
        if (auto findMapping = icl_s2::Find(mMusicDatabase["titleMapping"][section], title))
        {
            return findMapping.value().value();
        }
    }

    return std::nullopt;
}

std::optional<std::size_t>
Core::
FindMusicIndex(std::size_t versionIndex, const std::string &dbTitle)
const
{
    auto &versionMusics = GetVersionMusics(versionIndex);
    if (auto findMusic = icl_s2::Find(versionMusics, dbTitle))
    {
        auto musicIndex = static_cast<std::size_t>(std::distance(versionMusics.begin(), findMusic.value()));
        return musicIndex;
    }

    return std::nullopt;
}

std::pair<std::size_t, std::size_t>
Core::
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

const std::vector<std::string> &
Core::
GetVersionMusics(std::size_t versionIndex)
const
{
    return mAllVersionMusics.at(versionIndex);
}

MusicInfo
Core::
GetMusicInfo(std::size_t musicId)
const
{
    auto versionIndex = musicId/1000;
    if (versionIndex>=mAllVersionMusics.size())
    {
        throw std::runtime_error("versionIndex out of range in musicId.");
    }

    auto musicIndex = musicId%1000;
    auto &versionMusics = GetVersionMusics(versionIndex);
    if (musicIndex>=versionMusics.size())
    {
        throw std::runtime_error("musicIndex out of range in musicId.");
    }

    auto &title = versionMusics[musicIndex];

    auto findDbMusic = FindDbMusic(versionIndex, title);
    if (!findDbMusic)
    {
        std::cout << "versionJsonKey ["+ToDbVersionKey(versionIndex)+"] title ["+title+"].\n";
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
Core::
IsCsMusic(std::size_t musicId)
const
{
    auto versionIndex = musicId/1000;
    if (versionIndex>=mAllVersionMusics.size())
    {
        throw std::runtime_error("versionIndex out of range in musicId.");
    }

    auto musicIndex = musicId%1000;
    auto &versionMusics = GetVersionMusics(versionIndex);
    if (musicIndex>=versionMusics.size())
    {
        throw std::runtime_error("musicIndex out of range in musicId.");
    }

    auto &title = versionMusics[musicIndex];

    auto versionJsonKey = fmt::format("{:02}", versionIndex);
    auto findVersion = icl_s2::Find(mMusicDatabase["csMusicTable"], versionJsonKey);
    if (!findVersion)
    {
        return false;
    }

    auto findMusic = icl_s2::Find(findVersion.value().value(), title);
    return findMusic.has_value();
}

bool
Core::
LoadDirectory(std::string_view directory, bool verbose)
{
    if (!fs::exists(directory)||!fs::is_directory(directory))
    {
        if (verbose)
        {
            std::cout << "directory is not a valid directory.\n";
        }
        return false;
    }

    auto path = fs::canonical(directory).lexically_normal();
    auto id = path.filename().string().substr(0, 9);
    if (!IsIidxId(id))
    {
        if (verbose)
        {
            std::cout << "directory is not a valid IIDX ID.\n";
        }
        return false;
    }

    if (!icl_s2::Find(mPlayerScores, id))
    {
        CreatePlayer(id);
    }

    auto &playerScore = mPlayerScores.at(id);

    for (auto &entry : fs::directory_iterator{directory})
    {
        if (entry.is_regular_file()&&entry.path().extension()==".csv")
        {
            if (verbose) std::cout << "\n";
            auto filename = entry.path().filename().string();

            auto [isValid, invalidReason] = IsValidCsvFilename(filename);
            if (!isValid)
            {
                if (verbose) std::cout << "Skip CSV ["+filename+"] with invalid filename: "+invalidReason+".\n";
                continue;
            }

            if (!filename.starts_with(id))
            {
                if (verbose) std::cout << "Skip CSV ["+filename+"] with unmatch IIDX ID.\n";
                continue;
            }

            if (verbose) std::cout << "Load CSV [" << filename << "]\n";
            std::unique_ptr<Csv> csvPtr;
            try
            {
                csvPtr = std::make_unique<Csv>(entry.path().string(), mMusicDatabase, verbose);
            }
            catch (const std::exception &e)
            {
                std::cout << "Construct CSV ["+filename+"] failed, reason:\n    " << e.what() << "\n";
                continue;
            }

            if (!csvPtr)
            {
                throw std::runtime_error("Cannot construct CSV.");
            }

            auto &csv = *csvPtr;
            if (verbose)
            {
                csv.PrintSummary();
            }

            auto dateTime = csv.GetLastDateTime();
            auto playStyle = csv.GetPlayStyle();
            auto &allTimeCsvs = mAllPlayerCsvs.at(id).at(playStyle);
            allTimeCsvs[dateTime] = std::move(csvPtr);

            AddCsvToPlayerScore(id, playStyle, dateTime);
        }

        if (entry.is_regular_file()&&entry.path().extension()==".json")
        {
            auto filename = entry.path().filename().string();
            if (filename.starts_with("score2dx_export_"))
            {
                Import(id, entry.path().string(), verbose);
            }
        }
    }

    if (verbose)
    {
        auto &playerCsvs = mAllPlayerCsvs.at(id);
        std::cout << "Player IIDX ID ["+playerScore.GetIidxId()+"] CSV files:\n";
        for (auto playStyle : PlayStyleSmartEnum::ToRange())
        {
            std::cout << "["+ToString(playStyle)+"]:\n";
            for (auto &[dateTime, csvPtr] : playerCsvs.at(playStyle))
            {
                auto &csv = *csvPtr;
                std::cout   << "DateTime ["+dateTime
                            << "] Filename ["+csv.GetFilename()
                            << "] Version ["+csv.GetVersion()
                            << "] TotalPlayCount [" << csv.GetTotalPlayCount()
                            << "]\n";
            }
        }
    }

    return true;
}

std::map<std::string, const Csv*>
Core::
GetCsvs(const std::string &iidxId, PlayStyle playStyle)
const
{
    std::map<std::string, const Csv*> csvs;
    for (auto &[dateTime, csv] : mAllPlayerCsvs.at(iidxId).at(playStyle))
    {
        csvs[dateTime] = csv.get();
    }
    return csvs;
}

void
Core::
Export(const std::string &iidxId,
       PlayStyle playStyle,
       const std::string &outputDirectory,
       const std::string &dateTimeType,
       const std::string &suffix)
const
{
    try
    {
        if (!fs::exists(outputDirectory)||!fs::is_directory(outputDirectory))
        {
            throw std::runtime_error("outputDirectory ["+outputDirectory+"] is not a directory.");
        }

        auto findPlayerScore = icl_s2::Find(mPlayerScores, iidxId);
        if (!findPlayerScore)
        {
            return;
        }

        auto &playerScore = findPlayerScore.value()->second;
        auto &musicScores = playerScore.GetMusicScores(playStyle);
        if (musicScores.empty())
        {
            return;
        }

        auto playStyleAcronym = static_cast<PlayStyleAcronym>(playStyle);

        auto GetCurrentDate = []() -> std::string
        {
            auto t = std::time(nullptr);
            auto date = fmt::format("{:%Y-%m-%d}", fmt::localtime(t));
            return date;
        };

        auto directoryPath = fs::canonical(outputDirectory);
        auto filename = "score2dx_export_"+ToString(playStyleAcronym)+"_"+GetCurrentDate();
        if (!suffix.empty())
        {
            filename += "_"+suffix;
        }
        filename += ".json";

        auto path = (directoryPath / filename).lexically_normal();

        std::ofstream exportFile{path};
        if (!exportFile)
        {
            throw std::runtime_error("cannot open file ["+path.string()+"].");
        }

        Json exportData;
        auto &metadata = exportData["metadata"];
        metadata["id"] = iidxId;
        metadata["playStyle"] = ToString(playStyle);
        metadata["dateTimeType"] = dateTimeType;
        metadata["scoreVersion"] = "";
        //'' todo: decide score version if all score in a version.

        std::string lastDateTime;

        auto &data = exportData["data"];

        for (auto &[musicId, dateTimeScores] : musicScores)
        {
            auto musicInfo = GetMusicInfo(musicId);
            auto &title = musicInfo.GetField(MusicInfoField::Title);
            auto versionIndex = ToIndexes(musicId).first;
            auto versionName = VersionNames[versionIndex];
            if (versionIndex==0||versionIndex==0)
            {
                versionName = "1st&substream";
            }

            auto &versionData = data[versionName];
            auto &titleData = versionData[title];

            for (auto &[dateTime, musicScore] : dateTimeScores)
            {
                if (dateTime>lastDateTime)
                {
                    lastDateTime = dateTime;
                }

                auto &record = titleData[dateTime];
                record["play"] = musicScore.GetPlayCount();
                auto &scoreData = record["score"];

                for (auto &[difficulty, chartScore] : musicScore.GetChartScores())
                {
                    auto diffAcronym = static_cast<DifficultyAcronym>(difficulty);

                    //! @brief [score, pgreat, great, miss, clear, djLevel], same order as CSV.
                    std::array<std::string, 6> difficultyData;

                    difficultyData[0] = std::to_string(chartScore.ExScore);
                    difficultyData[1] = std::to_string(chartScore.PGreatCount);
                    difficultyData[2] = std::to_string(chartScore.GreatCount);
                    difficultyData[3] = "---";
                    if (chartScore.MissCount)
                    {
                        difficultyData[3] = std::to_string(chartScore.MissCount.value());
                    }

                    difficultyData[4] = ToString(chartScore.ClearType);
                    difficultyData[5] = ToString(chartScore.DjLevel);
                    if (chartScore.ExScore==0)
                    {
                        difficultyData[5] = "---";
                    }

                    scoreData[ToString(diffAcronym)] = difficultyData;
                }
            }
        }

        metadata["lastDateTime"] = lastDateTime;

        exportFile << exportData << std::endl;

        //std::cout << "Filename: [" << path << "]\n";
    }
    catch (const std::exception &e)
    {
        throw std::runtime_error("Core::Export(): exception:\n    "+std::string{e.what()});
    }
}

void
Core::
Import(const std::string &requiredIidxId,
       const std::string &exportedFilename,
       bool verbose)
{
    try
    {
        (void)requiredIidxId;
        if (!fs::exists(exportedFilename)||fs::is_directory(exportedFilename))
        {
            throw std::runtime_error("exportedFilename ["+exportedFilename+"] is not a file.");
        }

        static const std::string signature = "score2dx_export_";

        auto path = fs::canonical(exportedFilename).lexically_normal();
        auto filename = path.filename().string();
        if (!filename.starts_with(signature)
            ||!filename.ends_with(".json")
            ||filename.size()<MinExportFilenameSize)
        {
            throw std::runtime_error("incorrect filename format ["+filename+"].");
        }

        auto filenamePlayStyleAcronym = ToPlayStyleAcronym(filename.substr(signature.size(), 2));
        auto filenamePlayStyle = static_cast<PlayStyle>(filenamePlayStyleAcronym);

        std::ifstream file{path};
        if (!file)
        {
            throw std::runtime_error("cannot open file ["+path.string()+"].");
        }

        Json exportedData;

        file >> exportedData;

        const auto &constExportedData = exportedData;

        auto &metadata = constExportedData["metadata"];
        auto metaPlayStyle = ToPlayStyle(metadata["playStyle"]);
        if (metaPlayStyle!=filenamePlayStyle)
        {
            std::cout << "warning: filename playstyle not agreed with metadata in exported json.\n";
        }

        std::string iidxId = metadata["id"];
        if (iidxId!=requiredIidxId)
        {
            if (verbose)
            {
                std::cout << "warning: exported data's IIDX ID [" << iidxId
                          << "] is not required [" << requiredIidxId
                          << "]. Skipped file [" << filename << "]\n";
            }
            return;
        }

        if (!icl_s2::Find(mPlayerScores, iidxId))
        {
            CreatePlayer(iidxId);
        }

        auto &playerScore = mPlayerScores.at(iidxId);

        auto &data = constExportedData["data"];
        if (data.empty())
        {
            throw std::runtime_error("data has empty entry.");
        }

        auto checkDateTime = (metadata["dateTimeType"]=="scriptUpdateDate");
        (void)checkDateTime;

        auto firstVersionItem = data.items().begin();
        auto firstVersion = firstVersionItem.key();
        auto &firstVersionData = firstVersionItem.value();
        if (firstVersionData.empty())
        {
            throw std::runtime_error("first version has empty music.");
        }

        auto firstMusicItem = firstVersionData.items().begin();
        auto firstMusic = firstMusicItem.key();
        auto &firstMusicData = firstMusicItem.value();
        if (firstMusicData.empty())
        {
            throw std::runtime_error("first version first music has empty date time.");
        }

        auto firstDateTime = firstMusicData.items().begin().key();

        for (auto &[versionName, versionData] : data.items())
        {
            for (auto &[title, musicData] : versionData.items())
            {
                std::string dbTitle = title;
                if (auto findMappedTitle = FindDbTitle(title))
                {
                    dbTitle = findMappedTitle.value();
                }

                auto [versionIndex, musicIndex] = FindIndexes(versionName, dbTitle);

                for (auto &[dateTime, recordData] : musicData.items())
                {
                    auto playCount = static_cast<std::size_t>(recordData["play"]);

                    MusicScore musicScore
                    {
                        versionIndex,
                        musicIndex,
                        metaPlayStyle,
                        playCount,
                        dateTime
                    };

                    for (auto &[difficultyAcronym, scoreData] : recordData["score"].items())
                    {
                        ChartScore chartScore;
                        auto difficulty = static_cast<Difficulty>(ToDifficultyAcronym(difficultyAcronym));

                        //! @brief [score, pgreat, great, miss, clear, djLevel], same order as CSV.
                        std::array<std::string, 6> difficultyData = scoreData;

                        if (!difficultyData[0].empty()) chartScore.ExScore = std::stoi(difficultyData[0]);
                        if (!difficultyData[1].empty()) chartScore.PGreatCount = std::stoi(difficultyData[1]);
                        if (!difficultyData[2].empty()) chartScore.GreatCount = std::stoi(difficultyData[2]);
                        if (difficultyData[3]!="---") chartScore.MissCount = std::stoi(difficultyData[3]);
                        chartScore.ClearType = ToClearType(difficultyData[4]);
                        if (difficultyData[5]!="---") chartScore.DjLevel = ToDjLevel(difficultyData[5]);

                        musicScore.AddChartScore(difficulty, chartScore);
                    }

                    playerScore.AddMusicScore(musicScore);
                }
            }
        }
    }
    catch (const std::exception &e)
    {
        throw std::runtime_error("Core::Import(): exception:\n    "+std::string{e.what()});
    }
}

const std::map<std::string, PlayerScore> &
Core::
GetPlayerScores()
const
{
    return mPlayerScores;
}

void
Core::
CreatePlayer(const std::string &iidxId)
{
    mPlayerScores.emplace(iidxId, iidxId);
    mAllPlayerCsvs[iidxId];
    for (auto playStyle : PlayStyleSmartEnum::ToRange())
    {
        mAllPlayerCsvs[iidxId][playStyle];
    }
}

void
Core::
AddCsvToPlayerScore(const std::string &iidxId,
                    PlayStyle playStyle,
                    const std::string &dateTime)
{
    auto findPlayer = icl_s2::Find(mAllPlayerCsvs, iidxId);
    if (!findPlayer)
    {
        throw std::runtime_error("cannot find ["+iidxId+"] player.");
    }

    auto findCsv = icl_s2::Find((findPlayer.value()->second).at(playStyle), dateTime);
    if (!findCsv)
    {
        throw std::runtime_error("cannot find ["+iidxId+"] player ["+ToString(playStyle)+"]["+dateTime+"] CSV.");
    }

    auto &csv = *(findCsv.value()->second);
    auto &playerScore = mPlayerScores.at(iidxId);
    for (auto &[musicId, musicScore] : csv.GetScores())
    {
        (void)musicId;
        playerScore.AddMusicScore(musicScore);
    }
}

const Json*
Core::
FindDbMusic(std::size_t versionIndex, const std::string &title)
const
{
    auto versionJsonKey = ToDbVersionKey(versionIndex);
    auto findMusic = icl_s2::Find(mMusicDatabase["musicTable"][versionJsonKey], title);
    if (findMusic)
    {
        return &(findMusic.value().value());
    }

    auto findCsMusic = icl_s2::Find(mMusicDatabase["csMusicTable"][versionJsonKey], title);
    if (findCsMusic)
    {
        return &(findCsMusic.value().value());
    }

    return nullptr;
}

std::string
ToDbVersionKey(std::size_t versionIndex)
{
    return fmt::format("{:02}", versionIndex);
}

}
