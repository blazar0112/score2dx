#pragma once

#include <map>
#include <string_view>

#include "score2dx/Analysis/Analyzer.hpp"
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

    //! @brief Load directory of player score data and update player score analysis.
    //! Directory name need in form of IIDX ID format.
    //! Search and load all CSV begin with that ID. Also load all exported files with same ID inside.
    //! Do nothing if directory is not IIDX ID or failed.
    //! @return If load directory succeeded.
        bool
        LoadDirectory(std::string_view directory,
                      bool verbose=false,
                      bool checkWithDatabase=false);

    //! @brief Export loaded PlayerScore to score2dx Json format data.
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

    //! @brief Import score2dx Json format data.
    //! @note Does not update player score analysis.
        void
        Import(const std::string &requiredIidxId,
               const std::string &exportedFilename,
               bool verbose=false);

        const PlayerScore &
        GetPlayerScore(const std::string &iidxId)
        const;

    //! @brief Map of {IidxId, PlayerScore}.
        const std::map<std::string, PlayerScore> &
        GetPlayerScores()
        const;

    //! @brief Map of {DateTime, Csv}.
        std::map<std::string, const Csv*>
        GetCsvs(const std::string &iidxId, PlayStyle playStyle)
        const;

    //! @brief Set active version for score analysis, also clear all previous stored player analyses.
    //! Call Analyze for player manually. Not auto analyze for all players.
        void
        SetActiveVersionIndex(std::size_t activeVersionIndex);

        std::size_t
        GetActiveVersionIndex()
        const;

    //! @brief Generate score analysis for player of IIDX ID.
    //! @note LoadDirectory will analyze once after loaded. Use for manually import player score.
        void
        Analyze(const std::string &iidxId);

    //! @brief Find if player of IIDX ID has analysis.
    //! @note SetActiveVersion, LoadDirectory, Analyze invalidate previous ScoreAnalysis.
        const ScoreAnalysis*
        FindAnalysis(const std::string &iidxId)
        const;

        const ActivityAnalysis*
        FindVersionActivityAnalysis(const std::string &iidxId)
        const;

    //! @note Requires manually call analyze because need date time range, does not auto
    //! analyze upon LoadDirectory.
        void
        AnalyzeActivity(const std::string &iidxId,
                        const std::string &beginDateTime,
                        const std::string &endDateTime);

    //! @brief Find if player of IIDX ID has activity analysis.
        const ActivityAnalysis*
        FindActivityAnalysis(const std::string &iidxId)
        const;

    //! @brief Return IIDX ID if user exist, empty if user not found.
    //! Throw if CURL has error.
        std::string
        AddIidxMeUser(const std::string &user);

    //! @brief Get data of IIDX ME user and export to
    //!     ME/<IIDX_ID>/score2dx_export_<PlayStyleAcronym>_<CurrentDate>[_<suffix>].json
        void
        ExportIidxMeData(const std::string &user, std::size_t endVersionIndex=30);

        void
        CheckIidxMeDataTable()
        const;

private:
    MusicDatabase mMusicDatabase;

    //! @brief Map of {IidxId, PlayerScore}.
    std::map<std::string, PlayerScore> mPlayerScores;

    //! @brief Map of {IidxId, Map of {PlayStyle, Map of {DateTime, Csv}}}.
    std::map<std::string, std::map<PlayStyle, std::map<std::string, std::unique_ptr<Csv>>>> mPlayerCsvs;

    Analyzer mAnalyzer;
    //! @brief Map of {IidxId, ScoreAnalysis}.
    std::map<std::string, ScoreAnalysis> mPlayerAnalyses;
    //! @brief Map of {IidxId, ActivityAnalysis(ActiveVersionDateTimeRange)}.
    std::map<std::string, ActivityAnalysis> mPlayerVersionActivityAnalyses;
    //! @brief Map of {IidxId, ActivityAnalysis(SpecificDateTimeRange)}.
    std::map<std::string, ActivityAnalysis> mPlayerActivityAnalyses;

    //! @brief Map of {IidxMeUser, Iidxid}.
    std::map<std::string, std::string> mIidxMeUserIdMap;

        void
        CreatePlayer(const std::string &iidxId);

        void
        AddCsvToPlayerScore(const std::string &iidxId,
                            PlayStyle playStyle,
                            const std::string &dateTime);

        void
        Analyze(const std::string &iidxId,
                const PlayerScore &playerScore);

        void
        Export(const PlayerScore &playerScore,
               PlayStyle playStyle,
               const std::string &outputDirectory,
               const std::string &dateTimeType="official",
               const std::string &suffix="")
        const;
};

}
