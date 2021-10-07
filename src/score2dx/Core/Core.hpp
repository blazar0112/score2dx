#pragma once

#include <map>
#include <string_view>

#include "score2dx/Core/JsonDefinition.hpp"
#include "score2dx/Core/MusicInfo.hpp"
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

    //! @brief Create Player's PlayerScore, do nothing if IIDX ID is invalid or player already exist.
        void
        AddPlayer(const std::string &iidxId);

    //! @brief Load directory of name in IIDX ID format.
    //! Search and load all CSV begin with that ID. Also load all exported files with same ID inside.
    //! Do nothing if directory is not IIDX ID or failed.
    //! @return If load directory succeeded.
        bool
        LoadDirectory(std::string_view directory, bool verbose=false);

    //! @brief Find if title is database title, if not, return the mapped database title.
    //! DbTitle: actual title used in DB, almost same as official title.
    //!     * Not replace half width character to full width like CSV title.
    //! Official title: occurs in official data in website or CSV.
    //!
    //! Known mismatch title mapping are recorded in database titleMapping sections:
    //! 'display': display title is official title but without removing special characters
    //!     (like unicode 'heart').
    //!     Each music in DB record its display title (if needed), and mapping back to DB here.
    //! 'wiki': any mismatch from official CSV, not limit to wiki
    //!     * Typo in bemani wiki
    //!         because wiki is manually input, especially for unicode characters
    //!         and konmai in official title, e.g. some have two spaces
    //!     * Any additional mapping also record here, from any 3rd party data site,
    //!         like iidx.me, IST, ereter
    //! 'csv': few titles have half width puncuation converted to full width in CSV.
        std::optional<std::string>
        FindDbTitle(const std::string &title)
        const;

    //! @brief Find music index in version musics.
        std::optional<std::size_t>
        FindMusicIndex(std::size_t versionIndex, const std::string &dbTitle)
        const;

    //! @brief Find {VersionIndex, MusicIndex} from officialVersionName and dbTitle.
    //! @note Because official combines database version '1st style' and 'substream' to '1st&substream'.
    //! To know which version title belongs and what's music index, need search version musics in DB.
        std::pair<std::size_t, std::size_t>
        FindIndexes(const std::string &officialVersionName, const std::string &dbTitle)
        const;

    //! @brief Get version's music title in vector, index same as musicIndex.
        const std::vector<std::string> &
        GetVersionMusics(std::size_t versionIndex)
        const;

        MusicInfo
        GetMusicInfo(std::size_t musicId)
        const;

        bool
        IsCsMusic(std::size_t musicId)
        const;

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
    Json mMusicDatabase;
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

        const Json*
        FindDbMusic(std::size_t versionIndex, const std::string &title)
        const;
};

std::string
ToDbVersionKey(std::size_t versionIndex);

}
