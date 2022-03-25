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

#include "score2dx/Iidx/Definition.hpp"
#include "score2dx/Iidx/Version.hpp"
#include "score2dx/Score/MusicScore.hpp"
#include "score2dx/Score/ScoreLevel.hpp"

namespace fs = std::filesystem;
namespace s2Time = ies::Time;

namespace score2dx
{

//'' columns:
//'' english_header_start_part = ['version', 'title', 'genre', 'artist', 'play count']
//'' english_header_end_part = ['last play date time']
//'' difficulty_list = ['beginner', 'normal', 'hyper', 'another', 'leggendaria']
//'' detail_list = ['level', 'score', 'pgreat', 'great', 'miss count', 'clear type', 'dj level']
//'' english_header = [difficulty+' '+detail for difficulty in difficulty_list for detail in detail_list]
//'' english_header = english_header_start_part+english_header+english_header_end_part

//! @brief MusicColumn = ColumnIndex
IES_SMART_ENUM(CsvMusicColumn,
    Version,
    Title,
    Genre,
    Artist,
    PlayCount
);

//! @brief Each Difficulty have ScoreColumns.
IES_SMART_ENUM(CsvScoreColumn,
    Level,
    ExScore,
    PGreatCount,
    GreatCount,
    MissCount,
    ClearType,
    DjLevel
);

std::size_t
ToColumnIndex(Difficulty difficulty, CsvScoreColumn scoreColumn)
{
    return CsvMusicColumnSmartEnum::Size()
           +static_cast<std::size_t>(difficulty)*CsvScoreColumnSmartEnum::Size()
           +static_cast<std::size_t>(scoreColumn);
}

constexpr std::size_t DateTimeColumnIndex = CsvMusicColumnSmartEnum::Size()+DifficultySmartEnum::Size()*CsvScoreColumnSmartEnum::Size();
constexpr std::size_t CsvColumnSize = DateTimeColumnIndex+1;

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
            headers[ToColumnIndex(difficulty, scoreColumn)] = ToString(difficulty)+ToString(scoreColumn);
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
        std::set<std::string> dateTimes;

        std::array<std::string, CsvColumnSize> columns;

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

            auto columnCount = SplitCsvLine(",", line, columns);
            if (columnCount!=CsvColumnSize)
            {
                throw std::runtime_error("incorrect columnn size.");
            }

            auto &versionName = columns[static_cast<std::size_t>(CsvMusicColumn::Version)];
            auto &csvTitle = columns[static_cast<std::size_t>(CsvMusicColumn::Title)];

            auto dbTitle = csvTitle;
            if (auto findMappedTitle = musicDatabase.FindDbTitle(csvTitle))
            {
                dbTitle = findMappedTitle.value();
            }

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

            lastVersionIndex = versionIndex;
            ies::MapIncrementCount(versionMusicCounts, versionIndex);

            auto playCount = std::stoull(columns[static_cast<std::size_t>(CsvMusicColumn::PlayCount)]);
            mTotalPlayCount += playCount;

            auto &dateTime = columns[DateTimeColumnIndex];
            dateTimes.emplace(dateTime);

            auto findActiveVersionIndex = FindVersionIndexFromDateTime(dateTime);
            if (!findActiveVersionIndex)
            {
                std::cout << ToVersionString(versionIndex) << " Title [" << dbTitle << "]\n"
                          << "DateTime: " << dateTime << " is not supported.\n";
                continue;
            }
            auto activeVersionIndex = findActiveVersionIndex.value();

            auto musicId = ToMusicId(versionIndex, musicIndex);
            auto itPair = mMusicScores.emplace(
                std::piecewise_construct,
                std::forward_as_tuple(musicId),
                std::forward_as_tuple(ToMusicId(versionIndex, musicIndex), mPlayStyle, playCount, dateTime)
            );
            auto &musicScore = itPair.first->second;

            for (auto difficulty : DifficultySmartEnum::ToRange())
            {
                auto level = columns[ToColumnIndex(difficulty, CsvScoreColumn::Level)];
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
                chartScore.ExScore = std::stoi(columns[ToColumnIndex(difficulty, CsvScoreColumn::ExScore)]);
                chartScore.PGreatCount = std::stoi(columns[ToColumnIndex(difficulty, CsvScoreColumn::PGreatCount)]);
                chartScore.GreatCount = std::stoi(columns[ToColumnIndex(difficulty, CsvScoreColumn::GreatCount)]);

                auto &missCount = columns[ToColumnIndex(difficulty, CsvScoreColumn::MissCount)];
                if (missCount!="---")
                {
                    chartScore.MissCount = {std::stoi(missCount)};
                }
                //'' HARD failed will have ex score but no miss count.

                auto clearType = columns[ToColumnIndex(difficulty, CsvScoreColumn::ClearType)];
                chartScore.ClearType = ConvertToClearType(clearType);

                auto &djLevel = columns[ToColumnIndex(difficulty, CsvScoreColumn::DjLevel)];
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

        if (dateTimes.empty())
        {
            throw std::runtime_error("empty date times in csv.");
        }

        mLastDateTime = *dateTimes.rbegin();

        if (verbose) std::cout << "DateTime [" << *dateTimes.begin() << ", " << mLastDateTime << "].\n";

        s2Time::Print<std::chrono::milliseconds>(s2Time::CountNs(begin), "CSV construct");
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
