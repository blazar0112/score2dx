#pragma once

#include <map>
#include <string_view>

#include "score2dx/Core/JsonDefinition.hpp"
#include "score2dx/Core/MusicDatabase.hpp"
#include "score2dx/Csv/Csv.hpp"
#include "score2dx/Score/PlayerScore.hpp"

namespace score2dx
{

const std::string ExampleExportFilename = "score2dx_export_DP_2021-09-12.json";
const std::size_t MinExportFilenameSize = ExampleExportFilename.size();

class Core
{
public:
        Core();

        const MusicDatabase &
        GetMusicDatabase()
        const;

    //! @brief Create Player's PlayerScore, do nothing if IIDX ID is invalid or player already exist.
        void
        AddPlayer(const std::string &iidxId);

    //! @brief Load directory of name in IIDX ID format.
    //! Search and load all CSV begin with that ID. Also load all exported files with same ID inside.
    //! Do nothing if directory is not IIDX ID or failed.
    //! @return If load directory succeeded.
        bool
        LoadDirectory(std::string_view directory, bool verbose=false);

    //! @brief Export loaded PlayerScore to score2dx json format export data.
    //! Filename: score2dx_export_<PlayStyleAcronym>_<CurrentDate>[_<suffix>].json
    //! @example Load csv and export filename will be:
    //!     score2dx_export_DP_2021-09-12.json
    //! @note Will not generate file if have no data of IIDX ID or PlayStyle.
        void
        Export(const std::string &iidxId,
               PlayStyle playStyle,
               const std::string &outputDirectory,
               const std::string &dateTimeType="official",
               const std::string &suffix="")
        const;

        void
        Import(const std::string &requiredIidxId,
               const std::string &exportedFilename,
               bool verbose=false);

    //! @brief Map of {IidxId, PlayerScore}.
        const std::map<std::string, PlayerScore> &
        GetPlayerScores()
        const;

    //! @brief Map of {DateTime, Csv}.
        std::map<std::string, const Csv*>
        GetCsvs(const std::string &iidxId, PlayStyle playStyle)
        const;

private:
    MusicDatabase mMusicDatabase;
    std::vector<std::vector<std::string>> mAllVersionMusics;

    //! @brief Map of {IidxId, PlayerScore}.
    std::map<std::string, PlayerScore> mPlayerScores;

    //! @brief Map of {IidxId, Map of {PlayStyle, Map of {DateTime, Csv}}}.
    std::map<std::string, std::map<PlayStyle, std::map<std::string, std::unique_ptr<Csv>>>> mAllPlayerCsvs;

        void
        CreatePlayer(const std::string &iidxId);

        void
        AddCsvToPlayerScore(const std::string &iidxId,
                            PlayStyle playStyle,
                            const std::string &dateTime);
};



}
