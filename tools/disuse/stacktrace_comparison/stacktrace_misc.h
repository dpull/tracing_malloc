#pragma once
#include "stacktrace.h"
#include "stacktrace_backtrace.h"
#include "stacktrace_backtrace_dladdr.h"
#include "stacktrace_boost.h"
#include "stacktrace_gcc_unwind.h"
#include "stacktrace_libunwind.h"
#include <chrono>

enum class stacktrace_type { backtrace_dladdr, backtrace, boost_stacktrace, gcc_unwind, libunwind };

inline stacktrace* create(stacktrace_type type)
{
    switch (type) {
    case stacktrace_type::backtrace_dladdr:
        return new stacktrace_backtrace_dladdr;

    case stacktrace_type::backtrace:
        return new stacktrace_backtrace;

    case stacktrace_type::boost_stacktrace:
        return new stacktrace_backtrace;

    case stacktrace_type::gcc_unwind:
        return new stacktrace_gcc_unwind;

    case stacktrace_type::libunwind:
        return new stacktrace_libunwind;

    default:
        return nullptr;
    }
}

inline long long cost_time_us(std::function<void()> fun)
{
    auto start = std::chrono::system_clock::now();
    fun();
    auto end = std::chrono::system_clock::now();
    auto cost_time = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
    return cost_time.count();
}
