#pragma once

#include <map>
#include <string>

#include "ies/Common/SmartEnum.hxx"

#include "score2dx/Score/MusicScore.hpp"

namespace score2dx
{

IES_SMART_ENUM(MusicScoreRecordType,
    Invalid,
    Valid
);

struct MusicScoreEntry
{
    MusicScore PropMusicScore;
    MusicScoreRecordType RecordType{MusicScoreRecordType::Invalid};
};

//! @brief Sort all MusicScore in a version by datetime timeline.
//! MusicScore record cannot decrease during a version.
//! This class help manage valid records and separate invalid records.
class VersionScoreTimeline
{
public:
    //! @brief Add a MusicScore to timeline, either update ValidRecord or saved as InvalidRecord.
    //! Valid Invariant: MusicScore is increasing better in timeline.
    //! i.e., the added MusicScore need increasingly >= prev and <= next.
    //! 1. If datetime does not exist ValidRecord and MusicRecord is valid, it's added to ValidRecords.
    //!     Otherwise, it's regarded as InvalidRecord.
    //! 2. If datetime exist, try to merge MusicScore to ValidRecord if
    //!     [Update] Adding MusicData is updating other difficulties, or same score of existing difficulties.
    //!     The added difficulies scores keeps the validness.
    //!     Otherwise, it's regarded as InvalidRecord.
    //! InvalidRecord will not be added to ValidRecords timeline.
    void
    AddMusicScore(
        const MusicScore& musicScore,
        bool keepingEntry = true);

    const std::map<std::string, MusicScore>&
    GetTimeline()
    const;

    const std::vector<MusicScoreEntry>&
    GetRecordEntries()
    const;

    void
    ClearRecordEntries();

private:
    //! @brief Map of {DateTime, ValidScore}. Timeline of valid records.
    std::map<std::string, MusicScore> mTimeline;
    //! @brief All MusicScore added by calling order kept.
    std::vector<MusicScoreEntry> mRecordEntryList;
};

}
