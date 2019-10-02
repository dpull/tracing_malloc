#include "tracking_malloc_stacktrace_comparison.h"
#include "tracking_malloc_stacktrace_default.h"
#include "tracking_malloc.hpp"
#include <string.h>
#include <string>
#include <sstream>
#include <chrono>
#include <atomic>

#ifdef ENABLE_BACKTRACE
#include <execinfo.h>
class stacktrace_backtrace : public stacktrace {
public:
    ~stacktrace_backtrace() override
    {
        if (strings) 
            free(strings);
    }
    void collect() override
    {
        size = backtrace(buffer, sizeof(buffer) / sizeof(buffer[0]));
    }
    void analysis() override
    {
        if (!strings)
            strings = backtrace_symbols(buffer, size);
    }
    void output(FILE* stream, int start_level) override
    {
        fprintf(stream, "stacktrace_backtrace:%d\n", size - start_level);
        for (auto i = start_level; i < size; ++i) {
            fprintf(stream, "%s\n", strings[i]);
        }
    }

    void* buffer[STACK_TRACE_DEPTH];
    int size = 0;
    char** strings = nullptr;
};
#endif // ENABLE_BACKTRACE

#ifdef ENABLE_BOOST_STACKTRACE
#include <boost/stacktrace.hpp>
class stacktrace_boost_stacktrace : public stacktrace {
public:
    ~stacktrace_boost_stacktrace() override
    {
        if (stacktrace)
            sys_delete(stacktrace);
    }
    void collect() override
    {
        stacktrace = sys_new<boost::stacktrace::stacktrace>();
    }
    void analysis() override
    {
        if (!stacktrace)
            return;

        std::stringstream str;
        str << *stacktrace;
        stacktrace_str = str.str();

        sys_delete(stacktrace);
        stacktrace = nullptr;
    }
    void output(FILE* stream, int start_level) override
    {
        start_level;
        fprintf(stream, "stacktrace_boost_stacktrace_step\n%s\n", stacktrace_str.c_str());
    }
    boost::stacktrace::stacktrace* stacktrace = nullptr;
    std::string stacktrace_str;
};
#endif // ENABLE_BOOST_STACKTRACE

#ifdef ENABLE_LIBUNWIND
#define UNW_LOCAL_ONLY
#include <libunwind.h>
#include <cxxabi.h>
class stacktrace_libunwind : public stacktrace {
    struct stack_frame {
        unw_word_t reg_ip;
        char symbol[1024];
        unw_word_t offset;
    };
public:
    void collect() override
    {
        unw_cursor_t cursor;
        unw_context_t uc;
        unw_getcontext(&uc);
        unw_init_local(&cursor, &uc);

        stacktrace_len = 0;
        for (auto i = 0; i < sizeof(stacktrace) / sizeof(stacktrace[0]); ++i) {
            if (unw_step(&cursor) <= 0)
                break;

            stack_frame* frame = stacktrace + i;
            unw_get_reg(&cursor, UNW_REG_IP, &frame->reg_ip);

            int status = unw_get_proc_name(&cursor, frame->symbol, sizeof(frame->symbol), &frame->offset);
            if (status == 0) {
                char* demangled = abi::__cxa_demangle(frame->symbol, nullptr, nullptr, &status);
                if (status == 0)
                    strncpy(frame->symbol, demangled, sizeof(frame->symbol));
                if (demangled)
                    free(demangled);
            }
            stacktrace_len++;
        }
    }
    void analysis() override
    {

    }
    void output(FILE* stream, int start_level) override
    {
        fprintf(stream, "stacktrace_libunwind:%d\n", stacktrace_len - start_level);
        for (auto i = start_level; i < stacktrace_len; ++i) {
            stack_frame* frame = stacktrace + i;
            fprintf(stream, "0x%lx(%s+0x%lx)\n", frame->reg_ip, frame->symbol, frame->offset);
        }
    }
    stack_frame stacktrace[STACK_TRACE_DEPTH];
    int stacktrace_len;
};
#endif // ENABLE_LIBUNWIND

#ifdef USE_STACKTRACE_COMPARISON
static std::atomic<long long> collect_cost[comparison_type_total];
static std::atomic<long long> analysis_cost[comparison_type_total];

stacktrace_comparison::stacktrace_comparison()
{
    memset(stacktrace_array, 0, sizeof(stacktrace_array));
    stacktrace_array[comparison_type_default] = sys_new<stacktrace_default>();

#ifdef ENABLE_BACKTRACE
    stacktrace_array[comparison_type_backtrace] = sys_new<stacktrace_backtrace>();
#endif // ENABLE_BACKTRACE

#ifdef ENABLE_BOOST_STACKTRACE
    stacktrace_array[comparison_type_boost_stacktrace] = sys_new<stacktrace_boost_stacktrace>();
#endif // ENABLE_BOOST_STACKTRACE

#ifdef ENABLE_LIBUNWIND
    stacktrace_array[comparison_type_boost_libunwind] = sys_new<stacktrace_libunwind>();
#endif // ENABLE_LIBUNWIND      
}

stacktrace_comparison::~stacktrace_comparison()
{
    for (auto i = 0; i < comparison_type_total; ++i) {
        auto st = stacktrace_array[i];
        if (st)
            sys_delete(st);
    }
}

void stacktrace_comparison::collect() 
{
    for (auto i = 0; i < comparison_type_total; ++i) {
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

void stacktrace_comparison::output(FILE* stream, int start_level)
{
    start_level++;
    for (auto i = 0; i < comparison_type_total; ++i) {
        auto st = stacktrace_array[i];
        if (!st)
            continue;
        st->output(stream, start_level);
    }
}

#endif // USE_STACKTRACE_COMPARISON

void stacktrace_comparison_output() 
{
#ifdef USE_STACKTRACE_COMPARISON
    const char* format = "%-32s\tcollect_cost:[%lld]ms\tanalysis_cost:[%lld]ms\n";
    printf(format, "default", collect_cost[comparison_type_default].load(), analysis_cost[comparison_type_default].load());
    printf(format, "backtrace", collect_cost[comparison_type_backtrace].load(), analysis_cost[comparison_type_backtrace].load());
    printf(format, "boost_stacktrace,", collect_cost[comparison_type_boost_stacktrace].load(), analysis_cost[comparison_type_boost_stacktrace].load());
    printf(format, "libunwind", collect_cost[comparison_type_boost_libunwind].load(), analysis_cost[comparison_type_boost_libunwind].load());
#endif
}

