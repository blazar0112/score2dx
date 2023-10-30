#pragma once

#include <array>
#include <string>
#include <string_view>

#include "ies/Common/SmartEnum.hxx"

#include "score2dx/Iidx/Definition.hpp"
#include "score2dx/Score/ChartScore.hpp"

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

constexpr std::size_t DateTimeColumnIndex = CsvMusicColumnSmartEnum::Size()+DifficultySmartEnum::Size()*CsvScoreColumnSmartEnum::Size();
constexpr std::size_t CsvColumnSize = DateTimeColumnIndex+1;

std::size_t
ToCsvColumnIndex(Difficulty difficulty, CsvScoreColumn scoreColumn);

struct CsvMusic
{
    //'' [1, 29], 0 means error version string, 1 means "1st&substream".
    std::size_t CsvVersionIndex{0};
    std::size_t PlayCount{0};
    std::string Title;
    std::string Genre;
    std::string Artist;
    std::string DateTime;

    std::array<ChartScore, DifficultySmartEnum::Size()> ChartScores;
};

CsvMusic
ParseCsvLine(std::string_view csvLine);

void
Print(const CsvMusic& csvMusic);

}
