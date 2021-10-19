#pragma once

#include <string>

#include "score2dx/Core/JsonDefinition.hpp"
#include "score2dx/Core/MusicInfo.hpp"

namespace score2dx
{

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

    //! @brief Find {VersionIndex, MusicIndex} from officialVersionName and dbTitle.
    //! @note Because official combines database version '1st style' and 'substream' to '1st&substream'.
    //! To know which version title belongs and what's music index, need search version musics in DB.
        std::pair<std::size_t, std::size_t>
        FindIndexes(const std::string &officialVersionName, const std::string &dbTitle)
        const;

        MusicInfo
        GetMusicInfo(std::size_t musicId)
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

    //! @brief Find music index in version musics.
        std::optional<std::size_t>
        FindMusicIndex(std::size_t versionIndex, const std::string &dbTitle)
        const;
};

}
