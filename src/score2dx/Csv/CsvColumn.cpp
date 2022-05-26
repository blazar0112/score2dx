#include "score2dx/Csv/CsvColumn.hpp"

#include <algorithm>
#include <charconv>
#include <iostream>
#include <vector>

#include "ies/Common/IntegralRangeUsing.hpp"
#include "ies/Time/TimeUtilFormat.hxx"

#include "score2dx/Core/CheckedParse.hxx"
#include "score2dx/Iidx/Definition.hpp"
#include "score2dx/Iidx/Version.hpp"

namespace score2dx
{

std::size_t
ToCsvColumnIndex(Difficulty difficulty, CsvScoreColumn scoreColumn)
{
    return CsvMusicColumnSmartEnum::Size()
           +static_cast<std::size_t>(difficulty)*CsvScoreColumnSmartEnum::Size()
           +static_cast<std::size_t>(scoreColumn);
}

CsvMusic
ParseCsvLine(std::string_view csvLine)
{
    //auto begin = ies::Time::Now();

    static std::size_t PreviousParsedVersionIndex = 0;
    static std::vector<std::string> CsvClearTypes;
    if (CsvClearTypes.empty())
    {
        CsvClearTypes.reserve(ClearTypeSmartEnum::Size());
        for (auto clearType : ClearTypeSmartEnum::ToRange())
        {
            CsvClearTypes.emplace_back(ToSpaceSeparated(clearType));
        }
    }

    static std::vector<std::string> DjLevels;
    if (DjLevels.empty())
    {
        DjLevels.reserve(DjLevelSmartEnum::Size());
        for (auto djLevel : DjLevelSmartEnum::ToRange())
        {
            DjLevels.emplace_back(ToString(djLevel));
        }
    }

    auto count = std::count(csvLine.begin(), csvLine.end(), ',');
    if (count!=CsvColumnSize-1)
    {
        throw std::runtime_error("csvLine has incorrect column count.");
    }

    CsvMusic music;

    std::size_t start = 0;
    std::size_t end = 0;
    std::size_t index = 0;

    while ((start = csvLine.find_first_not_of(',', end))!=std::string_view::npos)
    {
        end = csvLine.find(',', start+1);
        if (end==std::string_view::npos)
        {
            end = csvLine.length();
        }

        auto column = csvLine.substr(start, end-start);

        if (index<CsvMusicColumnSmartEnum::Size())
        {
            auto musicColumn = static_cast<CsvMusicColumn>(index);

            switch (musicColumn)
            {
                case CsvMusicColumn::Version:
                {
                    if (PreviousParsedVersionIndex!=0)
                    {
                        if (column==VersionNames[PreviousParsedVersionIndex])
                        {
                            music.CsvVersionIndex = PreviousParsedVersionIndex;
                            break;
                        }
                    }

                    if (column==Official1stSubVersionName)
                    {
                        music.CsvVersionIndex = 1;
                        PreviousParsedVersionIndex = 1;
                        break;
                    }

                    for (auto i : IndexRange{2, VersionNames.size()})
                    {
                        if (column==VersionNames[i])
                        {
                            music.CsvVersionIndex = i;
                            PreviousParsedVersionIndex = i;
                            break;
                        }
                    }

                    break;
                }
                case CsvMusicColumn::Title:
                {
                    music.Title = column;
                    break;
                }
                case CsvMusicColumn::Genre:
                {
                    music.Genre = column;
                    break;
                }
                case CsvMusicColumn::Artist:
                {
                    music.Artist = column;
                    break;
                }
                case CsvMusicColumn::PlayCount:
                {
                    CheckedParse(column, music.PlayCount, "PlayCount");
                    break;
                }
                default:
                    break;
            }
        }
        else if (index==DateTimeColumnIndex)
        {
            music.DateTime = column;
        }
        else
        {
            auto allScoreIndex = (index-CsvMusicColumnSmartEnum::Size());
            auto difficultyIndex = allScoreIndex/CsvScoreColumnSmartEnum::Size();
            auto scoreColumn = static_cast<CsvScoreColumn>(allScoreIndex%CsvScoreColumnSmartEnum::Size());

            auto &chartScore = music.ChartScores[difficultyIndex];

            switch (scoreColumn)
            {
                case CsvScoreColumn::Level:
                {
                    CheckedParse(column, chartScore.Level, "Level");
                    break;
                }
                case CsvScoreColumn::ExScore:
                {
                    CheckedParse(column, chartScore.ExScore, "ExScore");
                    break;
                }
                case CsvScoreColumn::PGreatCount:
                {
                    CheckedParse(column, chartScore.PGreatCount, "PGreatCount");
                    break;
                }
                case CsvScoreColumn::GreatCount:
                {
                    CheckedParse(column, chartScore.GreatCount, "GreatCount");
                    break;
                }
                case CsvScoreColumn::MissCount:
                {
                    //'' HARD failed will have ex score but no miss count.
                    if (column!="---")
                    {
                        int missCount{0};
                        CheckedParse(column, missCount, "GreatCount");
                        chartScore.MissCount = missCount;
                    }
                    break;
                }
                case CsvScoreColumn::ClearType:
                {
                    for (auto i : IndexRange{0, CsvClearTypes.size()})
                    {
                        if (column==CsvClearTypes[i])
                        {
                            chartScore.ClearType = static_cast<ClearType>(i);
                            break;
                        }
                    }
                    break;
                }
                case CsvScoreColumn::DjLevel:
                {
                    if (column!="---")
                    {
                        for (auto i : IndexRange{0, DjLevels.size()})
                        {
                            if (column==DjLevels[i])
                            {
                                chartScore.DjLevel = static_cast<DjLevel>(i);
                                break;
                            }
                        }
                    }
                    break;
                }
                default:
                    break;
            }
        }

        ++index;
    }

    //ies::Time::Print<std::chrono::microseconds>(ies::Time::CountNs(begin), "ParseCsvLine");

    return music;
}

void
Print(const CsvMusic csvMusic)
{
    std::cout << "CsvMusic:\n"
              << "VersionIndex: " << csvMusic.CsvVersionIndex << "\n"
              << "Title: [" << csvMusic.Title << "]\n"
              << "Genre: [" << csvMusic.Genre << "]\n"
              << "Artist: [" << csvMusic.Artist << "]\n"
              << "PlayCount: " << csvMusic.PlayCount << "\n"
              << "DateTime: [" << csvMusic.DateTime << "]\n";
    for (auto difficulty : DifficultySmartEnum::ToRange())
    {
        auto difficultyIndex = static_cast<std::size_t>(difficulty);
        auto &chartScore = csvMusic.ChartScores[difficultyIndex];
        std::cout << "[" << ToString(difficulty) << "]: " << ToString(chartScore) << "\n";
    }
}

}
