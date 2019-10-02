#include "stacktrace_boost.h"
#include <sstream>

stacktrace_boost::~stacktrace_boost()
{
    if (stacktrace)
        delete(stacktrace);
}

void stacktrace_boost::collect()
{
    stacktrace = new boost::stacktrace::stacktrace(0, STACK_TRACE_DEPTH);
}

void stacktrace_boost::analysis()
{
    std::stringstream str;
    str << *stacktrace;
    stacktrace_str = str.str();
}

void stacktrace_boost::output(FILE* stream) 
{
    fprintf(stream, "stacktrace_boost\n%s\n", stacktrace_str.c_str());
}
