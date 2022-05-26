#include "score2dx/Iidx/Music.hpp"

#include <iostream>
#include <set>

#include "ies/Common/AdjacentArrayRange.hxx"
#include "ies/Common/IntegralRangeUsing.hpp"
#include "ies/StdUtil/Find.hxx"

#include "score2dx/Iidx/Version.hpp"

namespace score2dx
{

Music::
Music(std::size_t musicId, const std::string &title)
:   mMusicId(musicId)
{
    mMusicInfo.SetField(MusicInfoField::Title, title);

    for (auto styleDifficulty : StyleDifficultySmartEnum::ToRange())
    {
        mChartAvailabilityTable[static_cast<std::size_t>(styleDifficulty)].resize(VersionNames.size());
    }
}

std::size_t
Music::
GetMusicId()
const
{
    return mMusicId;
}

const MusicInfo &
Music::
GetMusicInfo()
const
{
    return mMusicInfo;
}

void
Music::
SetMusicInfoField(MusicInfoField field, const std::string &fieldString)
{
    if (field==MusicInfoField::Title)
    {
        throw std::runtime_error("cannot reset title of music.");
    }

    mMusicInfo.SetField(field, fieldString);
}

void
Music::
AddAvailability(StyleDifficulty styleDifficulty,
                const std::map<std::string, ChartInfo> &chartInfoByChartVersions)
{
    auto styleDifficultyIndex = static_cast<std::size_t>(styleDifficulty);
    auto &verTable = mChartAvailabilityTable[styleDifficultyIndex];
    auto &chartNotes = mChartNoteByIndex[styleDifficultyIndex];
    chartNotes.reserve(chartInfoByChartVersions.size());

    //'' by design chartVersions already sort by first version listed, assume chartVersions don't overlap.

    for (auto &[chartVersions, chartInfo] : chartInfoByChartVersions)
    {
        if (!ies::Find(chartNotes, chartInfo.Note))
        {
            chartNotes.emplace_back(chartInfo.Note);
        }

        std::size_t chartIndex = 0;
        for (auto i : IndexRange{0, chartNotes.size()})
        {
            if (chartNotes[i]==chartInfo.Note)
            {
                chartIndex = i;
                break;
            }
        }

        auto rangeList = ToRangeList(chartVersions);
        for (auto &range : rangeList.GetRanges())
        {
            for (auto versionIndex : range)
            {
                if (versionIndex>=verTable.size())
                {
                    throw std::runtime_error("versionIndex out of bound.");
                }

                auto &availability = verTable[versionIndex];
                availability.ChartAvailableStatus = ChartStatus::Available;
                availability.ChartIndex = chartIndex;
                availability.ChartInfoProp = chartInfo;
            }
        }
    }

    for (auto i : IndexRange{0, verTable.size()-1})
    {
        auto &previousAvailability = verTable[i];
        auto &currentAvailability = verTable[i+1];

        if ((previousAvailability.ChartAvailableStatus==ChartStatus::NotAvailable
                || previousAvailability.ChartAvailableStatus==ChartStatus::Removed)
            &&currentAvailability.ChartAvailableStatus==ChartStatus::Available)
        {
            currentAvailability.ChartAvailableStatus = ChartStatus::BeginAvailable;
        }

        if ((previousAvailability.ChartAvailableStatus==ChartStatus::BeginAvailable
                || previousAvailability.ChartAvailableStatus==ChartStatus::Available
                || previousAvailability.ChartAvailableStatus==ChartStatus::Removed)
            &&currentAvailability.ChartAvailableStatus==ChartStatus::NotAvailable)
        {
            currentAvailability.ChartAvailableStatus = ChartStatus::Removed;
        }
    }
}

const ChartAvailability &
Music::
GetChartAvailability(StyleDifficulty styleDifficulty, std::size_t versionIndex)
const
{
    auto &verTable = mChartAvailabilityTable[static_cast<std::size_t>(styleDifficulty)];
    if (versionIndex>=verTable.size())
    {
        throw std::runtime_error("GetChartAvailability: versionIndex is out of bound.");
    }

    return verTable[versionIndex];
}

std::list<std::size_t>
Music::
FindSameChartVersions(StyleDifficulty styleDifficulty, std::size_t versionIndex)
const
{
    std::list<std::size_t> versions;
    auto &availability = GetChartAvailability(styleDifficulty, versionIndex);
    if (availability.ChartAvailableStatus==ChartStatus::NotAvailable)
    {
        return versions;
    }

    auto &verTable = mChartAvailabilityTable[static_cast<std::size_t>(styleDifficulty)];
    for (auto ver : IndexRange{0, VersionNames.size()})
    {
        if (verTable[ver].ChartIndex==availability.ChartIndex)
        {
            versions.emplace_back(ver);
        }
    }

    return versions;
}

}
