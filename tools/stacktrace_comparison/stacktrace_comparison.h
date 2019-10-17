#pragma once
#include "stacktrace.h"

enum comparison_type {
    comparison_type_backtrace_dladdr,
    comparison_type_backtrace,
    comparison_type_boost_stacktrace,
    comparison_type_gcc_unwind,
    comparison_type_libunwind,
    comparison_type_total,
};

void set_comparison_type(comparison_type type);

class stacktrace_comparison : public stacktrace {
public:
    stacktrace_comparison();
    ~stacktrace_comparison() override;
    void collect() override;
    void analysis() override;
    void output(FILE* stream) override;

    static void output_comparison(FILE* stream);

private:
    stacktrace* stacktrace_array[comparison_type_total];
};
