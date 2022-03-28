#include "score2dx/Iidx/Definition.hpp"

#include <cctype>
#include <algorithm>
#include <array>

#include "fmt/format.h"

#include "ies/String/RecursiveReplace.hpp"

namespace score2dx
{

StyleDifficulty
ConvertToStyleDifficulty(PlayStyle playStyle, Difficulty difficulty)
{
    auto i = static_cast<std::size_t>(difficulty);
    i += static_cast<std::size_t>(playStyle)*DifficultySmartEnum::Size();
    return static_cast<StyleDifficulty>(i);
}

std::pair<PlayStyle, Difficulty>
Split(StyleDifficulty styleDifficulty)
{
    auto i = static_cast<std::size_t>(styleDifficulty);
    auto style = static_cast<PlayStyle>(i/DifficultySmartEnum::Size());
    auto difficulty = static_cast<Difficulty>(i%DifficultySmartEnum::Size());
    return {style, difficulty};
}

bool
IsStyle(StyleDifficulty styleDifficulty, PlayStyle playStyle)
{
    auto i = static_cast<std::size_t>(styleDifficulty);
    auto style = static_cast<PlayStyle>(i/DifficultySmartEnum::Size());
    return style==playStyle;
}

ClearType
ConvertToClearType(const std::string &spaceSeparatedClearType)
{
    auto s = spaceSeparatedClearType;
    ies::RecursiveReplace(s, " ", "_");
    return ToClearType(s);
}

std::string
ToSpaceSeparated(ClearType clearType)
{
    auto spaceSeparated = ToString(clearType);
    ies::RecursiveReplace(spaceSeparated, "_", " ");
    return spaceSeparated;
}

std::string
ToPrettyString(ClearType clearType)
{
    static const std::array<std::string, ClearTypeSmartEnum::Size()> prettyStrings
    {
        "NO PLAY",
        "FAILED",
        "ASSIST",
        "EASY",
        "CLEAR",
        "HARD",
        "EX HARD",
        "FC"
    };

    return prettyStrings[static_cast<std::size_t>(clearType)];
}

bool
IsIidxId(std::string_view iidxId)
{
    if (iidxId.size()!=9)
    {
        return false;
    }

    auto IsFourDigits = [](std::string_view sv, std::size_t pos)
    {
        return std::all_of(sv.begin()+pos, sv.begin()+pos+4, [](unsigned char c){ return std::isdigit(c); });
    };

    return IsFourDigits(iidxId, 0) && iidxId[4]=='-' && IsFourDigits(iidxId, 5);
}

std::size_t
ToMusicId(std::size_t versionIndex,
          std::size_t musicIndex)
{
    return versionIndex*1000+musicIndex;
}

std::pair<std::size_t, std::size_t>
ToIndexes(std::size_t musicId)
{
    auto versionIndex = musicId/1000;
    auto musicIndex = musicId%1000;
    return {versionIndex, musicIndex};
}

std::string
ToMusicIdString(std::size_t musicId)
{
    return fmt::format("{:05}", musicId);
}

std::size_t
ToChartId(std::size_t musicId,
          PlayStyle playStyle,
          Difficulty difficulty)
{
    return musicId*100+static_cast<std::size_t>(playStyle)*10+static_cast<std::size_t>(difficulty);
}

std::size_t
ToChartId(std::size_t musicId,
          StyleDifficulty styleDifficulty)
{
    auto [playStyle, difficulty] = Split(styleDifficulty);
    return ToChartId(musicId, playStyle, difficulty);
}

std::tuple<std::size_t, PlayStyle, Difficulty>
ToMusicStyleDiffculty(std::size_t chartId)
{
    auto musicId = chartId/100;
    auto playStyle = static_cast<PlayStyle>((chartId%100)/10);
    auto difficulty = static_cast<Difficulty>(chartId%10);
    return {musicId, playStyle, difficulty};
}

}
