#pragma once

#include "windows.h"
#include <string>
#include <time.h>

#include <intsafe.h>
#define EXPORT

#define _FOR_TEST 1

#ifndef max
    #define max(a, b) (((a) > (b)) ? (a) : (b))
#endif

#ifndef min
    #define min(a, b) (((a) < (b)) ? (a) : (b))
#endif

#define CHECK_NULL_POINTER(ptr)               { if (!ptr) throw std::bad_alloc(); }

#define _ALIGNED_FREE_PTR(ptr)  {_aligned_free(ptr), ptr = NULL;}
#define _ALIGNED_MALLOC_PTR(ptr, type, size)  {_ALIGNED_FREE_PTR(ptr); ptr = (type *)_aligned_malloc(sizeof(type) * (size), 32); CHECK_NULL_POINTER(ptr);}

#define _NEW_PTRS(ptr, type, size)  {_DELETE_PTRS(ptr); try { ptr = new type[size]; } catch (std::bad_alloc&) {} CHECK_NULL_POINTER(ptr);}
#define _DELETE_PTRS(ptr)  {delete[] ptr, ptr = NULL;}

#define ALIGN(x, y)  (((x) + (y) - 1) & ~((y) - 1))

#define CLAMP(val, ubound, lbound)  ((val) > (ubound) ? (ubound) : ((val) < (lbound) ? (lbound) : (val)))

#define FORCE_FALSE (-1 == __LINE__)

__forceinline int round_to_int(float x)
{
    return (int)((x < 0) ? (x - 0.5f) : (x + 0.5f));
}

__forceinline int round_to_int(double x)
{
    return (int)((x < 0) ? (x - 0.5) : (x + 0.5));
}

#define __align16  __declspec(align(16))

#define DBG_MSG_PREFIX_TAG  _T("[EffectTransform]")

#ifndef _DPRINTF
    #if defined(_DEBUG) || (_FOR_TEST == 1)
        #pragma message("Warning! _DPRINTF is defined!")
        #define _DPRINTF(argument)  _dprintf##argument
    #else
        #define _DPRINTF(argument)
    #endif
#endif

#ifndef _MYASSERT
    #if defined(_DEBUG)  || (_FOR_TEST == 1) // for debug purpose
        #pragma message("Warning! _MYASSERT is defined!")
        #define _MYASSERT(argument) \
            do \
            { \
                if (!(argument)) \
                { \
                    fprintf(stderr, "_MYASSERT fail %s line %d", __FILE__, __LINE__); \
                    _DPRINTF((L"_MYASSERT fail %s line %d", __FILE__, __LINE__)); \
                    exit(-1); \
                } \
            } while(0, 0)
    #else                   // for release mode
        #define _MYASSERT(argument)  ((void)0)
    #endif
#endif

//////////////////////////////////////////////////////////////////////////
int _dprintf(LPCTSTR fmt, ...);

Platform::String ^ GetMonthName(DWORD month);