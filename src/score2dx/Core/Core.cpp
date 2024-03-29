#include "score2dx/Core/Core.hpp"

#include <ctime>
#include <filesystem>
#include <fstream>

#include "curl/curl.h"

#include "fmt/chrono.h"

#include "ies/Common/IntegralRangeUsing.hpp"
#include "ies/StdUtil/Find.hxx"
#include "ies/Time/ScopeTimePrinter.hxx"
#include "ies/Time/TimeUtilFormat.hxx"

#include "score2dx/Iidx/Version.hpp"

namespace fs = std::filesystem;

namespace score2dx
{

Core::
Core()
:   mAnalyzer(mMusicDatabase)
{
}

const MusicDatabase &
Core::
GetMusicDatabase()
const
{
    return mMusicDatabase;
}

void
Core::
AddPlayer(const std::string &iidxId)
{
    if (!IsIidxId(iidxId))
    {
        return;
    }

    if (!ies::Find(mPlayerScores, iidxId))
    {
        CreatePlayer(iidxId);
    }
}

bool
Core::
LoadDirectory(std::string_view directory,
              bool verbose,
              bool checkWithDatabase)
{
    fmt::print("Core::LoadDirectory(): load from [{}]\n", directory);
    if (!fs::exists(directory)||!fs::is_directory(directory))
    {
        if (verbose)
        {
            std::cout << "directory is not a valid directory.\n";
        }
        return false;
    }

    auto path = fs::canonical(directory).lexically_normal();
    auto iidxId = path.filename().string().substr(0, 9);
    if (!IsIidxId(iidxId))
    {
        if (verbose)
        {
            std::cout << "directory is not a valid IIDX ID.\n";
        }
        return false;
    }

    if (!ies::Find(mPlayerScores, iidxId))
    {
        CreatePlayer(iidxId);
    }

    auto &playerScore = mPlayerScores.at(iidxId);

    for (auto &entry : fs::directory_iterator{directory})
    {
        ies::Time::ScopeTimePrinter<std::chrono::milliseconds> timePrinter{"LoadDirectory: file"};

        if (entry.is_regular_file()&&entry.path().extension()==".csv")
        {
            if (verbose) { std::cout << "\n"; }
            auto filename = entry.path().filename().string();

            auto [isValid, invalidReason] = IsValidCsvFilename(filename);
            if (!isValid)
            {
                if (verbose) { std::cout << "Skip CSV ["+filename+"] with invalid filename: "+invalidReason+".\n"; }
                continue;
            }

            if (!filename.starts_with(iidxId))
            {
                if (verbose) { std::cout << "Skip CSV ["+filename+"] with unmatch IIDX ID.\n"; }
                continue;
            }

            if (verbose) { std::cout << "Load CSV [" << filename << "]\n"; }
            std::unique_ptr<Csv> csvPtr;
            try
            {
                csvPtr = std::make_unique<Csv>(entry.path().string(), mMusicDatabase, verbose, checkWithDatabase);
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
            auto &allTimeCsvs = mPlayerCsvs.at(iidxId).at(playStyle);
            allTimeCsvs[dateTime] = std::move(csvPtr);

            AddCsvToPlayerScore(iidxId, playStyle, dateTime);
        }

        if (entry.is_regular_file()&&entry.path().extension()==".json")
        {
            auto filename = entry.path().filename().string();
            if (filename.starts_with("score2dx_export_"))
            {
                Import(iidxId, entry.path().string(), verbose);
            }
        }
    }

    {
        ies::Time::ScopeTimePrinter<std::chrono::milliseconds> timePrinter{"Propagate"};
        playerScore.Propagate();
    }

    if (verbose)
    {
        auto &playerCsvs = mPlayerCsvs.at(iidxId);
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

    Analyze(iidxId, playerScore);

    return true;
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

        auto findPlayerScore = ies::Find(mPlayerScores, iidxId);
        if (!findPlayerScore)
        {
            return;
        }

        auto &playerScore = findPlayerScore.value()->second;
        Export(playerScore, playStyle, outputDirectory, dateTimeType, suffix);
    }
    catch (const std::exception &e)
    {
        throw std::runtime_error("Core::Export(PlayStyle): exception:\n    "+std::string{e.what()});
    }
}

void
Core::
Export(const PlayerScore &playerScore,
       PlayStyle playStyle,
       const std::string &outputDirectory,
       const std::string &dateTimeType,
       const std::string &suffix)
const
{
    try
    {
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
        metadata["id"] = playerScore.GetIidxId();
        metadata["playStyle"] = ToString(playStyle);
        metadata["dateTimeType"] = dateTimeType;
        metadata["scoreVersion"] = "";
        //'' todo: decide score version if all score in a version.

        std::string lastDateTime;

        auto &data = exportData["data"];

        for (auto &[musicId, versionScoreTable] : playerScore.GetVersionScoreTables())
        {
            auto &title = mMusicDatabase.GetTitle(musicId);
            auto versionIndex = ToIndexes(musicId).first;
            auto versionName = VersionNames[versionIndex];
            if (versionIndex==0||versionIndex==1)
            {
                versionName = Official1stSubVersionName;
            }

            auto &versionData = data[versionName];
            auto &titleData = versionData[title];

            for (auto scoreVersionIndex : GetSupportScoreVersionRange())
            {
                for (auto &[dateTime, musicScore] : versionScoreTable.GetMusicScores(scoreVersionIndex, playStyle))
                {
                    if (dateTime>lastDateTime)
                    {
                        lastDateTime = dateTime;
                    }

                    auto &record = titleData[dateTime];
                    record["play"] = musicScore.GetPlayCount();
                    auto &scoreData = record["score"];

                    for (auto &[difficulty, chartScorePtr] : musicScore.GetChartScores())
                    {
                        auto diffAcronym = static_cast<DifficultyAcronym>(difficulty);
                        auto &chartScore = *chartScorePtr;

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
        ies::Time::ScopeTimePrinter<std::chrono::milliseconds> timePrinter{"Import"};

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

        const std::string iidxId = metadata["id"];
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

        if (!ies::Find(mPlayerScores, iidxId))
        {
            CreatePlayer(iidxId);
        }

        auto &playerScore = mPlayerScores.at(iidxId);

        auto &data = constExportedData["data"];
        if (data.empty())
        {
            throw std::runtime_error("data has empty entry.");
        }

        /*
        auto checkDateTime = (metadata["dateTimeType"]=="scriptUpdateDate");
        (void)checkDateTime;
        */

        for (auto &[versionName, versionData] : data.items())
        {
            for (auto &[title, musicData] : versionData.items())
            {
                if (musicData.is_null())
                {
                    continue;
                }

                std::string dbTitle = title;
                if (auto findMappedTitle = mMusicDatabase.FindDbTitle(title))
                {
                    dbTitle = findMappedTitle.value();
                }

                auto [versionIndex, musicIndex] = mMusicDatabase.FindIndexes(versionName, dbTitle);
                auto musicId = ToMusicId(versionIndex, musicIndex);

                for (auto& [dateTime, recordData] : musicData.items())
                {
                    auto playCount = static_cast<std::size_t>(recordData["play"]);

                    //'' design changed, all existing Json are assumed dumped from IIDX ME.
                    //'' until new Json format that dump ScoreSource also.
                    MusicScore musicScore
                    {
                        musicId,
                        metaPlayStyle,
                        playCount,
                        dateTime,
                        ScoreSource::Me
                    };

                    auto findScoreVersionIndex = FindVersionIndexFromDateTime(dateTime);
                    if (!findScoreVersionIndex)
                    {
                        std::cout << "Data contains date time not supported.\n"
                                  << recordData << "\n";
                        continue;
                    }

                    auto scoreVersionIndex = findScoreVersionIndex.value();
                    auto* activeVersionPtr = mMusicDatabase.FindActiveVersion(scoreVersionIndex);
                    if (!activeVersionPtr)
                    {
                        throw std::runtime_error("cannot find active versioin");
                    }
                    auto& activeVersion = *activeVersionPtr;
                    auto& availableChartDifficulties = activeVersion.GetAvailableCharts(musicId, metaPlayStyle);
                    for (auto difficulty : availableChartDifficulties)
                    {
                        musicScore.EnableChartScore(difficulty);
                    }

                    for (auto &[difficultyAcronym, scoreData] : recordData["score"].items())
                    {
                        auto difficulty = static_cast<Difficulty>(ToDifficultyAcronym(difficultyAcronym));
                        auto &chartScore = musicScore.EnableChartScore(difficulty);

                        //! @brief [score, pgreat, great, miss, clear, djLevel], same order as CSV.
                        std::array<std::string, 6> difficultyData = scoreData;

                        if (!difficultyData[0].empty()) { chartScore.ExScore = std::stoi(difficultyData[0]); }
                        if (!difficultyData[1].empty()) { chartScore.PGreatCount = std::stoi(difficultyData[1]); }
                        if (!difficultyData[2].empty()) { chartScore.GreatCount = std::stoi(difficultyData[2]); }
                        if (difficultyData[3]!="---") { chartScore.MissCount = std::stoi(difficultyData[3]); }
                        chartScore.ClearType = ToClearType(difficultyData[4]);
                        if (difficultyData[5]!="---") { chartScore.DjLevel = ToDjLevel(difficultyData[5]); }

                        auto styleDifficulty = ConvertToStyleDifficulty(metaPlayStyle, difficulty);

                        if (chartScore.MissCount && chartScore.MissCount==0 && chartScore.ClearType!=ClearType::FULLCOMBO_CLEAR)
                        {
                            if (verbose)
                            {
                                std::cout << "[" << ToVersionString(versionIndex)
                                          << "][" << dbTitle
                                          << "][" << ToString(styleDifficulty)
                                          << "][" << dateTime
                                          << "] has ClearType [" << ToString(chartScore.ClearType)
                                          << "] and MissCount has value " << chartScore.MissCount.value()
                                          << "\n";
                            }
                            chartScore.MissCount = std::nullopt;
                        }

                        auto* findChartInfo = mMusicDatabase.FindChartInfo(
                            musicId,
                            styleDifficulty,
                            scoreVersionIndex
                        );

                        if (!findChartInfo)
                        {
                            //'' exported data from script may contain difficulty not existing (yet).
                            /*
                            if (chartScore.ClearType==ClearType::NO_PLAY)
                            {
                                continue;
                            }
                            */
                            std::cout << ToVersionString(versionIndex) << " Title [" << dbTitle << "]\n"
                                      << "DateTime: " << dateTime << "\n"
                                      << "ScoreVersion: " << ToVersionString(scoreVersionIndex) << "\n"
                                      << "StyleDifficulty: " << ToString(styleDifficulty) << "\n";
                            throw std::runtime_error("cannot find chart info");
                        }

                        auto &chartInfo = *findChartInfo;
                        if (chartInfo.Note<=0)
                        {
                            std::cout << ToVersionString(versionIndex) << " Title [" << dbTitle << "]\n"
                                      << "DateTime: " << dateTime << "\n"
                                      << "ScoreVersion: " << ToVersionString(scoreVersionIndex) << "\n"
                                      << "StyleDifficulty: " << ToString(styleDifficulty) << "\n";
                            throw std::runtime_error("DB chart info note is non-positive.");
                        }

                        auto actualDjLevel = FindDjLevel(chartInfo.Note, chartScore.ExScore);
                        if (actualDjLevel!=chartScore.DjLevel)
                        {
                            if (verbose)
                            {
                                std::cout << "Warning: unmatched DJ level in import file:"
                                          << "\n[" << ToVersionString(versionIndex)
                                          << "][" << dbTitle
                                          << "][" << ToString(styleDifficulty)
                                          << "]\nLevel: " << chartInfo.Level
                                          << ", Note: " << chartInfo.Note
                                          << ", Score: " << chartScore.ExScore
                                          << ", Actual DJ Level: " << ToString(actualDjLevel)
                                          << ", Data DJ Level: " << ToString(chartScore.DjLevel)
                                          << "\nDateTime: " << dateTime
                                          << ", ScoreVersion: " << ToVersionString(scoreVersionIndex)
                                          << "."
                                          << std::endl;
                            }
                            chartScore.DjLevel = actualDjLevel;
                        }
                    }

                    playerScore.AddMusicScore(scoreVersionIndex, musicScore);
                }
            }
        }
    }
    catch (const std::exception &e)
    {
        throw std::runtime_error("Core::Import(): exception:\n    "+std::string{e.what()});
    }
}

const PlayerScore &
Core::
GetPlayerScore(const std::string &iidxId)
const
{
    auto findPlayerScore = ies::Find(mPlayerScores, iidxId);
    if (!findPlayerScore)
    {
        throw std::runtime_error("no player score for ["+iidxId+"].");
    }
    return findPlayerScore.value()->second;
}

const std::map<std::string, PlayerScore> &
Core::
GetPlayerScores()
const
{
    return mPlayerScores;
}

std::map<std::string, const Csv*>
Core::
GetCsvs(const std::string &iidxId, PlayStyle playStyle)
const
{
    std::map<std::string, const Csv*> csvs;
    for (auto &[dateTime, csv] : mPlayerCsvs.at(iidxId).at(playStyle))
    {
        csvs[dateTime] = csv.get();
    }
    return csvs;
}

void
Core::
SetActiveVersionIndex(std::size_t activeVersionIndex)
{
    mAnalyzer.SetActiveVersionIndex(activeVersionIndex);
    mPlayerAnalyses.clear();
}

std::size_t
Core::
GetActiveVersionIndex()
const
{
    return mAnalyzer.GetActiveVersionIndex();
}

void
Core::
Analyze(const std::string &iidxId)
{
    auto findPlayerScore = ies::Find(mPlayerScores, iidxId);
    if (!findPlayerScore)
    {
        throw std::runtime_error("no such player ["+iidxId+"].");
    }

    auto &playerScore = findPlayerScore.value()->second;

    Analyze(iidxId, playerScore);
}

void
Core::
Analyze(const std::string &iidxId,
        const PlayerScore &playerScore)
{
    mPlayerAnalyses.erase(iidxId);
    mPlayerAnalyses.emplace(iidxId, mAnalyzer.Analyze(playerScore));
    mPlayerVersionActivityAnalyses.erase(iidxId);
    mPlayerVersionActivityAnalyses.emplace(iidxId, mAnalyzer.AnalyzeVersionActivity(playerScore));
}

const ScoreAnalysis*
Core::
FindAnalysis(const std::string &iidxId)
const
{
    auto findAnalysis = ies::Find(mPlayerAnalyses, iidxId);
    if (!findAnalysis) { return nullptr; }

    return &(findAnalysis.value()->second);
}

const ActivityAnalysis*
Core::
FindVersionActivityAnalysis(const std::string &iidxId)
const
{
    auto findAnalysis = ies::Find(mPlayerVersionActivityAnalyses, iidxId);
    if (!findAnalysis) { return nullptr; }

    return &(findAnalysis.value()->second);
}

void
Core::
AnalyzeActivity(const std::string &iidxId,
                const std::string &beginDateTime,
                const std::string &endDateTime)
{
    auto findPlayerScore = ies::Find(mPlayerScores, iidxId);
    if (!findPlayerScore)
    {
        throw std::runtime_error("no such player ["+iidxId+"].");
    }

    auto &playerScore = findPlayerScore.value()->second;

    mPlayerActivityAnalyses.erase(iidxId);
    mPlayerActivityAnalyses.emplace(iidxId, mAnalyzer.AnalyzeActivity(playerScore, beginDateTime, endDateTime));
}

const ActivityAnalysis*
Core::
FindActivityAnalysis(const std::string &iidxId)
const
{
    auto findAnalysis = ies::Find(mPlayerActivityAnalyses, iidxId);
    if (!findAnalysis) { return nullptr; }

    return &(findAnalysis.value()->second);
}

std::string
Core::
AddIidxMeUser(const std::string &user)
{
    ies::Time::ScopeTimePrinter<std::chrono::milliseconds> timePrinter{"AddIidxMeUser"};

    if (auto findId = ies::Find(mIidxMeUserIdMap, user))
    {
        return findId.value()->second;
    }

    auto* curl = curl_easy_init();
    auto* slist = curl_slist_append(nullptr, "Iidxme-Api-Key: 295d293051a911ecbf630242ac130002");
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, slist);
    curl_easy_setopt(curl, CURLOPT_CAINFO, "ca-bundle.crt");

    auto url = "https://api.iidx.me/user/data/djdata?user="+user+"&ver=29";
    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());

    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION,
        +[](char* bufptr, std::size_t size, std::size_t nitems, void* userp)
        -> std::size_t
        {
            auto* buffer = reinterpret_cast<std::string*>(userp);
            buffer->append(bufptr, size*nitems);
            return size*nitems;
        }
    );

    std::string buffer;
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &buffer);

    auto errorCode = curl_easy_perform(curl);
    if (errorCode!=CURLE_OK)
    {
        throw std::runtime_error("CURL error: "+std::to_string(errorCode));
    }

    curl_slist_free_all(slist);
    curl_easy_cleanup(curl);

    auto json = score2dx::Json::parse(buffer);

    if (ies::Find(json, "status"))
    {
        std::cout << "Cannot find IIDXME user ["+user+"].\n";
        return {};
    }

    std::string iidxId = json.at("iidxid").at("value");
    if (iidxId.empty())
    {
        throw std::runtime_error("empty IidxMe found");
    }

    std::cout << "IIDXME user ["+user+"] IIDX ID ["+iidxId+"].\n";

    mIidxMeUserIdMap[user] = iidxId;

    return iidxId;
}

void
Core::
ExportIidxMeData(const std::string &user, std::size_t endVersionIndex)
{
    ies::Time::ScopeTimePrinter<std::chrono::seconds> timePrinter{"ExportIidxMeData"};
    auto* curl = curl_easy_init();
    auto* slist = curl_slist_append(nullptr, "Iidxme-Api-Key: 295d293051a911ecbf630242ac130002");
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, slist);
    curl_easy_setopt(curl, CURLOPT_CAINFO, "ca-bundle.crt");

    if (!ies::Find(mIidxMeUserIdMap, user))
    {
        auto iidxMeIidxId = AddIidxMeUser(user);
        if (iidxMeIidxId.empty())
        {
            throw std::runtime_error("cannot get IIDX ID of IIDX ME user ["+user+"].");
        }
    }

    auto &iidxId = mIidxMeUserIdMap.at(user);
    PlayerScore playerScore{mMusicDatabase, iidxId};

    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION,
        +[](char* bufptr, std::size_t size, std::size_t nitems, void* userp)
        -> std::size_t
        {
            auto* buffer = reinterpret_cast<std::string*>(userp);
            buffer->append(bufptr, size*nitems);
            return size*nitems;
        }
    );

    auto urlPrefix = "https://api.iidx.me/user/data/music?user="+user+"&mid=";

    for (auto versionIndex: IndexRange{0, endVersionIndex})
    {
        std::size_t noEntryCount = 0;
        for (auto iidxMeMusicIndex : IndexRange{1, 150})
        {
            auto iidxMeMusicId = score2dx::ToMusicId(versionIndex, iidxMeMusicIndex);
            auto iidxMeMusicIdString = score2dx::ToMusicIdString(iidxMeMusicId);
            auto iidxmeMid = iidxMeMusicIdString;
            if (versionIndex<10)
            {
                iidxmeMid = iidxMeMusicIdString.substr(1);
            }
            auto url =  urlPrefix+iidxmeMid;
            std::cout << "Check Music Data [" << iidxmeMid << "]" << std::endl;

            curl_easy_setopt(curl, CURLOPT_URL, url.c_str());

            std::string buffer;

            curl_easy_setopt(curl, CURLOPT_WRITEDATA, &buffer);

            auto curlBegin = ies::Time::Now();
            auto errorCode = curl_easy_perform(curl);

            if (errorCode!=CURLE_OK)
            {
                std::cout << "CURL error: " << errorCode << "\n";
                break;
            }

            auto json = score2dx::Json::parse(buffer);

            if (!ies::Find(json, "code"))
            {
                if (ies::Find(json, "metadata"))
                {
                    noEntryCount = 0;

                    auto &metadata = json.at("metadata");
                    std::string title = metadata.at("title");
                    std::size_t metaVersionIndex = metadata.at("version");
                    if (metaVersionIndex!=versionIndex)
                    {
                        throw std::runtime_error("metadata version != url version");
                    }

                    auto dbTitle = title;
                    auto findMappedTitle = mMusicDatabase.FindDbTitle(dbTitle);
                    if (findMappedTitle)
                    {
                        dbTitle = findMappedTitle.value();
                    }

                    auto findMusicId = mMusicDatabase.FindMusicId(versionIndex, dbTitle);
                    if (!findMusicId)
                    {
                        std::cout << "IIDXME [" << iidxmeMid << "][" << dbTitle << "] cannot find in music db.\n";
                        continue;
                    }

                    auto musicId = findMusicId.value();

                    for (auto playStyle : PlayStyleSmartEnum::ToRange())
                    {
                        std::map<Difficulty, std::set<std::size_t>> noUpdateDateVersionsByDifficulty;

                        auto iidxMeStyle = ToString(playStyle).substr(0, 6);
                        std::transform(
                            iidxMeStyle.begin(), iidxMeStyle.end(),
                            iidxMeStyle.begin(),
                            [](unsigned char c) { return static_cast<char>(std::tolower(c)); }
                        );

                        if (!ies::Find(json, iidxMeStyle))
                        {
                            std::cout << "IIDXME [" << iidxmeMid << "][" << title << "] lack style ["+iidxMeStyle+"]\n";
                            continue;
                        }

                        auto &difficultyData = json.at(iidxMeStyle);
                        for (auto &[chartIndex, chartData] : difficultyData.items())
                        {
                            if (!ies::Find(chartData, "diff"))
                            {
                                std::cout << "IIDXME [" << iidxmeMid << "][" << title << "]["+iidxMeStyle+"]["+chartIndex+"] chartData lack key diff.\n";
                                continue;
                            }

                            int iideMeDiff = chartData.at("diff");
                            auto difficulty = static_cast<Difficulty>(iideMeDiff-1);
                            auto styleDifficulty = ConvertToStyleDifficulty(playStyle, difficulty);

                            if (!ies::Find(chartData, "scores"))
                            {
                                std::cout << "IIDXME [" << iidxmeMid << "][" << title << "]["+iidxMeStyle+"]["+chartIndex+"]["+ToString(styleDifficulty)+"] chartData lack score.\n";
                                continue;
                            }

                            auto &noUpdateDateVersions = noUpdateDateVersionsByDifficulty[difficulty];

                            for (auto &[scoreIndex, scoreData] : chartData.at("scores").items())
                            {
                                if (!ies::Find(scoreData, "version"))
                                {
                                    std::cout << "IIDXME [" << iidxMeMusicId << "][" << title
                                              << "]["+iidxMeStyle
                                              << "]["+chartIndex
                                              << "]["+ToString(styleDifficulty)
                                              << "] score ["+scoreIndex
                                              << "] don't have version.\n";
                                    continue;
                                }

                                if (!ies::Find(scoreData, "clear")
                                    ||!ies::Find(scoreData, "rank")
                                    ||!ies::Find(scoreData, "miss")
                                    ||!ies::Find(scoreData, "score"))
                                {
                                    continue;
                                }

                                if (scoreData.at("clear")==0)
                                {
                                    continue;
                                }

                                const std::size_t scoreVersionIndex = scoreData.at("version");
                                if (scoreVersionIndex==GetLatestVersionIndex()&&scoreData.at("score").is_null())
                                {
                                    continue;
                                }

                                std::string dateTime;
                                if (!ies::Find(scoreData, "updated")||scoreData.at("updated").is_null())
                                {
                                    if (ies::Find(noUpdateDateVersions, scoreVersionIndex))
                                    {
                                        std::cout << "IIDXME [" << iidxMeMusicId << "][" << title
                                                  << "]["+iidxMeStyle
                                                  << "]["+chartIndex
                                                  << "]["+ToString(styleDifficulty)
                                                  << "] score ["+scoreIndex
                                                  << "] has repeated no date versions.\n";
                                        continue;
                                    }
                                    noUpdateDateVersions.emplace(scoreVersionIndex);

                                    if (scoreVersionIndex<GetFirstSupportDateTimeVersionIndex())
                                    {
                                        std::cout << "IIDXME [" << iidxMeMusicId << "][" << title
                                                  << "]["+iidxMeStyle
                                                  << "]["+chartIndex
                                                  << "]["+ToString(styleDifficulty)
                                                  << "] score ["+scoreIndex
                                                  << "] has no date version before 17.\n";
                                        continue;
                                    }

                                    if (scoreVersionIndex>=GetLatestVersionIndex())
                                    {
                                        std::cout << "IIDXME [" << iidxMeMusicId << "][" << title
                                                  << "]["+iidxMeStyle
                                                  << "]["+chartIndex
                                                  << "]["+ToString(styleDifficulty)
                                                  << "] score ["+scoreIndex
                                                  << "] has score but no date at latest version index.\n";
                                        continue;
                                    }

                                    dateTime = GetVersionDateTimeRange(scoreVersionIndex).Get(ies::RangeSide::End);
                                }
                                else
                                {
                                    dateTime = scoreData.at("updated");
                                    dateTime += " 00:00";
                                }

                                if (dateTime.empty())
                                {
                                    std::cout << "IIDXME [" << iidxMeMusicId << "][" << title
                                              << "]["+iidxMeStyle
                                              << "]["+chartIndex
                                              << "]["+ToString(styleDifficulty)
                                              << "] score ["+scoreIndex
                                              << "] has empty date time.\n";
                                    continue;
                                }

                                auto firstSupportVersionIndex = GetFirstSupportDateTimeVersionIndex();
                                if (scoreVersionIndex<firstSupportVersionIndex)
                                {
                                    auto &firstDateTime = GetVersionDateTimeRange(firstSupportVersionIndex).Get(ies::RangeSide::Begin);
                                    if (dateTime>=firstDateTime)
                                    {
                                        dateTime = "2009-10-20 23:59";
                                    }
                                }
                                else if (scoreVersionIndex<GetLatestVersionIndex())
                                {
                                    auto &versionEndDateTime = GetVersionDateTimeRange(scoreVersionIndex).Get(ies::RangeSide::End);
                                    if (dateTime>versionEndDateTime)
                                    {
                                        std::cout << "IIDXME [" << iidxMeMusicId << "][" << title
                                                  << "]["+iidxMeStyle
                                                  << "]["+chartIndex
                                                  << "]["+ToString(styleDifficulty)
                                                  << "] score ["+scoreIndex
                                                  << "] fix score date time from ["+dateTime
                                                  << "] to ["+versionEndDateTime
                                                  << "]\n";
                                        dateTime = versionEndDateTime;
                                    }
                                }
                                else if (scoreVersionIndex>GetLatestVersionIndex())
                                {
                                    std::cout << "IIDXME [" << iidxMeMusicId << "][" << title
                                              << "]["+iidxMeStyle
                                              << "]["+chartIndex
                                              << "]["+ToString(styleDifficulty)
                                              << "] has score ["+scoreIndex
                                              << "] but is not in supported version [" << versionIndex
                                              << "]\n";
                                    continue;
                                }

                                ChartScore chartScore;
                                if (!scoreData.at("clear").is_null())
                                {
                                    const int iClear = scoreData.at("clear");
                                    chartScore.ClearType = static_cast<ClearType>(iClear);
                                }
                                if (!scoreData.at("miss").is_null())
                                {
                                    const int miss = scoreData.at("miss");
                                    chartScore.MissCount = miss;
                                }
                                if (!scoreData.at("rank").is_null())
                                {
                                    const int iDjLevel = scoreData.at("rank");
                                    chartScore.DjLevel = static_cast<DjLevel>(iDjLevel);
                                }
                                if (!scoreData.at("score").is_null())
                                {
                                    const int score = scoreData.at("score");
                                    chartScore.ExScore = score;
                                }

                                MusicScore musicScore{musicId, playStyle, 0, dateTime, ScoreSource::Me};
                                musicScore.SetChartScore(difficulty, chartScore);
                                playerScore.AddMusicScore(scoreVersionIndex, musicScore);
                            }
                        }
                    }
                }
                else
                {
                    std::cout << "Get response but without metadata.\n";
                    break;
                }
            }
            else
            {
                ++noEntryCount;
            }

            ies::Time::Print<std::chrono::milliseconds>(ies::Time::CountNs(curlBegin), iidxMeMusicIdString+" done.");
            while (ies::Time::CountNs(curlBegin)<200'000'000)
            {
                continue;
            }

            if (noEntryCount>4)
            {
                break;
            }
        }
    }

    curl_slist_free_all(slist);
    curl_easy_cleanup(curl);

    fs::create_directory("./ME");
    fs::create_directory("./ME/"+iidxId);

    for (auto playStyle : PlayStyleSmartEnum::ToRange())
    {
        Export(playerScore, playStyle, "ME/"+iidxId+"");
    }

    std::cout << std::endl;
}

void
Core::
CheckIidxMeDataTable()
const
{
    ies::Time::ScopeTimePrinter<std::chrono::milliseconds> timePrinter{"CheckIidxMeDataTable"};
    //std::string iidxMeDataTableFilename = R"(E:\project_document\score2dx\iidxme\iidxme_datatable.json)";
    const std::string iidxMeDataTableFilename = R"(E:\project_document\score2dx\iidxme\iidxme_datatable_delmitz.json)";
    std::cout << "iidxMeDataTableFilename = " << iidxMeDataTableFilename << "\n";
    std::ifstream tableFile{iidxMeDataTableFilename};
    Json iidxMeTable;
    tableFile >> iidxMeTable;

    for (auto &[iidxMeMusicId, musicData] : iidxMeTable.items())
    {
        auto &metadata = musicData.at("metadata");
        const std::string title = metadata.at("title");
        const std::size_t versionIndex = metadata.at("version");

        std::string dbTitle = title;
        auto findMappedTitle = mMusicDatabase.FindDbTitle(dbTitle);
        if (findMappedTitle)
        {
            /*
            auto findSection = mMusicDatabase.FindDbTitleMappingSection(dbTitle);
            if (!findSection) { throw std::runtime_error("cannot find mapping section."); }
            auto &section = findSection.value();
            */

            dbTitle = findMappedTitle.value();
            /*
            std::cout << "\nFound ["+section+"] title mapping:\n"
                      << "IIDXME [" << iidxMeMusicId << "][" << title << "]\n"
                      << "DB [" << dbTitle << "]\n";
            */
        }

        auto findMusicId = mMusicDatabase.FindMusicId(versionIndex, dbTitle);
        if (!findMusicId)
        {
            std::cout << "IIDXME [" << iidxMeMusicId << "][" << dbTitle << "] cannot find in music db.\n";
            continue;
        }

        //auto musicInfo = mMusicDatabase.GetLatestMusicInfo(context.MusicId);
        /*
        if (findMappedTitle && !musicInfo.GetField(MusicInfoField::DisplayTitle).empty())
        {
            std::cout << "DB display title [" << musicInfo.GetField(MusicInfoField::DisplayTitle) << "]\n";
        }
        */

        /*
        auto &dbArtist = musicInfo.GetField(MusicInfoField::Artist);
        auto &meArtist = metadata.at("artist");
        if (dbArtist!=meArtist)
        {
            std::cout << "IIDXME [" << iidxMeMusicId << "][" << dbTitle << "] artist mismatch.\n"
                      << "DB [" << dbArtist << "]\n"
                      << "ME [" << meArtist << "]\n";
        }

        auto &dbGenre = musicInfo.GetField(MusicInfoField::Genre);
        auto &meGenre = metadata.at("genre");
        if (dbGenre!=meGenre)
        {
            std::cout << "IIDXME [" << iidxMeMusicId << "][" << dbTitle << "] genre mismatch.\n"
                      << "DB [" << dbGenre << "]\n"
                      << "ME [" << meGenre << "]\n";
        }
        */

        for (auto playStyle : PlayStyleSmartEnum::ToRange())
        {
            auto iidxMeStyle = ToString(playStyle).substr(0, 6);
            std::transform(
                iidxMeStyle.begin(), iidxMeStyle.end(),
                iidxMeStyle.begin(),
                [](unsigned char c) { return static_cast<char>(std::tolower(c)); }
            );

            if (!ies::Find(musicData, iidxMeStyle))
            {
                std::cout << "IIDXME [" << iidxMeMusicId << "][" << title << "] lack style ["+iidxMeStyle+"]\n";
                continue;
            }

            std::map<Difficulty, std::set<std::size_t>> noUpdateDateVersionsByDifficulty;

            auto &difficultyData = musicData.at(iidxMeStyle);
            for (auto &[chartIndex, chartData] : difficultyData.items())
            {
                if (!ies::Find(chartData, "diff"))
                {
                    std::cout << "IIDXME [" << iidxMeMusicId << "][" << title << "]["+iidxMeStyle+"]["+chartIndex+"] chartData lack key diff.\n";
                }

                const int iideMeDiff = chartData.at("diff");
                auto difficulty = static_cast<Difficulty>(iideMeDiff-1);
                auto styleDifficulty = ConvertToStyleDifficulty(playStyle, difficulty);
                auto &noUpdateDateVersions = noUpdateDateVersionsByDifficulty[difficulty];

                auto iidxMeNote = chartData.at("notes");

                if (!ies::Find(chartData, "scores"))
                {
                    std::cout << "IIDXME [" << iidxMeMusicId << "][" << title << "]["+iidxMeStyle+"]["+chartIndex+"]["+ToString(styleDifficulty)+"] chartData lack score.\n";
                }

                for (auto &[scoreIndex, scoreData] : chartData.at("scores").items())
                {
                    if (!ies::Find(scoreData, "version"))
                    {
                        std::cout << "IIDXME [" << iidxMeMusicId << "][" << title
                                  << "]["+iidxMeStyle
                                  << "]["+chartIndex
                                  << "]["+ToString(styleDifficulty)
                                  << "] score ["+scoreIndex
                                  << "] don't have version.\n";
                        continue;
                    }

                    if (!ies::Find(scoreData, "clear")
                        ||!ies::Find(scoreData, "rank")
                        ||!ies::Find(scoreData, "miss")
                        ||!ies::Find(scoreData, "score"))
                    {
                        continue;
                    }

                    if (scoreData.at("clear")==0)
                    {
                        continue;
                    }

                    const std::size_t scoreVersionIndex = scoreData.at("version");
                    if (scoreVersionIndex==GetLatestVersionIndex()&&scoreData.at("score").is_null())
                    {
                        continue;
                    }

                    //'' check with DB:
                    /*
                    auto findChartInfo = mMusicDatabase.FindChartInfo(versionIndex, dbTitle, styleDifficulty, scoreVersionIndex);
                    if (!findChartInfo)
                    {
                        std::cout << "IIDXME [" << iidxMeMusicId << "][" << title
                                  << "]["+iidxMeStyle
                                  << "]["+chartIndex
                                  << "]["+ToString(styleDifficulty)
                                  << "] score ["+scoreIndex
                                  << "] cannot find chart info at ver ["+ToVersionString(scoreVersionIndex)
                                  << "].\n";
                        continue;
                    }

                    auto &chartInfo = findChartInfo.value();
                    if (chartInfo.Note!=iidxMeNote)
                    {
                        std::cout << "IIDXME [" << iidxMeMusicId << "][" << title << "]["+iidxMeStyle+"]["+chartIndex+"]["+ToString(styleDifficulty)+"] chartData note mismatch.\n"
                                  << "IIDXME note [" << iidxMeNote << "]\n"
                                  << "DB note [" << chartInfo.Note << "]\n"
                                  << "at Ver [" << ToVersionString(scoreVersionIndex) << "]\n";
                    }

                    if (!ies::Find(scoreData, "level"))
                    {
                        std::cout << "IIDXME [" << iidxMeMusicId << "][" << title
                                  << "]["+iidxMeStyle
                                  << "]["+chartIndex
                                  << "]["+ToString(styleDifficulty)
                                  << "] score ["+scoreIndex
                                  << "] don't have level.\n";
                        continue;
                    }

                    if (scoreData.at("level").is_null())
                    {
                        std::cout << "IIDXME [" << iidxMeMusicId << "][" << title
                                  << "]["+iidxMeStyle
                                  << "]["+chartIndex
                                  << "]["+ToString(styleDifficulty)
                                  << "] score ["+scoreIndex
                                  << "] level is null.\n";
                        continue;
                    }

                    int iidxMeLevel = scoreData.at("level");
                    if (iidxMeLevel!=chartInfo.Level)
                    {
                        std::cout << "IIDXME [" << iidxMeMusicId << "][" << title
                                  << "]["+iidxMeStyle+"]["+chartIndex+"]["+ToString(styleDifficulty)
                                  << "] chartData level mismatch.\n"
                                  << "IIDXME level [" << iidxMeLevel << "]\n"
                                  << "DB level [" << chartInfo.Level << "]\n"
                                  << "at Ver [" << ToVersionString(scoreVersionIndex) << "]\n";
                    }

                    */

                    std::string dateTime;
                    if (!ies::Find(scoreData, "updated")||scoreData.at("updated").is_null())
                    {
                        if (ies::Find(noUpdateDateVersions, scoreVersionIndex))
                        {
                            std::cout << "IIDXME [" << iidxMeMusicId << "][" << title
                                      << "]["+iidxMeStyle
                                      << "]["+chartIndex
                                      << "]["+ToString(styleDifficulty)
                                      << "] score ["+scoreIndex
                                      << "] has repeated no date versions.\n";
                            continue;
                        }
                        noUpdateDateVersions.emplace(scoreVersionIndex);

                        if (scoreVersionIndex<GetFirstSupportDateTimeVersionIndex())
                        {
                            std::cout << "IIDXME [" << iidxMeMusicId << "][" << title
                                      << "]["+iidxMeStyle
                                      << "]["+chartIndex
                                      << "]["+ToString(styleDifficulty)
                                      << "] score ["+scoreIndex
                                      << "] has no date version before 17.\n";
                            continue;
                        }

                        if (scoreVersionIndex==GetLatestVersionIndex())
                        {
                            std::cout << "IIDXME [" << iidxMeMusicId << "][" << title
                                      << "]["+iidxMeStyle
                                      << "]["+chartIndex
                                      << "]["+ToString(styleDifficulty)
                                      << "] score ["+scoreIndex
                                      << "] has score but no date at latest version index.\n";
                            continue;
                        }

                        dateTime = GetVersionDateTimeRange(scoreVersionIndex).Get(ies::RangeSide::End);
                    }
                    else
                    {
                        dateTime = scoreData.at("updated");
                        dateTime += " 00:00";
                    }

                    if (dateTime.empty())
                    {
                        std::cout << "IIDXME [" << iidxMeMusicId << "][" << title
                                  << "]["+iidxMeStyle
                                  << "]["+chartIndex
                                  << "]["+ToString(styleDifficulty)
                                  << "] score ["+scoreIndex
                                  << "] has empty date time.\n";
                        continue;
                    }

                    ChartScore chartScore;
                    if (!scoreData.at("clear").is_null())
                    {
                        const int iClear = scoreData.at("clear");
                        chartScore.ClearType = static_cast<ClearType>(iClear);
                    }
                    if (!scoreData.at("miss").is_null())
                    {
                        const int miss = scoreData.at("miss");
                        chartScore.MissCount = miss;
                    }
                    if (!scoreData.at("rank").is_null())
                    {
                        const int iDjLevel = scoreData.at("rank");
                        chartScore.DjLevel = static_cast<DjLevel>(iDjLevel);
                    }
                    if (!scoreData.at("score").is_null())
                    {
                        const int score = scoreData.at("score");
                        chartScore.ExScore = score;
                    }

                    /*
                    std::cout << "IIDXME [" << iidxMeMusicId << "][" << title
                              << "]["+iidxMeStyle
                              << "]["+ToString(styleDifficulty)
                              << "]\n" << ToString(chartScore) << "\n";
                    */
                }
            }
        }
    }
}

void
Core::
CreatePlayer(const std::string &iidxId)
{
    mPlayerScores.emplace
    (
        std::piecewise_construct,
        std::forward_as_tuple(iidxId),
        std::forward_as_tuple(mMusicDatabase, iidxId)
    );

    mPlayerCsvs[iidxId];
    for (auto playStyle : PlayStyleSmartEnum::ToRange())
    {
        mPlayerCsvs[iidxId][playStyle];
    }
}

void
Core::
AddCsvToPlayerScore(const std::string &iidxId,
                    PlayStyle playStyle,
                    const std::string &dateTime)
{
    ies::Time::ScopeTimePrinter<std::chrono::milliseconds> timePrinter{"AddCsvToPlayerScore"};

    auto findPlayer = ies::Find(mPlayerCsvs, iidxId);
    if (!findPlayer)
    {
        throw std::runtime_error("cannot find ["+iidxId+"] player.");
    }

    auto findCsv = ies::Find((findPlayer.value()->second).at(playStyle), dateTime);
    if (!findCsv)
    {
        throw std::runtime_error("cannot find ["+iidxId+"] player ["+ToString(playStyle)+"]["+dateTime+"] CSV.");
    }

    auto &csv = *(findCsv.value()->second);
    auto &playerScore = mPlayerScores.at(iidxId);
    for (auto &[musicId, musicScore] : csv.GetScores())
    {
        (void)musicId;
        playerScore.AddMusicScore(csv.GetVersionIndex(), musicScore);
    }
}

}
