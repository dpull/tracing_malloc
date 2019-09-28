#include "tracking_malloc.hpp"
#include "tracking_malloc_record.h"
#include "tracking_malloc_stacktrace.h"
#include <chrono>
#include <sys/types.h>
#include <unistd.h>

#define OPT_TYPE_ALLOC  (1)
#define OPT_TYPE_FREE   (2)

static record* g_record = nullptr;
thread_local int record_disable_flag = 0;
thread_local int record_thread_disable_flag = 0;

void alloc_info_destory(alloc_info* info)
{
    stacktrace_destroy(info->alloc_stacktrace);
    sys_delete(info);
}

record::~record() 
{
	m_exit_flag = true;
	m_thread.join();
}

int record::init()
{
	m_exit_flag = false;
    next_save_time = 0;
	m_thread = std::thread(&record::work_thread, this);
	return 0;
}

int record::work_thread()
{
    time_t now;
	alloc_opt_list swap_list;

    record_thread_disable_flag = 1;
	while (!m_exit_flag) {
		{
	        std::lock_guard<decltype(m_mutex)> lock(m_mutex);
			swap_list.swap(m_alloc_opt_list);
		}

		apply_alloc_opt_list(swap_list);
		swap_list.clear();

        now = time(NULL);
        if (now > next_save_time) {
            next_save_time = now + 120;
            output();
        }

        std::this_thread::sleep_for(std::chrono::seconds(1));
	}

    apply_alloc_opt_list(m_alloc_opt_list);
    output();

	for (auto& it : m_alloc_info_table) 
		alloc_info_destory(it.second);

    m_alloc_opt_list.clear();
	m_alloc_info_table.clear();
	return 0;
}

void record::output()
{
    char file_name[FILENAME_MAX];
    sprintf(file_name, "/tmp/%s.%d", "tracking.malloc", getpid());

	FILE* stream = fopen(file_name, "w");

	for (auto& it : m_alloc_info_table) {
        alloc_info* info = it.second;
        fprintf(stream, "time:%d\tsize:%d\tptr:%p\n", info->alloc_time, info->alloc_size, it.first);
        info->alloc_stacktrace->analysis();
        info->alloc_stacktrace->output(stream, 2);
	}

    fclose(stream);
}

int record::apply_alloc_opt_list(alloc_opt_list& opt_list)
{
	for (auto& opt : opt_list){
		switch (opt.opt_type){
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
	return 0;
}

int record::add_alloc(void* ptr, size_t size)
{
	alloc_opt opt;
	opt.opt_type = OPT_TYPE_ALLOC;
	opt.ptr = ptr;
	opt.info = sys_new<alloc_info>();
    opt.info->alloc_size = size;
    opt.info->alloc_time = time(NULL);
    opt.info->alloc_stacktrace = stacktrace_create();
    opt.info->alloc_stacktrace->collect();

    std::lock_guard<decltype(m_mutex)> lock(m_mutex);
    m_alloc_opt_list.push_back(opt);
	return 1;
}

int record::add_free(void* ptr)
{
	alloc_opt opt;
	opt.opt_type = OPT_TYPE_FREE;
	opt.ptr = ptr;
	opt.info = nullptr;

    std::lock_guard<decltype(m_mutex)> lock(m_mutex);
    m_alloc_opt_list.push_back(opt);
	return 1;
}

extern "C" int record_init()
{
    record_disable_flag = 1;
	g_record = sys_new<record>();
    g_record->init();
    record_disable_flag = 0;
    return 0;
}

extern "C" int record_uninit()
{
    record_disable_flag = 1;
	sys_delete(g_record);
    record_disable_flag = 0;
	return 0;
}

extern "C" int record_alloc(void* ptr, size_t size)
{
    if (record_disable_flag || record_thread_disable_flag)
	    return 0;
        
    record_disable_flag = 1;
	g_record->add_alloc(ptr, size);
    record_disable_flag = 0;
	return 0;
}

extern "C" int record_free(void* ptr)
{
    if (!ptr || record_disable_flag || record_thread_disable_flag)
	    return 0;

    record_disable_flag = 1;
	g_record->add_free(ptr);
    record_disable_flag = 0;
	return 0;
}