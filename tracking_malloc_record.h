#pragma once
#include ""
#include "tracking_malloc.h"
#include <map>
#include <list>
#include <thread>
#include <mutex>
#include <time.h>

template<class T>
class sys_allocator : public std::allocator<T> {
public:
	T* allocate(size_t n, void const*) 
    {
		return (T*)sys_malloc(n);
	}

	void deallocate(T* p, size_t n) 
    {
		sys_free(p);
	}
};

template<class T>
inline T* sys_new() 
{
	void* ptr = sys_malloc((sizeof(T)));
	return new(ptr) T;
}

template<class T>
inline void sys_delete(T* ptr) 
{
	ptr->~T();
	sys_free(ptr);
}

struct alloc_info {
	size_t alloc_size;
	time_t alloc_time;
    stacktrace* alloc_stacktrace;
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
	int init();
	int add_alloc(void* ptr, size_t size);
	int add_free(void* ptr);
	
private:
	int apply_alloc_opt_list(alloc_opt_list& opt_list);
	void output();
	int work_thread();

	alloc_info_table m_alloc_info_table;
	alloc_opt_list m_alloc_opt_list;
	std::recursive_mutex m_mutex;
	std::thread m_thread;
	int m_exit_flag;
	time_t next_save_time;
};
