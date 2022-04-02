#include "score2dx/Core/MusicInfo.hpp"

#include <iostream>

#include "ies/StdUtil/Find.hxx"

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
}

void
MusicInfo::
AddField(MusicInfoField field, const std::string &fieldString)
{
    mFields[field] = fieldString;
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
}

}
