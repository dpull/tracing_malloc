#pragma once
#include "tracking_malloc.h"
#include <new>

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

template<typename _Tp>
class sys_allocator {
public:
    typedef size_t     size_type;
    typedef ptrdiff_t  difference_type;
    typedef _Tp*       pointer;
    typedef const _Tp* const_pointer;
    typedef _Tp&       reference;
    typedef const _Tp& const_reference;
    typedef _Tp        value_type;

    template<typename _Tp1>
    struct rebind
    {
        typedef sys_allocator<_Tp1> other;
    };

    sys_allocator() throw() { }

    sys_allocator(const sys_allocator& Copy_allocator) throw() { }

    template<typename _Tp1>
    sys_allocator(const sys_allocator<_Tp1>& Related_allocator) throw() { }

    ~sys_allocator() throw() { }

    pointer address(reference __x) const { return &__x; }

    const_pointer address(const_reference __x) const { return &__x; }

    // NB: __n is permitted to be 0.  The C++ standard says nothing
    // about what the return value is when __n == 0.
    pointer allocate(size_type __n, const void* = 0)
    {
        if (__n > max_size())
            throw(std::bad_alloc());
        // allocate storage for _Count elements of type _Ty
        return static_cast<_Tp *>(sys_malloc(__n * sizeof(_Tp)));
    }

    // __p is not permitted to be a null pointer.
    void deallocate(pointer __p, size_type)
    {
        if (!__p)
            return;
        sys_free(__p);
    }

    size_type max_size() const throw()
    {
        return size_t(-1) / sizeof(_Tp);
    }

    // _GLIBCXX_RESOLVE_LIB_DEFECTS
    // 402. wrong new expression in [some_] allocator::construct
    void construct(pointer __p, const _Tp& __val)
    {
        ::new(__p) _Tp(__val);
    }

    void destroy(pointer __p) { __p->~_Tp(); }
};

template<typename _Tp>
inline bool operator==(const sys_allocator<_Tp>&, const sys_allocator<_Tp>&)
{
    return true;
}

template<typename _Tp>
inline bool operator!=(const sys_allocator<_Tp>&, const sys_allocator<_Tp>& )
{
    return false;
}
