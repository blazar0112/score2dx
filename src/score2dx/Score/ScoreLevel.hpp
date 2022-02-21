#pragma once

#include "ies/Common/SmartEnum.hxx"

#include "score2dx/Iidx/Definition.hpp"

namespace score2dx
{

//! @note AtMin is for calculation convenience, all functions below use lowest as F-.
//! @note Exact KeyScores: (round up)
//! Min:  0/9 of MaxScore (2*note)
//! F:    1/9
//! E:    2/9
//! D:    3/9
//! C:    4/9
//! B:    5/9
//! A:    6/9
//! 2A:   7/9
//! 3A:   8/9
//! Max:  9/9
IES_SMART_ENUM(ScoreLevel,
    Min,
    F,
    E,
    D,
    C,
    B,
    A,
    AA,
    AAA,
    Max
);

IES_SMART_ENUM(ScoreRange,
    LevelPlus,
    AtLevel,
    LevelMinus
);

//! @brief Category of ScoreLevelRange.
IES_SMART_ENUM(ScoreLevelCategory,
    AMinus,
    AEqPlus,
    AAMinus,
    AAEqPlus,
    AAAMinus,
    AAAEqPlus,
    MaxMinus,
    Max
);

using ScoreLevelRange = std::pair<ScoreLevel, ScoreRange>;

DjLevel
FindDjLevel(int note, int exScore);

int
FindKeyScore(int note, ScoreLevel scoreLevel);

//! @brief Find KeyScore to become ScoreLevel Minus range.
//! e.g. note=2000:
//!                  AA             AAA
//! KeyScore        3112           3556
//! HalfKeyScore            3334 <- choose to be HalfKeyScore of AAA
//!                  [   AA+ )[  AAA- )
int
FindHalfKeyScore(int note, ScoreLevel scoreLevel);

//! @brief FindScoreLevel of { Max, Max-, 3A+, 3A, 3A-, 2A+, ..., E-, F+, F, F- }
//! @note Lowest ScoreLevelRange is F-. (no Min+ or Min).
ScoreLevelRange
FindScoreLevelRange(int note, int exScore);

//! @brief Find difference score from ScoreLevelRange.
//! @return {ScoreLevelRange, ScoreDiff(absolute value)}.
//! e.g. AAA-5, ScoreDiff = 5, ScoreLevelRange = {AAA, LevelMinus}.
//! e.g. AA+0, ScoreDiff = 0, ScoreLevelRange = {AA, AtLevel}.
//! e.g. MAX+0, ScoreDiff = 0, ScoreLevelRange = {Max, AtLevel}.
//! e.g. AA+50, ScoreDiff = 50, ScoreLevelRange = {AA, LevelPlus}.
//! @note Lowest ScoreLevelRange is F-. (no Min+ or Min).
std::pair<ScoreLevelRange, int>
FindScoreLevelDiff(int note, int exScore);

//! @brief Convert scoreLevelRange {AAA, LevelMinus} to pretty string "AAA-".
//! @note Level in upper case.
std::string
ToPrettyString(const ScoreLevelRange &scoreLevelRange);

//! @brief Combined ScoreLevelRange and Diff value from FindScoreLevelDiff to pretty string.
//! e.g. "AAA-5", "MAX+0", "AA+0", "F+50".
std::string
ToScoreLevelDiffString(int note, int exScore);

DjLevel
FindDjLevel(const ScoreLevelRange &scoreLevelRange);

//! @brief Convert ScoreLevelCategory to pretty string, e.g. "A-".
//! Behavior is same as prefix in ToScoreLevelDiffString.
//! ScoreLevelCategory::AMinus = "A-"
//! ScoreLevelCategory::AEqPlus = "A+" (since ToScoreLevelDiffString is A+0 if AtLevel A).
std::string
ToPrettyString(ScoreLevelCategory scoreLevelCategory);

ScoreLevelCategory
FindScoreLevelCategory(int note, int exScore);

ScoreLevelCategory
FindScoreLevelCategory(const ScoreLevelRange &scoreLevelRange);

}
