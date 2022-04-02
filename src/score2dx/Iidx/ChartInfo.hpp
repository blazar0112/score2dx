#pragma once

namespace score2dx
{

struct ChartInfo
{
    int Level{0};
    int Note{0};

        ChartInfo(int level, int note);

        auto operator<=>(const ChartInfo&) const = default;
};

}
