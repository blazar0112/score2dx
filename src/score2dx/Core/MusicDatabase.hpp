#pragma once

#include <optional>
#include <map>
#include <string>
#include <unordered_map>

#include "ies/Common/IntegralRange.hxx"

#include "score2dx/Core/ActiveVersion.hpp"
#include "score2dx/Core/JsonDefinition.hpp"
#include "score2dx/Iidx/Music.hpp"

namespace score2dx
{

class MusicDatabase
{
public:
        MusicDatabase();

    //! @brief Get filename created this MusicDatabase.
        const std::string &
        GetFilename()
        const;

    //! @brief Vector of {Index=VersionIndex, Vector of {Index=MusicIndex, Music}}.
        const std::vector<std::vector<Music>> &
        GetAllTimeMusics()
        const;

        const Music &
        GetMusic(std::size_t musicId)
        const;

        const std::string &
        GetTitle(std::size_t musicId)
        const;

        std::optional<std::size_t>
        FindMusicId(std::size_t versionIndex, const std::string &dbTitle)
        const;

    //! @brief Find if title is database title:
    //!     If so return nullopt.
    //!     If not, return the mapped database title.
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
    //!     * Db use half width here.
    //!     * Music changed title (official fix) between versions.
        std::optional<std::string>
        FindDbTitle(const std::string &title)
        const;

    //! @brief Same as FindDbTitle but only for CSV title.
        std::optional<std::string>
        FindCsvDbTitle(const std::string &title)
        const;

        std::optional<std::string>
        FindDbTitleMappingSection(const std::string &title)
        const;

    //! @brief Find VersionIndex of dbTitle belong to official combined version '1st&substream'.
    //! @return 0 (1st style) or 1 (substream) or nullopt (not found).
        std::optional<std::size_t>
        Find1stSubVersionIndex(const std::string &dbTitle)
        const;

    //! @brief Find Pair of {VersionIndex, MusicIndex} by versionName, and dbTitle.
    //! versionName can be Official1stSubVersionName.
        std::pair<std::size_t, std::size_t>
        FindIndexes(const std::string &versionName, const std::string &dbTitle)
        const;

        bool
        IsCsMusic(std::size_t musicId)
        const;

        const std::map<std::size_t, ActiveVersion> &
        GetActiveVersions()
        const;

        const ActiveVersion*
        FindActiveVersion(std::size_t activeVersionIndex)
        const;

    //! @brief Find music's styleDifficulty ChartInfo if available at availableVersionIndex.
    //! @return ChartInfo if find, otherwise nullptr if:
    //!     1. title is not available at that version.
    //!     2. title has no such style difficulty.
    //!     3. that style difficulty is not available at that version.
        const ChartInfo*
        FindChartInfo(std::size_t musicId,
                      StyleDifficulty styleDifficulty,
                      std::size_t availableVersionIndex)
        const;

    //! @brief Find avaiableVersionRange contains containingVersionIndex of music.
    //! e.g. music is available [[15, 20], [24], [28, 29]]
    //! FindContainingAvailableVersionRange(Ver=17) returns IndexRange{15, 21}.
        std::optional<ies::IndexRange>
        FindContainingAvailableVersionRange(std::size_t musicId,
                                            StyleDifficulty styleDifficulty,
                                            std::size_t containingVersionIndex)
        const;

/*
        ies::IntegralRangeList<std::size_t>
        GetAvailableVersions(std::size_t musicId)
        const;
*/

    //! @brief [Debug] Check database validity and print inconsistency.
        void
        CheckValidity()
        const;

private:
    std::string mDatabaseFilename{"table/MusicDatabase31_2023-10-31.json"};
    Json mDatabase;

    //! @brief Vector of {Index=VersionIndex, Vector of {Index=MusicIndex, Music}}.
    std::vector<std::vector<Music>> mAllTimeMusics;

    //! @brief Cache all 00 and 01 musics to lookup version index.
    //! Map of {DbTitle Version="00" or "01", versionIndex}.
    std::map<std::string, std::size_t> m1stSubVersionIndexMap;

    //! @brief Vector of {Index=VersionIndex, Map of {DbTitle, MusicIndex}}.
    //! @note Since index is unchanged after loading, cache all music index.
    std::vector<std::unordered_map<std::string, std::size_t>> mTitleMusicIndexByVersion;

    //! @brief Map of {VersionIndex, ActiveVersion}.
    std::map<std::size_t, ActiveVersion> mActiveVersions;

    //! @brief Generate all active versions between version range [begin, latest].
        void
        GenerateActiveVersions(std::size_t beginVersionIndex);

        const Json*
        FindDbMusic(std::size_t versionIndex, const std::string &title)
        const;
};

void
UpgradeMusicDatabase(const std::string &currentFilename,
                     const std::string &newFilename);

}
