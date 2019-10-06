#include "tracking_malloc.hpp"
#include "tracking_malloc_record.h"
#include "tracking_malloc_stacktrace.h"
#include <chrono>
#include <sys/types.h>
#include <unistd.h>
#include <assert.h>

#define OPT_TYPE_ALLOC      (1)
#define OPT_TYPE_FREE       (2)

static tracking_malloc_record* g_record = nullptr;
thread_local int record_disable_flag = 0;
thread_local int record_thread_disable_flag = 0;

tracking_malloc_record::~tracking_malloc_record()
{
    m_exit_flag = true;
    m_thread.join();
}

bool tracking_malloc_record::init()
{
    m_exit_flag = false;
    next_save_time = time(NULL) + STACK_RECORD_INTERVAL_SEC;
    m_alloc_opt_queue.reserve(STACK_RECORD_OPT_QUEUE_RESERVE);
    m_thread = std::thread(&tracking_malloc_record::work_thread, this);
    return true;
}

bool tracking_malloc_record::add_alloc(void* ptr, size_t size)
{
    auto alloc_stacktrace = stacktrace_create(STACK_TRACE_SKIP, STACK_TRACE_DEPTH);
    if (!alloc_stacktrace)
        return false;

    tracking_malloc_alloc_opt opt;
    opt.opt_type = OPT_TYPE_ALLOC;
    opt.ptr = ptr;
    opt.info = sys_new<tracking_malloc_alloc_info>();
    opt.info->alloc_size = size;
    opt.info->alloc_time = time(NULL);
    opt.info->alloc_stacktrace_analysed = false;
    opt.info->alloc_stacktrace = alloc_stacktrace;

    std::lock_guard<decltype(m_mutex)> lock(m_mutex);
    m_alloc_opt_queue.push_back(opt);
    return true;
}

bool tracking_malloc_record::add_free(void* ptr)
{
    tracking_malloc_alloc_opt opt;
    opt.opt_type = OPT_TYPE_FREE;
    opt.ptr = ptr;
    opt.info = nullptr;

    std::lock_guard<decltype(m_mutex)> lock(m_mutex);
    m_alloc_opt_queue.push_back(opt);
    return true;
}

int tracking_malloc_record::work_thread()
{
    time_t now;
    decltype(m_alloc_opt_queue) swap_queue;
    swap_queue.reserve(m_alloc_opt_queue.capacity());

    record_thread_disable_flag = 1;
    while (!m_exit_flag) {
        {
            std::lock_guard<decltype(m_mutex)> lock(m_mutex);
            swap_queue.swap(m_alloc_opt_queue);
        }

        alloc_opt_queue_apply(swap_queue);

        now = time(NULL);
        if (now > next_save_time) {
            next_save_time = now + STACK_RECORD_INTERVAL_SEC;
            output("interval");
        }

        std::this_thread::sleep_for(std::chrono::seconds(1));
    }

    alloc_opt_queue_apply(m_alloc_opt_queue);
    output("exit");

    for (auto& it : m_alloc_info_table)
        alloc_info_destory(it.second);

    assert(m_alloc_opt_queue.empty());
    return 0;
}

void tracking_malloc_record::output(const char* suffix)
{
    char file_name[FILENAME_MAX];
    sprintf(file_name, "/tmp/%s.%d.%s", "tracking.malloc", getpid(), suffix);

    FILE* stream = fopen(file_name, "w");

    for (auto& it : m_alloc_info_table) {
        tracking_malloc_alloc_info* info = it.second;
        alloc_info_analyse(info);

        fprintf(stream, "time:%d\tsize:%d\tptr:%p\n", info->alloc_time, info->alloc_size, it.first);
        for (int i = 0; i < info->alloc_stacktrace->count; ++i) {
            stacktrace_frame* frame = info->alloc_stacktrace->frames + i;
            fprintf(stream, "%p\t%p\t%s\n", frame->address, frame->fbase, frame->fname);
        }
        fprintf(stream, "========\n\n");
    }

    fclose(stream);
}

void tracking_malloc_record::alloc_opt_queue_apply(tracking_malloc_alloc_opt_queue& opt_queue)
{
    for (auto& opt : opt_queue) {
        switch (opt.opt_type) {
        case OPT_TYPE_ALLOC:
            m_alloc_info_table.insert(std::make_pair(opt.ptr, opt.info));
            break;

        case OPT_TYPE_FREE:
            auto it = m_alloc_info_table.find(opt.ptr);
            if (it != m_alloc_info_table.end()) {
                alloc_info_destory(it->second);
                m_alloc_info_table.erase(it);
            }
            break;
        }
    }
    opt_queue.clear();
}

void tracking_malloc_record::alloc_info_destory(tracking_malloc_alloc_info* info)
{
    stacktrace_destroy(info->alloc_stacktrace);
    sys_delete(info);
}

void tracking_malloc_record::alloc_info_analyse(tracking_malloc_alloc_info* info)
{
    if (info->alloc_stacktrace_analysed) 
        return;

    stacktrace_analysis(info->alloc_stacktrace, [](const void* fbase, const char* fname){
        auto& cache = g_record->m_fname_cache;
        auto it = cache.find(fbase);
        if (it == cache.end()) {
            auto insert_it = cache.insert(std::make_pair(fbase, sys_string(fname)));
            it = insert_it.first;
        }
        return it->second.c_str();
    });
    info->alloc_stacktrace_analysed = true;
}

int record_init()
{
    record_disable_flag = 1;
    g_record = sys_new<tracking_malloc_record>();
    g_record->init();
    record_disable_flag = 0;
    return 0;
}

int record_uninit()
{
    record_disable_flag = 1;
    sys_delete(g_record);
    record_disable_flag = 0;
    return 0;
}

int record_alloc(void* ptr, size_t size)
{
    if (!ptr || record_disable_flag || record_thread_disable_flag)
        return 0;

    record_disable_flag = 1;
    g_record->add_alloc(ptr, size);
    record_disable_flag = 0;
    return 0;
}

int record_free(void* ptr)
{
    if (!ptr || record_disable_flag || record_thread_disable_flag)
        return 0;

    record_disable_flag = 1;
    g_record->add_free(ptr);
    record_disable_flag = 0;
    return 0;
}
