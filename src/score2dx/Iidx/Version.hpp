#pragma once

#include <map>
#include <optional>
#include <string>
#include <vector>

#include "ies/Common/RangeSide.hpp"
#include "ies/Common/SmartEnum.hxx"

namespace score2dx
{

IES_SMART_ENUM(VersionDateType,
    None,
    VersionBegin,
    VersionEnd
);

//! @brief Convert versionIndex(size_t) to 2-size version string padded with 0.
//! e.g. versionIndex=4, versionString = "04".
std::string
ToVersionString(std::size_t versionIndex);

//! @brief Check if version string format and value is valid (i.e. "00" to latest "29").
bool
IsValidVersion(const std::string &version);

//! @brief Return latest version index, i.e. max index of VersionNames.
std::size_t
GetLatestVersionIndex();

//! @brief Names of each version. Vector index same as VersionIndex.
const std::vector<std::string> VersionNames
{
    "1st style",
    "substream",
    "2nd style",
    "3rd style",
    "4th style",
    "5th style",
    "6th style",
    "7th style",
    "8th style",
    "9th style",
    "10th style",
    "IIDX RED",
    "HAPPY SKY",
    "DistorteD",
    "GOLD",
    "DJ TROOPERS",
    "EMPRESS",
    "SIRIUS",
    "Resort Anthem",
    "Lincle",
    "tricoro",
    "SPADA",
    "PENDUAL",
    "copula",
    "SINOBUZ",
    "CANNON BALLERS",
    "Rootage",
    "HEROIC VERSE",
    "BISTROVER",
    "CastHour"
};

const std::string Official1stSubVersionName = "1st&substream";

//! @brief Find VersionIndex of database VersionName.
//! e.g. "1st style" => 0, "substream" => 1, "2nd style" => 2.
std::optional<std::size_t>
FindVersionIndex(const std::string &dbVersionName);

//! @brief Get first version with date time range implemented.
//! @note Currently it's 17.
std::size_t
GetFirstDateTimeAvailableVersionIndex();

//! @brief Get Version's DateTime range [BeginDateTime, EndDateTime] (including end).
//! @note Empty string if DateTime is unknown.
//! DateTime is assigned using release day of each version from bemani wiki.
//! e.g. BeginDateTime = 0:00 of new version release day
//!      EndDateTime = 23:59 of day before next version release day
//! For example:
//! IIDX 26 Rootage:        Released at 2018-11-07, service end at 2020-01-14
//! IIDX 27 HEROIC VERSION:             2019-10-16,                2021-01-12
//! Rootage's DateTime range is [ BeginDateTime "2018-11-07 00:00", EndDateTime "2019-10-15 23:59" ]
//!
//! @note Update process was manually/online installed during release day
//! it is possible have older version data after new version release day
//! (can also cause by delaying inherit data, playing older machine)
//!
//! Since score2dx does not record each score's origin version, mixing score
//! should not be a problem.
//!
//! @note Current version, IIDX 29's EndDateTime is empty.
//!
std::map<ies::RangeSide, std::string>
GetVersionDateTimeRange(std::size_t versionIndex);

//! @brief Find version index from date time. Possible value range: [17, latest].
//! @return std::nullopt if date time before version 17.
//! @note Minimum is 17, since date time before 17 is not implemented.
std::optional<std::size_t>
FindVersionIndexFromDateTime(const std::string &dateTime);

//! @return None if dateTime is out of range (before 17).
VersionDateType
FindVersionDateType(const std::string &dateTime);

}
