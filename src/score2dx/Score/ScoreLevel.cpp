#include "score2dx/Score/ScoreLevel.hpp"

#include <algorithm>
#include <iostream>
#include <set>
#include <stdexcept>

#include "icl_s2/Common/IntegralRangeUsing.hpp"
#include "icl_s2/StdUtil/Find.hxx"
#include "icl_s2/StdUtil/FormatString.hxx"
#include "icl_s2/StdUtil/ReverseEqualRange.hxx"
#include "icl_s2/Time/TimeUtilFormat.hxx"

namespace s2Time = icl_s2::Time;

namespace score2dx
{

DjLevel
FindDjLevel(int note, int exScore)
{
    //'' int value DjLevel ScoreLevel
    //'' 0         F       Min
    //'' 1         E       F
    //'' 2         D       E
    //'' 3         C       D
    //'' 4         B       C
    //'' 5         A       B
    //'' 6         2A      A
    //'' 7         3A      2A
    //'' 8                 3A
    //'' 9                 Max

    auto [scoreLevel, scoreRange] = FindScoreLevelRange(note, exScore);
    if (scoreLevel==ScoreLevel::F)
    {
        return DjLevel::F;
    }
    if (scoreLevel==ScoreLevel::Max)
    {
        return DjLevel::AAA;
    }

    if (scoreRange!=ScoreRange::LevelMinus)
    {
        return static_cast<DjLevel>(static_cast<int>(scoreLevel)-1);
    }

    return static_cast<DjLevel>(static_cast<int>(scoreLevel)-2);
}

int
FindKeyScore(int note, ScoreLevel scoreLevel)
{
    auto maxScore = note*2;
    auto multiple = static_cast<int>(scoreLevel);
    return static_cast<int>(std::ceil(static_cast<double>(maxScore)*multiple/9));
}

int
FindHalfKeyScore(int note, ScoreLevel scoreLevel)
{
    auto maxScore = note*2;
    auto multiple = static_cast<int>(scoreLevel)*2-1;
    return static_cast<int>(std::ceil(static_cast<double>(maxScore)*multiple/18));
}

ScoreLevelRange
FindScoreLevelRange(int note, int exScore)
{
    return FindScoreLevelRangeDiff(note, exScore).first;
}

std::pair<ScoreLevelRange, int>
FindScoreLevelRangeDiff(int note, int exScore)
{
    if (note<=0)
    {
        throw std::runtime_error("note is non-positive.");
    }

    auto maxScore = note*2;
    if (exScore<0) exScore = 0;
    if (exScore>maxScore) exScore = maxScore;

    if (exScore==maxScore)
    {
        return {{ScoreLevel::Max, ScoreRange::AtLevel}, 0};
    }

    //'' Range                Diff
    //'' A-: [(A+B)/2, A)     A - s
    //'' A+: [A, 1.5A)        s - A
    //'' AA-: [1.5A, 2A)      2A - s
    //'' AA+: [2A, 2.5A)      s - 2A
    //'' AAA-: [2.5A, 3A)     3A - s
    //'' AAA+: [3A, 3.5A)     s - 3A
    //'' MAX-: [3.5A, MAX)    Max - s
    //'' MAX(+0): [MAX]       s - Max
    //''
    //''        Max 3.5A  3A 2.5A  ... E E.F F   Min
    //'' n/9     9   8.5   8  7.5      2 1.5 1   0
    //'' n/18   18    17  16   15      4   3 2 1 0
    //'' in set of {0, 1, 2, ..., 16, 17, 18}, form range of [prev, next), find key in set
    //'' if key is even, is [Level, half Level), diff = score - begin, Range = Level Plus (Or At Level)
    //''           odd      [half lower Level, Level), diff = end - score, Range = Level Minus
    std::set<int> keyScores;
    for (auto i : IntRange{0, 18+1})
    {
        auto keyScore = static_cast<int>(std::ceil(static_cast<double>(maxScore)*i/18));
        keyScores.emplace(keyScore);
    }

    //'' find range [RLB, UB) contains exScore
    //'' begin is first value <= score. (RLB)
    //'' end is first value > score. (UB)

    auto upperBound = keyScores.upper_bound(exScore);
    //'' is only possible if score >= maxScore.
    if (upperBound==keyScores.end())
    {
        //'' but already handle max before.
        throw std::runtime_error("cannot find upper bound.");
    }

    auto reverseLowerBound = icl_s2::ReverseLowerBound(keyScores, exScore);
    //'' is only possible if score < minScore.
    if (reverseLowerBound==keyScores.rend())
    {
        //'' should be impossible, since exScore is in [0, maxScore] range, and keyScores just pivots of the range.
        throw std::runtime_error("cannot find reverse lower bound.");
    }

    auto beginScore = *reverseLowerBound;
    auto endScore = *upperBound;

    auto index = std::distance(keyScores.begin(), keyScores.find(beginScore));

    auto scoreLevel = ScoreLevel::F;
    auto scoreRange = ScoreRange::AtLevel;
    int diff = 0;

    if (index%2==0)
    {
        scoreLevel = static_cast<ScoreLevel>(index/2);
        diff = exScore-beginScore;

        if (diff!=0)
        {
            scoreRange = ScoreRange::LevelPlus;
        }
    }
    else
    {
        scoreLevel = static_cast<ScoreLevel>((index+1)/2);
        scoreRange = ScoreRange::LevelMinus;
        diff = endScore-exScore;
    }

    //'' need adjust Min+ and Min.
    if (scoreLevel==ScoreLevel::Min)
    {
        scoreLevel = ScoreLevel::F;
        scoreRange = ScoreRange::LevelMinus;
        auto fScore = FindKeyScore(note, ScoreLevel::F);
        diff = fScore-exScore;
    }

    return {{scoreLevel, scoreRange}, diff};
}

std::string
ToString(const ScoreLevelRange &scoreLevelRange)
{
    auto &[scoreLevel, scoreRange] = scoreLevelRange;

    auto prettyScoreLevel = ToString(scoreLevel);
    std::transform(prettyScoreLevel.begin(), prettyScoreLevel.end(), prettyScoreLevel.begin(), ::toupper);
    if (scoreLevel==ScoreLevel::Min)
    {
        prettyScoreLevel = "F";
    }

    switch (scoreRange)
    {
        case ScoreRange::LevelPlus:
            return prettyScoreLevel+"+";
        case ScoreRange::AtLevel:
            return prettyScoreLevel;
        case ScoreRange::LevelMinus:
            return prettyScoreLevel+"-";
    }

    return prettyScoreLevel;
}

std::string
ToScoreLevelRangeDiffString(int note, int exScore)
{
    auto [scoreLevelRange, scoreDiff] = FindScoreLevelRangeDiff(note, exScore);
    if (scoreDiff==0)
    {
        return ToString(scoreLevelRange)+"+"+std::to_string(scoreDiff);
    }
    return ToString(scoreLevelRange)+std::to_string(scoreDiff);
}

}