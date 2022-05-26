#pragma once

#include <array>
#include <string>

#include "ies/Common/SmartEnum.hxx"

#include "score2dx/Iidx/Definition.hpp"

namespace score2dx
{

IES_SMART_ENUM(MusicInfoField,
    Title,
    Genre,
    Artist,
    DisplayTitle
);

class MusicInfo
{
public:
        MusicInfo() = default;

        void
        SetField(MusicInfoField field, const std::string &fieldString);

        const std::string &
        GetField(MusicInfoField field)
        const;

        void
        Print()
        const;

private:
    std::array<std::string, MusicInfoFieldSmartEnum::Size()> mFields;
};

}
