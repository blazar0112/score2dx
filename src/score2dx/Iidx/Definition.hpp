#pragma once

#include <string>
#include <string_view>
#include <utility>

#include "icl_s2/Common/SmartEnum.hxx"

namespace score2dx
{

inline constexpr int MaxLevel = 12;

ICL_S2_SMART_ENUM(PlayStyle,
    SinglePlay,
    DoublePlay
);

//! @note Prefer to use PlayStyle in code. This for parse/generate string of desired use.
ICL_S2_SMART_ENUM(PlayStyleAcronym,
    SP,
    DP
);

ICL_S2_SMART_ENUM(Difficulty,
    Beginner,
    Normal,
    Hyper,
    Another,
    Leggendaria
);

//! @note Prefer to use Difficulty in code. This for parse/generate string of desired use.
ICL_S2_SMART_ENUM(DifficultyAcronym,
    B,
    N,
    H,
    A,
    L
);

ICL_S2_SMART_ENUM(StyleDifficulty,
    SPB,
    SPN,
    SPH,
    SPA,
    SPL,
    DPB,
    DPN,
    DPH,
    DPA,
    DPL
);

StyleDifficulty
ConvertToStyleDifficulty(PlayStyle playStyle, Difficulty difficulty);

std::pair<PlayStyle, Difficulty>
Split(StyleDifficulty styleDifficulty);

//! @note To match string used in CSV, use capitalized style, and underscore for space.
//! i.e. FULLCOMBO_CLEAR can replace to "FULLCOMBO CLEAR" in CSV.
ICL_S2_SMART_ENUM(ClearType,
    NO_PLAY,
    FAILED,
    ASSIST_CLEAR,
    EASY_CLEAR,
    CLEAR,
    HARD_CLEAR,
    EX_HARD_CLEAR,
    FULLCOMBO_CLEAR
);

ClearType
ConvertToClearType(const std::string &spaceSeparatedClearType);

std::string
ToSpaceSeparated(ClearType clearType);

//! @brief DjLevel is arcade's DJ LEVEL, so it only has at most AAA.
//! To express "MAX", "MAX-", "AAA+", @see ScoreLevel.
ICL_S2_SMART_ENUM(DjLevel,
    F,
    E,
    D,
    C,
    B,
    A,
    AA,
    AAA
);

bool
IsIidxId(std::string_view iidxId);

std::size_t
ToMusicId(std::size_t versionIndex,
          std::size_t musicIndex);

//! @brief Split to pair of {VersionIndex, MusicIndex}.
std::pair<std::size_t, std::size_t>
ToIndexes(std::size_t musicId);

std::string
ToFormatted(std::size_t musicId);

std::size_t
ToChartId(std::size_t musicId,
          PlayStyle playStyle,
          Difficulty difficulty);

std::size_t
ToChartId(std::size_t musicId,
          StyleDifficulty styleDifficulty);

std::tuple<std::size_t, PlayStyle, Difficulty>
ToMusicStyleDiffculty(std::size_t chartId);

}
