#include "score2dx/Core/MusicInfo.hpp"

#include <iostream>

#include "icl_s2/StdUtil/Find.hxx"

namespace score2dx
{

ChartInfo::
ChartInfo(int level, int note)
:   Level(level),
    Note(note)
{

}

MusicInfo::
MusicInfo(std::size_t musicId)
:   mMusicId(musicId)
{
    for (auto field : MusicInfoFieldSmartEnum::ToRange())
    {
        mFields[field];
    }
    for (auto playStyle : PlayStyleSmartEnum::ToRange())
    {
        mChartInfos[playStyle];
    }
}

void
MusicInfo::
AddField(MusicInfoField field, const std::string &fieldString)
{
    mFields[field] = fieldString;
}

void
MusicInfo::
AddChartInfo(PlayStyle playStyle, Difficulty difficulty, const ChartInfo &chartInfo)
{
    mChartInfos[playStyle][difficulty] = chartInfo;
}

std::size_t
MusicInfo::
GetMusicId()
const
{
    return mMusicId;
}

const std::string &
MusicInfo::
GetField(MusicInfoField field)
const
{
    return mFields.at(field);
}

const ChartInfo*
MusicInfo::
FindChartInfo(PlayStyle playStyle, Difficulty difficulty)
const
{
    auto &styleChartInfos = mChartInfos.at(playStyle);
    auto findDifficulty = icl_s2::Find(styleChartInfos, difficulty);
    if (findDifficulty)
    {
        return &(findDifficulty.value()->second);
    }
    return nullptr;
}

void
MusicInfo::
Print()
const
{
    std::cout << "MusicInfo [" << mMusicId << "]:\n";
    for (auto field : MusicInfoFieldSmartEnum::ToRange())
    {
        std::cout << ToString(field)+" ["+GetField(field)+"]\n";
    }
    for (auto playStyle : PlayStyleSmartEnum::ToRange())
    {
        std::cout << ToString(playStyle)+":\n";
        for (auto &[difficulty, chartInfo] : mChartInfos.at(playStyle))
        {
            std::cout << "    " << ToString(difficulty)
                      << " level: " << chartInfo.Level
                      << " note: " << chartInfo.Note
                      << "\n";
        }
    }
}

}
