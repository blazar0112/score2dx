#include "score2dx/Iidx/Definition.hpp"

#include <cctype>
#include <algorithm>

#include "fmt/format.h"

#include "icl_s2/String/RecursiveReplace.hpp"

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

ClearType
ConvertToClearType(const std::string &spaceSeparatedClearType)
{
    auto s = spaceSeparatedClearType;
    icl_s2::RecursiveReplace(s, " ", "_");
    return ToClearType(s);
}

std::string
ToSpaceSeparated(ClearType clearType)
{
    auto spaceSeparated = ToString(clearType);
    icl_s2::RecursiveReplace(spaceSeparated, "_", " ");
    return spaceSeparated;
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
ToFormatted(std::size_t musicId)
{
    return fmt::format("{:05}", musicId);
}

}
