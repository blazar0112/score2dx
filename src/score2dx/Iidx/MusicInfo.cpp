#include "score2dx/Iidx/MusicInfo.hpp"

#include <iostream>

#include "ies/StdUtil/Find.hxx"

namespace score2dx
{

void
MusicInfo::
AddField(MusicInfoField field, const std::string &fieldString)
{
    mFields[static_cast<std::size_t>(field)] = fieldString;
}

const std::string &
MusicInfo::
GetField(MusicInfoField field)
const
{
    return mFields[static_cast<std::size_t>(field)];
}

void
MusicInfo::
Print()
const
{
    for (auto field : MusicInfoFieldSmartEnum::ToRange())
    {
        std::cout << ToString(field)+" ["+GetField(field)+"]\n";
    }
}

}
