#pragma once
#include "stacktrace.h"
#include <boost/stacktrace.hpp>
#include <string>

class stacktrace_boost : public stacktrace {
public:
    ~stacktrace_boost() override;
    void collect() override;
    void analysis() override;
    void output(FILE* stream) override;

private:
    boost::stacktrace::stacktrace* stacktrace = nullptr;
    std::string stacktrace_str;
};