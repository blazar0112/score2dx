#pragma once

#include <array>
#include <map>
#include <set>

#include "score2dx/Iidx/ChartInfo.hpp"
#include "score2dx/Iidx/Definition.hpp"
#include "score2dx/Iidx/MusicInfo.hpp"

namespace score2dx
{

int
CheckChromeDriverVersion(const std::string &driverPath);

int
CheckChromeBrowserVersion();

}
