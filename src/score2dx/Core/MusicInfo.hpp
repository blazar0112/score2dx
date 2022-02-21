#pragma once

#include <map>

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

struct ChartInfo
{
    int Level{0};
    int Note{0};

        ChartInfo(int level=0, int note=0);
};

class MusicInfo
{
public:
        explicit MusicInfo(std::size_t musicId);

        void
        AddField(MusicInfoField field, const std::string &fieldString);

        void
        AddChartInfo(PlayStyle playStyle, Difficulty difficulty, const ChartInfo &chartInfo);

        std::size_t
        GetMusicId()
        const;

        const std::string &
        GetField(MusicInfoField field)
        const;

        const ChartInfo*
        FindChartInfo(PlayStyle playStyle, Difficulty difficulty)
        const;

        void
        Print()
        const;

private:
    std::size_t mMusicId;
    //! @brief Map of {Field, FieldString}.
    std::map<MusicInfoField, std::string> mFields;
    //! @brief Map of {PlayStyle, Map of {Difficulty, ChartInfo}}.
    std::map<PlayStyle, std::map<Difficulty, ChartInfo>> mChartInfos;
};

}
