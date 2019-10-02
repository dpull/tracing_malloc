#pragma once
#include "tracking_malloc.hpp"
#include <map>
#include <list>
#include <thread>
#include <mutex>
#include <time.h>

struct alloc_info {
    size_t alloc_size;
    time_t alloc_time;
    int alloc_stacktrace_analysed;
    struct stacktrace* alloc_stacktrace;
};

struct alloc_opt {
    int opt_type;
    void* ptr;
    alloc_info* info;
};

typedef std::map<void*, alloc_info*, std::less<void*>, sys_allocator<std::pair<void*, alloc_info*> > > alloc_info_table;
typedef std::list<alloc_opt, sys_allocator<alloc_opt>> alloc_opt_list;

class record {
public:
    ~record();
    bool init();
    bool add_alloc(void* ptr, size_t size);
    bool add_free(void* ptr);

private:
    void apply_alloc_opt_list(alloc_opt_list& opt_list);
    void output();
    int work_thread();

    alloc_info_table m_alloc_info_table;
    alloc_opt_list m_alloc_opt_list;
    std::recursive_mutex m_mutex;
    std::thread m_thread;
    int m_exit_flag;
    time_t next_save_time;
};
