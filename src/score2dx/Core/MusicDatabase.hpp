#pragma once

#include <string>

#include "score2dx/Core/JsonDefinition.hpp"
#include "score2dx/Core/MusicInfo.hpp"

namespace score2dx
{

const std::string Official1stSubVersionName = "1st&substream";

class MusicDatabase
{
public:
        MusicDatabase();

        //! @brief Vector of {Index=VersionIndex, Vector of {Index=MusicIndex, Title}}.
        const std::vector<std::vector<std::string>> &
        GetAllTimeMusics()
        const;

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
    //!     * Db use half width here.
    //!     * Music changed title (official fix) between versions.
        std::optional<std::string>
        FindDbTitle(const std::string &title)
        const;

    //! @brief Find VersionIndex of dbTitle belong to official combined version '1st&substream'.
    //! @return 0 (1st style) or 1 (substream) or nullopt (not found).
        std::optional<std::size_t>
        Find1stSubVersionIndex(const std::string &dbTitle)
        const;

    //! @brief Find music index in version musics.
        std::optional<std::size_t>
        FindMusicIndex(std::size_t versionIndex, const std::string &dbTitle)
        const;

    //! @brief Find Pair of {VersionIndex, MusicIndex} by versionName, and dbTitle.
    //! versionName can be Official1stSubVersionName.
        std::pair<std::size_t, std::size_t>
        FindIndexes(const std::string &versionName, const std::string &dbTitle)
        const;

    //! @brief Get latest music info of music id.
    //! @note Each difficulty can change info or availability at different versionRange.
    //! This function get last available versionRange.
    //! e.g. "5.1.1." SPA "12-29" level: 10, note: 786,
    //!               SPB "cs05, cs07-cs08, cs10, cs12, cs16" level: 1 note: 70
    //!               SPN "12-26" level: 1, note 99 "27-29" level: 2, note 99.
    //! "27-29" is latest info for SPN, so SPN will contain that info.
    //! @note This is not for active version, which requires only chart info available at specific version.
    //! e.g Active version = 26, "5.1.1." not have SPB, SPN level = 1.
        MusicInfo
        GetLatestMusicInfo(std::size_t musicId)
        const;

        bool
        IsCsMusic(std::size_t musicId)
        const;

        const Json*
        FindDbMusic(std::size_t versionIndex, const std::string &title)
        const;

private:
    Json mDatabase;

    //! @brief Vector of {Index=VersionIndex, Vector of {Index=MusicIndex, Title}}.
    std::vector<std::vector<std::string>> mAllTimeMusics;
    //! @brief Cache all 00 and 01 musics to lookup version index.
    //! Map of {DbTitle Version="00" or "01", versionIndex}.
    std::map<std::string, std::size_t> m1stSubVersionIndexMap;
    //! @brief Since index is unchanged after loading, cache all music index.
    //! Map of {VersionIndex, Map of {DbTitle, MusicIndex}}.
    std::map<std::size_t, std::map<std::string, std::size_t>> mVersionMusicIndexMap;
};

}