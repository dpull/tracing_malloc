#include "stacktrace_comparison.h"
#include "stacktrace_backtrace_dladdr.h"
#include "stacktrace_backtrace.h"
#include "stacktrace_boost.h"
#include "stacktrace_gcc_unwind.h"
#include "stacktrace_libunwind.h"
#include <string.h>
#include <string>
#include <chrono>
#include <atomic>

static std::atomic<long long> collect_cost[comparison_type_total];
static std::atomic<long long> analysis_cost[comparison_type_total];
static comparison_type cur_comparison_type = comparison_type_total;

void set_comparison_type(comparison_type type)
{
    cur_comparison_type = type;
}

stacktrace_comparison::stacktrace_comparison()
{
    memset(stacktrace_array, 0, sizeof(stacktrace_array));
    stacktrace_array[comparison_type_backtrace_dladdr] = new stacktrace_backtrace_dladdr;
    stacktrace_array[comparison_type_backtrace] = new stacktrace_backtrace;
    stacktrace_array[comparison_type_boost_stacktrace] = new stacktrace_boost;
    stacktrace_array[comparison_type_gcc_unwind] = new stacktrace_gcc_unwind;
    stacktrace_array[comparison_type_libunwind] = new stacktrace_libunwind;
}

stacktrace_comparison::~stacktrace_comparison()
{
    for (auto i = 0; i < comparison_type_total; ++i) {
        auto st = stacktrace_array[i];
        if (st)
            delete(st);
    }
}

void stacktrace_comparison::collect() 
{
    for (auto i = 0; i < comparison_type_total; ++i) {
        if (i != cur_comparison_type)
            continue;

        auto st = stacktrace_array[i];
        if (!st)
            continue;
        auto start = std::chrono::system_clock::now();
        st->collect();
        auto end = std::chrono::system_clock::now();

        auto cost_time = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
        collect_cost[i].fetch_add(cost_time.count());
    }
}

void stacktrace_comparison::analysis()
{
    for (auto i = 0; i < comparison_type_total; ++i) {
        if (i != cur_comparison_type)
            continue;

        auto st = stacktrace_array[i];
        if (!st)
            continue;
        auto start = std::chrono::system_clock::now();
        st->analysis();
        auto end = std::chrono::system_clock::now();

        auto cost_time = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
        analysis_cost[i].fetch_add(cost_time.count());
    }
}

void stacktrace_comparison::output(FILE* stream)
{
    for (auto i = 0; i < comparison_type_total; ++i) {
        if (i != cur_comparison_type)
            continue;

        auto st = stacktrace_array[i];
        if (!st)
            continue;
        st->output(stream);
        fprintf(stream, "\n");
    }
}

void stacktrace_comparison::output_comparison(FILE* stream) 
{
    const char* format = "%-16s\tcollect_cost:[%lld]µs\tanalysis_cost:[%lld]µs\n";
    fprintf(stream, "stacktrace_comparison:\n");
    fprintf(stream, format, "backtrace_dladdr", collect_cost[comparison_type_backtrace_dladdr].load(), analysis_cost[comparison_type_backtrace_dladdr].load());
    fprintf(stream, format, "backtrace", collect_cost[comparison_type_backtrace].load(), analysis_cost[comparison_type_backtrace].load());
    fprintf(stream, format, "boost_stacktrace", collect_cost[comparison_type_boost_stacktrace].load(), analysis_cost[comparison_type_boost_stacktrace].load());
    fprintf(stream, format, "gccunwind", collect_cost[comparison_type_gcc_unwind].load(), analysis_cost[comparison_type_gcc_unwind].load());
    fprintf(stream, format, "libunwind", collect_cost[comparison_type_libunwind].load(), analysis_cost[comparison_type_libunwind].load());
}

