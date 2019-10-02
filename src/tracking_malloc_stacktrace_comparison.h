#pragma once
#include "tracking_malloc_stacktrace.h"

#ifdef USE_STACKTRACE_COMPARISON

#define ENABLE_BACKTRACE
#define ENABLE_BOOST_STACKTRACE
#define ENABLE_LIBUNWIND

enum comparison_type {
    comparison_type_default,
    comparison_type_backtrace,
    comparison_type_boost_stacktrace,
    comparison_type_boost_libunwind,
    comparison_type_total,
};

class stacktrace_comparison : public stacktrace {
public:
    stacktrace_comparison();
    ~stacktrace_comparison() override;
    void collect() override;
    void analysis() override;
    void output(FILE* stream, int start_level) override;

private:
    stacktrace* stacktrace_array[comparison_type_total];
};
#endif // USE_STACKTRACE_COMPARISON


