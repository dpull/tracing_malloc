#pragma once

struct stacktrace {
    virtual ~stacktrace();
    virtual void collect();
    virtual void analysis();
    virtual void output(FILE* stream, int start_level);
}

stacktrace* stacktrace_create();
void stacktrace_destroy(stacktrace* st);