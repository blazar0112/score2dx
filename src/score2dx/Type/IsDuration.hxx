#pragma once

#include <chrono>

namespace score2dx::Type
{

//! @note Cannot use IsAssociativeContainer because nlohmann::json does not define json::key_type, but in json::object_t::key_type.
template <typename T>
struct IsDuration : std::false_type {};

template <typename Rep, typename Period>
struct IsDuration<std::chrono::duration<Rep, Period>> : std::true_type {};

template <typename T>
inline constexpr bool IsDurationV = IsDuration<T>::value;

}
