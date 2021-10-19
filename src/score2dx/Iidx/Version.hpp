#pragma once

#include <map>
#include <optional>
#include <string>
#include <vector>

#include "icl_s2/Common/RangeSide.hpp"

namespace score2dx
{

//! @brief Convert versionIndex(size_t) to 2-size version string padded with 0.
//! e.g. versionIndex=4, versionString = "04".
std::string
ToVersionString(std::size_t versionIndex);

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

//! @brief Find VersionIndex of database VersionName.
//! e.g. "1st style" => 0, "substream" => 1, "2nd style" => 2.
std::optional<std::size_t>
FindVersionIndex(const std::string &dbVersionName);

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
std::map<icl_s2::RangeSide, std::string>
GetVersionDateTimeRange(std::size_t versionIndex);

}
