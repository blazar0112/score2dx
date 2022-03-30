#pragma once

#include <charconv>
#include <stdexcept>
#include <string>
#include <string_view>

namespace score2dx
{

//! @brief Use from_chars to parse value directly from string_view, throws if error.
template <typename T>
void
CheckedParse(std::string_view valueView, T &value, const std::string &valueName)
{
    auto result = std::from_chars(valueView.data(), valueView.data()+valueView.size(), value);
    if (result.ec!=std::errc())
    {
        throw std::runtime_error("cannot parse value ["+valueName+"].");
    }
}

}
