#include "score2dx/Score/PlayerScore.hpp"

#include <iostream>
#include <optional>
#include <stdexcept>

namespace score2dx
{

PlayerScore::
PlayerScore(const MusicDatabase &musicDatabase, const std::string &iidxId)
:   mMusicDatabase(musicDatabase),
    mIidxId(iidxId)
{
    if (!IsIidxId(iidxId))
    {
        throw std::runtime_error("["+iidxId+"] is not a valid IIDX ID.");
    }
}

const std::string &
PlayerScore::
GetIidxId()
const
{
    return mIidxId;
}

void
PlayerScore::
AddMusicScore(std::size_t scoreVersionIndex,
              const MusicScore &musicScore)
{
    auto musicId = musicScore.GetMusicId();
    auto& music = mMusicDatabase.GetMusic(musicId);
    auto styleIndex = ToIndex(musicScore.GetPlayStyle());
    auto& versionScoreTables = mStyleVersionScoreTables[styleIndex];
    auto [it, flag] = versionScoreTables.emplace(musicId, music);
    auto& scoreTable = it->second;
    scoreTable.AddMusicScore(scoreVersionIndex, musicScore);
}

/*
void
PlayerScore::
AddChartScore(std::size_t scoreVersionIndex,
              std::size_t musicId,
              PlayStyle playStyle,
              Difficulty difficulty,
              const std::string &dateTime,
              const ChartScore &chartScore)
{
    auto& music = mMusicDatabase.GetMusic(musicId);
    auto [it, flag] = mVersionScoreTables.emplace(musicId, music);
    auto& scoreTable = it->second;
    scoreTable.AddChartScore(scoreVersionIndex, dateTime, playStyle, difficulty, chartScore);
}
*/

void
PlayerScore::
Propagate()
{
/*
    for (auto &[musicId, versionScoreTable] : mVersionScoreTables)
    {
        auto &music = mMusicDatabase.GetMusic(musicId);
        versionScoreTable.CleanupInVersionScores();

        for (auto playStyle : PlayStyleSmartEnum::ToRange())
        {
            for (auto difficulty : DifficultySmartEnum::ToRange())
            {
                auto styleDifficulty = ConvertToStyleDifficulty(playStyle, difficulty);

                std::optional<std::size_t> previousClearVersionIndex;
                for (auto scoreVersionIndex : GetSupportScoreVersionRange())
                {
                    auto &availability = music.GetChartAvailability(styleDifficulty, scoreVersionIndex);
                    if (availability.ChartAvailableStatus!=ChartStatus::BeginAvailable
                        &&availability.ChartAvailableStatus!=ChartStatus::Available)
                    {
                        previousClearVersionIndex = std::nullopt;
                        continue;
                    }

                    auto* chartScorePtr = versionScoreTable.GetBestChartScore(scoreVersionIndex, playStyle, difficulty);
                    if (chartScorePtr)
                    {
                        previousClearVersionIndex = scoreVersionIndex;
                        continue;
                    }

                    if (!previousClearVersionIndex)
                    {
                        continue;
                    }

                    auto &beginDateTime = GetVersionDateTimeRange(scoreVersionIndex).Get(ies::RangeSide::Begin);
                    ChartScore chartScore;

                    auto* previousBestChartScorePtr = versionScoreTable.GetBestChartScore(previousClearVersionIndex.value(), playStyle, difficulty);
                    if (!previousBestChartScorePtr)
                    {
                        if (!previousClearVersionIndex) continue;
                        std::cout << "previousClearVersionIndex = " << previousClearVersionIndex.value() << "\n";
                        for (auto &[dateTime, musicScore] : versionScoreTable.GetMusicScores(previousClearVersionIndex.value(), playStyle))
                        {
                            std::cout << "[" << dateTime << "]: \n";
                            musicScore.Print();
                        }
                        std::cout << ToString(difficulty) << "\n";
                        std::cout << std::flush;
                        throw std::runtime_error("previous clear should have best chart score.");
                    }

                    chartScore.Level = availability.ChartInfoProp.Level;
                    chartScore.ClearType = previousBestChartScorePtr->ClearType;

                    versionScoreTable.AddChartScore(scoreVersionIndex, beginDateTime, playStyle, difficulty, chartScore);

                    previousClearVersionIndex = scoreVersionIndex;
                }
            }
        }
    }
*/
}

const std::map<std::size_t, VersionScoreTable> &
PlayerScore::
GetVersionScoreTables(PlayStyle playStyle)
const
{
    auto styleIndex = ToIndex(playStyle);
    return mStyleVersionScoreTables[styleIndex];
}


}
