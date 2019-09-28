#include "tracking_malloc_stacktrace.h"
#include "tracking_malloc.hpp"
#include <string.h>
#include <string>
#include <sstream>
#include <vector>
#include <chrono>

#ifdef USE_BACKTRACE
#include <execinfo.h>
class stacktrace_backtrace : public stacktrace {
public:
    ~stacktrace_backtrace() override
    {
        if (strings) {
            free(strings);
            strings = nullptr;
        }
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
#endif 

#ifdef USE_BOOST_STACKTRACE
#include <boost/stacktrace.hpp>
class stacktrace_boost_stacktrace : public stacktrace {
public:
    void collect() override 
    {
        std::stringstream str;
        str << boost::stacktrace::stacktrace();
        stacktrace = str.str();
    }
    void analysis() override
    {

    }
    void output(FILE* stream, int start_level) override
    {
        start_level;
        fprintf(stream, "stacktrace_backtrace\n%s\n", stacktrace.c_str());
    }
    std::string stacktrace;
};
class stacktrace_boost_stacktrace_step : public stacktrace {
public:
    void collect() override 
    {
        stacktrace = boost::stacktrace::stacktrace();
    }
    void analysis() override
    {
        if (!stacktrace_levels.empty())
            return;

        stacktrace_levels.reserve(STACK_TRACE_DEPTH);
        for (auto& frame: stacktrace) {
            stacktrace_levels.push_back(std::make_pair(frame.address(), frame.source_file()));
            if (stacktrace_levels.size() == STACK_TRACE_DEPTH)
                break;
        }
    }
    void output(FILE* stream, int start_level) override
    {
        auto size = stacktrace_levels.size();
        fprintf(stream, "stacktrace_boost_stacktrace_step:%d\n", size - start_level);
        for (auto i = start_level; i < size; ++i) {
            auto& frame = stacktrace_levels.at(i);
            fprintf(stream, "%p(%s)\n", frame.first, frame.second.c_str());
        }
    }
    boost::stacktrace::stacktrace stacktrace;
    std::vector<std::pair<const void*, std::string>> stacktrace_levels;
};
#endif 

#ifdef USE_LIBUNWIND
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
#endif 

class stacktrace_compare : public stacktrace {
    struct node {
        stacktrace* st;
        std::string name;
        std::chrono::microseconds collect_cost_time;
        std::chrono::microseconds analysis_cost_time;
    };

public:
    stacktrace_compare() 
    {
        node tmp;
#ifdef USE_BACKTRACE
        tmp.st = sys_new<stacktrace_backtrace>();
        tmp.name = "stacktrace_backtrace";
        nodes.push_back(tmp);
#endif 

#ifdef USE_BOOST_STACKTRACE
        tmp.st = sys_new<stacktrace_boost_stacktrace>() ;
        tmp.name = "stacktrace_boost_stacktrace";
        nodes.push_back(tmp);

        tmp.st = sys_new<stacktrace_boost_stacktrace_step>();
        tmp.name = "stacktrace_boost_stacktrace_step";
        nodes.push_back(tmp);
#endif 

#ifdef USE_LIBUNWIND
        tmp.st = sys_new<stacktrace_libunwind>();
        tmp.name = "stacktrace_libunwind";
        nodes.push_back(tmp);
#endif        
    }

    ~stacktrace_compare() override
    {
        for(auto& node : nodes)  {
            printf("%s\tcollect:[%lld]\tanalysis:[%lld]\n", node.name.c_str(), node.collect_cost_time.count(), node.analysis_cost_time.count());
            sys_delete(node.st);
        }
        nodes.clear();
    }

    void collect() override 
    {
        for(auto& node : nodes)  {
            auto start = std::chrono::system_clock::now();
            node.st->collect();
            auto end = std::chrono::system_clock::now();
            node.collect_cost_time += std::chrono::duration_cast<std::chrono::microseconds>(end - start);
        }
    }
    void analysis() override
    {
        for(auto& node : nodes) {
            auto start = std::chrono::system_clock::now();
            node.st->analysis();
            auto end = std::chrono::system_clock::now();
            node.analysis_cost_time += std::chrono::duration_cast<std::chrono::microseconds>(end - start);
        }
    }
    void output(FILE* stream, int start_level) override
    {
        for(auto& node : nodes) {
            node.st->output(stream, start_level);
        }
    }

    std::vector<node> nodes;
};

stacktrace* stacktrace_create() 
{
#ifdef USE_STACKTRACE_COMPARE
    return sys_new<stacktrace_compare>();
#else
    return sys_new<stacktrace_backtrace>();
#endif
}

void stacktrace_destroy(stacktrace* st) 
{
    sys_delete(st);
}
