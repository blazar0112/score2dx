#pragma once

#include <map>
#include <string>
#include <string_view>

#include "score2dx/Core/JsonDefinition.hpp"
#include "score2dx/Core/MusicDatabase.hpp"
#include "score2dx/Score/MusicScore.hpp"

namespace score2dx
{

const std::string ExampleCsvFilename = "5483-7391_dp_score.csv";
const std::size_t MinCsvFilenameSize = ExampleCsvFilename.size();

//! @brief Represent data from Konami official CSV.
//! CSV has filename format of [IIDX ID]_[sp|dp]_score.csv.
//! e.g. ExampleCsvFilename = "5483-7391_dp_score.csv";
//!
//! To allow user manage CSV files saved from different times,
//!     CSV can have filename format of [IIDX ID]_[sp|dp]_score[annotation].csv.
//!     e.g. "5483-7391_dp_score_20210831.csv"
//!
//! "annotation" has no meaning in code and is not checked or used for DateTime,
//!     DateTime is parsed from CSV.
class Csv
{
public:
        Csv(const std::string &csvPath, const MusicDatabase &musicDatabase, bool verbose=false);

        const std::string &
        GetFilename()
        const;

        PlayStyle
        GetPlayStyle()
        const;

        const std::string &
        GetVersion()
        const;

        const std::string &
        GetLastDateTime()
        const;

        std::size_t
        GetTotalPlayCount()
        const;

        const std::map<std::size_t, MusicScore> &
        GetScores()
        const;

        void
        PrintSummary()
        const;

private:
    bool mCheckWithDatabase{false};

    std::string mPath;
    std::string mFilename;
    std::string mIidxId;
    PlayStyle mPlayStyle{PlayStyle::SinglePlay};
    std::string mVersion;

    std::string mLastDateTime;
    std::size_t mMusicCount{0};
    std::size_t mTotalPlayCount{0};

    //! @brief Map of {MusicId, MusicScore}.
    std::map<std::size_t, MusicScore> mMusicScores;
};

//! @return {IsValid, InvalidReason}.
std::pair<bool, std::string>
IsValidCsvFilename(std::string_view filename);

}
