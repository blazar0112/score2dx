#include "score2dx/Csv/Csv.hpp"

#include <algorithm>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <map>
#include <set>
#include <string_view>

#include "ies/Common/IntegralRangeUsing.hpp"
#include "ies/Common/SmartEnum.hxx"
#include "ies/StdUtil/Find.hxx"
#include "ies/StdUtil/MapApply.hxx"
#include "ies/String/SplitString.hpp"
#include "ies/Time/TimeUtilFormat.hxx"

#include "nlohmann/json.hpp"

#include "score2dx/Csv/CsvColumn.hpp"
#include "score2dx/Iidx/Definition.hpp"
#include "score2dx/Iidx/Version.hpp"
#include "score2dx/Score/MusicScore.hpp"
#include "score2dx/Score/ScoreLevel.hpp"

namespace fs = std::filesystem;
namespace s2Time = ies::Time;

namespace score2dx
{

std::map<std::size_t, std::string>
GetColumnHeaders()
{
    std::map<std::size_t, std::string> headers;
    for (auto musicColumn : CsvMusicColumnSmartEnum::ToRange())
    {
        headers[static_cast<std::size_t>(musicColumn)] = ToString(musicColumn);
    }
    for (auto difficulty : DifficultySmartEnum::ToRange())
    {
        for (auto scoreColumn : CsvScoreColumnSmartEnum::ToRange())
        {
            headers[ToCsvColumnIndex(difficulty, scoreColumn)] = ToString(difficulty)+ToString(scoreColumn);
        }
    }
    headers[DateTimeColumnIndex] = "DateTime";

    return headers;
}

std::size_t
SplitCsvLine(const std::string &separators,
             const std::string &input,
             std::array<std::string, CsvColumnSize> &columns)
{
    std::size_t start = 0;
    std::size_t end = 0;
    std::size_t index = 0;
    while ((start = input.find_first_not_of(separators, end))!=std::string::npos)
    {
        end = input.find_first_of(separators, start+1);
        if (end==std::string::npos)
        {
            end = input.length();
        }

        if (index<CsvColumnSize)
        {
            columns[index] = input.substr(start, end-start);
        }
        ++index;
    }

    return index;
}

Csv::
Csv(const std::string &csvPath,
    const MusicDatabase &musicDatabase,
    bool verbose,
    bool checkWithDatabase)
{
    //'' default name [IIDX ID]_[sp|dp]_score.csv
    //'' e.g. 5483-7391_dp_score.csv
    //''      012345678901234567
    //'' minimum stem size: 18
    //''
    //'' user may add date to distinguish different csv saved.
    //'' e.g. 5483-7391_dp_score_2020-11-21.csv
    //'' requirement:
    //''  1. must in form of [IIDX_ID]_[sp|dp]_score(_[Date]).csv
    //''  2. only date part is optionally and may allow some flexibility.

    try
    {
        auto begin = s2Time::Now();
        auto path = fs::canonical(csvPath).lexically_normal();
        mPath = path.string();
        if (verbose) std::cout << "Reading CSV file: [" << mPath << "].\n";

        mFilename = path.filename().string();
        auto [isValid, invalidReason] = IsValidCsvFilename(mFilename);
        if (!isValid)
        {
            throw std::runtime_error("CSV invalid filename, reason: "+invalidReason+".");
        }

        mIidxId = mFilename.substr(0, 9);

        if (verbose) std::cout << "IIDX ID [" << mIidxId << "].\n";

        if (mFilename[10]=='d')
        {
            mPlayStyle = PlayStyle::DoublePlay;
        }

        if (verbose) std::cout << "PlayStyle [" << ToString(mPlayStyle) << "].\n";

        //'' Map of {VersionIndex, MusicCount}.
        std::map<std::size_t, int> versionMusicCounts;
        std::ifstream csvFile{csvPath};
        std::string line;
        int lineCount = 0;

        std::size_t lastVersionIndex = 0;
        std::map<std::string, int> debugCounts;
        std::string minDateTime;
        std::string maxDateTime;

        std::array<std::string, CsvColumnSize> columns;

        std::map<std::string, s2Time::NsCountType> profNsCounts;

        while (std::getline(csvFile, line))
        {
            if (line.empty())
            {
                continue;
            }

            ++lineCount;
            if (lineCount==1)
            {
                continue;
            }

            auto beginSplitString = s2Time::Now();
            auto columnCount = SplitCsvLine(",", line, columns);
            if (columnCount!=CsvColumnSize)
            {
                throw std::runtime_error("incorrect columnn size.");
            }
            ies::MapAddCount(profNsCounts, "SplitString", s2Time::CountNs(beginSplitString));

            auto &versionName = columns[static_cast<std::size_t>(CsvMusicColumn::Version)];
            auto &csvTitle = columns[static_cast<std::size_t>(CsvMusicColumn::Title)];

            auto dbTitle = csvTitle;
            auto beginFindDbTitle = s2Time::Now();
            if (auto findMappedTitle = musicDatabase.FindCsvDbTitle(csvTitle))
            {
                dbTitle = findMappedTitle.value();
            }
            ies::MapAddCount(profNsCounts, "FindDbTitle", s2Time::CountNs(beginFindDbTitle));

            auto beginFindVerIndex = s2Time::Now();
            std::optional<std::size_t> findVersionIndex;
            if (versionName==Official1stSubVersionName)
            {
                findVersionIndex = musicDatabase.Find1stSubVersionIndex(dbTitle);
            }
            else
            {
                findVersionIndex = FindVersionIndex(versionName);
            }

            if (!findVersionIndex)
            {
                throw std::runtime_error("cannot find "+versionName+" title "+dbTitle+" version index.");
            }

            auto versionIndex = findVersionIndex.value();
            ies::MapAddCount(profNsCounts, "FindVerIndex", s2Time::CountNs(beginFindVerIndex));

            auto beginFindMusicIndex = s2Time::Now();
            auto findMusicIndex = musicDatabase.FindMusicIndex(versionIndex, dbTitle);
            if (!findMusicIndex)
            {
                if (versionIndex==GetLatestVersionIndex())
                {
                    std::cout << "Possible ["+ToVersionString(versionIndex)+"] new music title ["+csvTitle+"] not in database, skipped.\n";
                    continue;
                } throw std::runtime_error("music title ["+dbTitle+"] is not listed in it's version ["+ToVersionString(versionIndex)+"] in database");
            }

            auto musicIndex = findMusicIndex.value();
            ies::MapAddCount(profNsCounts, "FindMusicIndex", s2Time::CountNs(beginFindMusicIndex));

            /*
            if (checkWithDatabase)
            {
                auto musicId = ToMusicId(versionIndex, musicIndex);
                auto musicInfo = musicDatabase.GetLatestMusicInfo(musicId);

                auto &dbArtist = musicInfo.GetField(MusicInfoField::Artist);
                auto &csvArtist = columns[static_cast<std::size_t>(CsvMusicColumn::Artist)];
                if (csvArtist!=dbArtist)
                {
                    std::cout << "[" << dbTitle << "] artist mismatch:\n"
                              << "CSV [" << csvArtist << "]\n"
                              << "DB [" << dbArtist << "]\n";
                }

                auto &dbGenre = musicInfo.GetField(MusicInfoField::Genre);
                auto &csvGenre = columns[static_cast<std::size_t>(CsvMusicColumn::Genre)];
                if (csvGenre!=dbGenre)
                {
                    std::cout << "[" << dbTitle << "] genre mismatch:\n"
                              << "CSV [" << csvGenre << "]\n"
                              << "DB [" << dbGenre << "]\n";
                }
            }
            */


            auto beginCount = s2Time::Now();
            lastVersionIndex = versionIndex;
            ies::MapIncrementCount(versionMusicCounts, versionIndex);

            auto playCount = std::stoull(columns[static_cast<std::size_t>(CsvMusicColumn::PlayCount)]);
            mTotalPlayCount += playCount;

            auto &dateTime = columns[DateTimeColumnIndex];
            if (maxDateTime.empty() || dateTime>maxDateTime)
            {
                maxDateTime = dateTime;
            }
            if (minDateTime.empty() || dateTime<minDateTime)
            {
                minDateTime = dateTime;
            }
            ies::MapAddCount(profNsCounts, "Count", s2Time::CountNs(beginCount));

            auto beginCheckDateTime = s2Time::Now();
            auto findActiveVersionIndex = FindVersionIndexFromDateTime(dateTime);
            if (!findActiveVersionIndex)
            {
                std::cout << ToVersionString(versionIndex) << " Title [" << dbTitle << "]\n"
                          << "DateTime: " << dateTime << " is not supported.\n";
                continue;
            }
            auto activeVersionIndex = findActiveVersionIndex.value();
            ies::MapAddCount(profNsCounts, "CheckDateTime", s2Time::CountNs(beginCheckDateTime));

            auto beginOther = s2Time::Now();
            auto musicId = ToMusicId(versionIndex, musicIndex);
            auto itPair = mMusicScores.emplace(
                std::piecewise_construct,
                std::forward_as_tuple(musicId),
                std::forward_as_tuple(ToMusicId(versionIndex, musicIndex), mPlayStyle, playCount, dateTime)
            );
            auto &musicScore = itPair.first->second;

            ies::MapAddCount(profNsCounts, "Other", s2Time::CountNs(beginOther));

            auto beginDifficulty = s2Time::Now();
            for (auto difficulty : DifficultySmartEnum::ToRange())
            {
                auto level = columns[ToCsvColumnIndex(difficulty, CsvScoreColumn::Level)];
                if (level=="0")
                {
                    continue;
                }

                if (checkWithDatabase)
                {
                    auto styleDifficulty = ConvertToStyleDifficulty(mPlayStyle, difficulty);
                    auto findChartInfo = musicDatabase.FindChartInfo(versionIndex, dbTitle, styleDifficulty, activeVersionIndex);
                    if (!findChartInfo)
                    {
                        std::cout << "[" << dbTitle << "] Cannot find chart info.\n";
                    }
                    else
                    {
                        auto &chartInfo = findChartInfo.value();
                        auto csvLevel = std::stoi(level);
                        if (csvLevel!=chartInfo.Level)
                        {
                            std::cout << "[" << dbTitle << "] level mismatch:\n"
                                      << "CSV [" << csvLevel << "]\n"
                                      << "DB [" << chartInfo.Level << "]\n";
                        }
                    }
                }

                auto &chartScore = musicScore.EnableChartScore(difficulty);
                chartScore.ExScore = std::stoi(columns[ToCsvColumnIndex(difficulty, CsvScoreColumn::ExScore)]);
                chartScore.PGreatCount = std::stoi(columns[ToCsvColumnIndex(difficulty, CsvScoreColumn::PGreatCount)]);
                chartScore.GreatCount = std::stoi(columns[ToCsvColumnIndex(difficulty, CsvScoreColumn::GreatCount)]);

                auto &missCount = columns[ToCsvColumnIndex(difficulty, CsvScoreColumn::MissCount)];
                if (missCount!="---")
                {
                    chartScore.MissCount = {std::stoi(missCount)};
                }
                //'' HARD failed will have ex score but no miss count.

                auto clearType = columns[ToCsvColumnIndex(difficulty, CsvScoreColumn::ClearType)];
                chartScore.ClearType = ConvertToClearType(clearType);

                auto &djLevel = columns[ToCsvColumnIndex(difficulty, CsvScoreColumn::DjLevel)];
                if (djLevel!="---")
                {
                    chartScore.DjLevel = ToDjLevel(djLevel);

                    if (checkWithDatabase)
                    {
                        auto styleDifficulty = ConvertToStyleDifficulty(mPlayStyle, difficulty);
                        auto findChartInfo = musicDatabase.FindChartInfo(
                            versionIndex,
                            dbTitle,
                            styleDifficulty,
                            activeVersionIndex
                        );

                        if (!findChartInfo)
                        {
                            std::cout << ToVersionString(versionIndex) << " Title [" << dbTitle << "]\n"
                                      << "DateTime: " << dateTime << "\n"
                                      << "ActiveVersion: " << ToVersionString(activeVersionIndex) << "\n"
                                      << "StyleDifficulty: " << ToString(styleDifficulty) << "\n";
                            throw std::runtime_error("cannot find chart info");
                        }

                        auto &chartInfo = findChartInfo.value();
                        if (chartInfo.Note<=0)
                        {
                            std::cout << ToVersionString(versionIndex) << " Title [" << dbTitle << "]\n"
                                      << "DateTime: " << dateTime << "\n"
                                      << "ActiveVersion: " << ToVersionString(activeVersionIndex) << "\n"
                                      << "StyleDifficulty: " << ToString(styleDifficulty) << "\n";
                            throw std::runtime_error("DB chart info note is non-positive.");
                        }

                        auto actualDjLevel = FindDjLevel(chartInfo.Note, chartScore.ExScore);
                        if (actualDjLevel!=chartScore.DjLevel)
                        {
                            std::cout << "Error: unmatched DJ level in CSV:"
                                      << "\n[" << ToVersionString(versionIndex)
                                      << "][" << dbTitle
                                      << "][" << ToString(styleDifficulty)
                                      << "]\nLevel: " << chartInfo.Level
                                      << ", Note: " << chartInfo.Note
                                      << ", Score: " << chartScore.ExScore
                                      << ", Actual DJ Level: " << ToString(actualDjLevel)
                                      << ", Data DJ Level: " << ToString(chartScore.DjLevel)
                                      << "\nDateTime: " << dateTime
                                      << ", ActiveVersion: " << ToVersionString(activeVersionIndex)
                                      << ".\n";
                            chartScore.DjLevel = actualDjLevel;
                        }
                    }
                }
            }

            ies::MapAddCount(profNsCounts, "Difficulty", s2Time::CountNs(beginDifficulty));
        }

        mVersion = VersionNames.at(lastVersionIndex);

        std::size_t totalMusicCount = 0;
        if (verbose)
        {
            std::cout << "CSV Version [" << mVersion << "]\n"
                      << "Each version music count:\n";
            for (auto &[versionIndex, count] : versionMusicCounts)
            {
                //std::cout << "[" << VersionNames[versionIndex] << "] " << count << " musics.\n";
                if (versionIndex%5==0)
                {
                    if (versionIndex!=0)
                    {
                        std::cout << "\n";
                    }
                }
                else
                {
                    std::cout << ", ";
                }
                std::cout << count;
                totalMusicCount += count;
            }
            std::cout << "\n"
                      << "TotalMusicCount [" << totalMusicCount << "].\n";
        }

        for (auto &[versionIndex, count] : versionMusicCounts)
        {
            (void)versionIndex;
            mMusicCount += count;
        }

        if (maxDateTime.empty())
        {
            throw std::runtime_error("empty date times in csv.");
        }

        mLastDateTime = maxDateTime;

        if (verbose) std::cout << "DateTime [" << minDateTime << ", " << maxDateTime << "].\n";

        s2Time::Print<std::chrono::milliseconds>(s2Time::CountNs(begin), "CSV construct");

//        for (auto &[timeName, nsCount] : profNsCounts)
//        {
//            s2Time::Print<std::chrono::milliseconds>(nsCount, "    "+timeName);
//        }
    }
    catch (const std::exception &e)
    {
        throw std::runtime_error("Csv::Csv(): exception:\n    "+std::string{e.what()});
    }
}

const std::string &
Csv::
GetFilename()
const
{
    return mFilename;
}

PlayStyle
Csv::
GetPlayStyle()
const
{
    return mPlayStyle;
}

const std::string &
Csv::
GetVersion()
const
{
    return mVersion;
}

const std::string &
Csv::
GetLastDateTime()
const
{
    return mLastDateTime;
}

std::size_t
Csv::
GetTotalPlayCount()
const
{
    return mTotalPlayCount;
}

const std::map<std::size_t, MusicScore> &
Csv::
GetScores()
const
{
    return mMusicScores;
}

void
Csv::
PrintSummary()
const
{
    std::cout << "CSV ["+mFilename+"] summary:\n"
              << "    Path ["+mPath+"]\n"
              << "    IIDX ID ["+mIidxId+"]\n"
              << "    PlayStyle ["+ToString(mPlayStyle)+"]\n"
              << "    Last DateTime ["+mLastDateTime+"]\n"
              << "    MusicCount [" << mMusicCount << "]\n"
              << "    Total PlayCount [" << mTotalPlayCount << "]\n";
}

std::pair<bool, std::string>
IsValidCsvFilename(std::string_view filename)
{
    if (filename.size()<MinCsvFilenameSize)
    {
        return {false, "invalid filename length"};
    }

    if (!filename.ends_with(".csv"))
    {
        return {false, "filename is not .csv extension"};
    }

    static const std::string signature = "p_score";
    auto detectSignature = filename.substr(11, signature.size());
    if (detectSignature!=signature)
    {
        return {false, "invalid filename format"};
    }

    auto iidxId = filename.substr(0, 9);
    if (!IsIidxId(iidxId))
    {
        return {false, "filename not begins with IIDX ID"};
    }

    if (filename[10]!='s'&&filename[10]!='d')
    {
        return {false, "filename not contains sp or dp"};
    }

    return {true, ""};
}

}
