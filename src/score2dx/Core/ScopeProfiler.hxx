#pragma once

#include "ies/Time/TimeUtilFormat.hxx"

#include "score2dx/Type/IsDuration.hxx"

namespace score2dx
{

//! @brief Profile time in scope and report at end of scope.
template <typename T>
class ScopeProfiler
{
public:
        ScopeProfiler(std::string timeName)
        :   mTimeName(timeName)
        {
            static_assert(Type::IsDurationV<T>, "type can only be duration");
            mBegin = ies::Time::Now();
        }

        ~ScopeProfiler()
        {
            ies::Time::Print<T>(ies::Time::CountNs(mBegin), mTimeName);
        }

private:
    std::string mTimeName;
    ies::Time::TimePointType mBegin;
};

}
