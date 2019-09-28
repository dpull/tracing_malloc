#pragma once
#include "tracking_malloc.h"
#include <memory>

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
