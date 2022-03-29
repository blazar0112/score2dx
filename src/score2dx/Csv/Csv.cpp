#include "score2dx/Csv/Csv.hpp"

#include <algorithm>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <map>
#include <set>
#include <string_view>
#include <sstream>

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
        int lineCount = 0;

        std::size_t lastVersionIndex = 0;
        std::map<std::string, int> debugCounts;
        std::string minDateTime;
        std::string maxDateTime;

        std::map<std::string, s2Time::NsCountType> profNsCounts;

        auto fileSize = fs::file_size(mPath);
        std::vector<char> bytes(fileSize);
        csvFile.read(bytes.data(), fileSize);
        std::string buffer(bytes.data(), bytes.size());
        std::string_view bufferView{buffer};
        auto lineView = bufferView;

        try
        {
            std::size_t start = 0;
            std::size_t end = 0;

            while ((start = bufferView.find_first_not_of('\n', end))!=std::string_view::npos)
            {
                end = bufferView.find('\n', start+1);
                if (end==std::string_view::npos)
                {
                    end = buffer.length();
                }

                ++lineCount;
                if (lineCount==1)
                {
                    continue;
                }

                lineView = bufferView.substr(start, end-start);

                auto csvMusic = ParseCsvLine(lineView);

                auto dbTitle = csvMusic.Title;
                if (auto findMappedTitle = musicDatabase.FindCsvDbTitle(csvMusic.Title))
                {
                    dbTitle = findMappedTitle.value();
                }

                std::optional<std::size_t> findVersionIndex = csvMusic.CsvVersionIndex;
                if (csvMusic.CsvVersionIndex==1)
                {
                    findVersionIndex = musicDatabase.Find1stSubVersionIndex(dbTitle);
                }

                if (!findVersionIndex)
                {
                    throw std::runtime_error("cannot find Ver["+ToVersionString(csvMusic.CsvVersionIndex)+"] title "+dbTitle+" version index.");
                }

                auto versionIndex = findVersionIndex.value();

                auto findMusicIndex = musicDatabase.FindMusicIndex(versionIndex, dbTitle);
                if (!findMusicIndex)
                {
                    if (versionIndex==GetLatestVersionIndex())
                    {
                        std::cout << "Possible ["+ToVersionString(versionIndex)+"] new music title ["+csvMusic.Title+"] not in database, skipped.\n";
                        continue;
                    }
                    throw std::runtime_error("music title ["+dbTitle+"] is not listed in it's version ["+ToVersionString(versionIndex)+"] in database");
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

                mTotalPlayCount += csvMusic.PlayCount;

                auto &dateTime = csvMusic.DateTime;
                if (maxDateTime.empty() || dateTime>maxDateTime)
                {
                    maxDateTime = dateTime;
                }
                if (minDateTime.empty() || dateTime<minDateTime)
                {
                    minDateTime = dateTime;
                }

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
                    std::forward_as_tuple(ToMusicId(versionIndex, musicIndex), mPlayStyle, csvMusic.PlayCount, dateTime)
                );
                auto &musicScore = itPair.first->second;

                for (auto difficulty : DifficultySmartEnum::ToRange())
                {
                    auto difficultyIndex = static_cast<std::size_t>(difficulty);
                    auto &csvChartScore = csvMusic.ChartScores[difficultyIndex];

                    if (csvChartScore.Level==0)
                    {
                        continue;
                    }

                    auto &chartScore = musicScore.EnableChartScore(difficulty);
                    chartScore = csvChartScore;

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
                            if (chartScore.Level!=chartInfo.Level)
                            {
                                std::cout << "[" << dbTitle << "] level mismatch:\n"
                                          << "CSV [" << chartScore.Level << "]\n"
                                          << "DB [" << chartInfo.Level << "]\n";
                            }
                        }
                    }

                    if (checkWithDatabase && chartScore.ExScore!=0)
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
        catch (const std::exception &e)
        {
            throw std::runtime_error("ParseCsvLine exception:\n    "
                                     +std::string{e.what()}+"\n"
                                     +"Line ("+std::to_string(lineCount)+"): "+std::string{lineView}+"\n");
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
