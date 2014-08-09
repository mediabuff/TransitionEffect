#pragma once

#define RET_IFFAIL(expr) if((hr = (expr)) != S_OK) { return; }
#define RETFALSE_IFFAIL(expr) if((hr = (expr)) != S_OK) { return false; }

#define RET_IFFALSE(expr) if(!(expr)) { return; }
#define RETFALSE_IFFALSE(expr) if(!(expr)) { return false; }

template<typename T> inline T math_lerp(const T& a, const T& b, float k)
{
    assert(k >= 0.0f && k <= 1.0f);

    return a + (b - a) * k;
}

template<typename T> inline T math_clamp(const T& low, const T& value, const T& high)
{
    return value <= low ? low : (value >= high ? high : value);
}

template <typename T> inline T math_max(const T& a, const T& b)
{
    return a > b ? a : b;
}

template <typename T> inline T math_min(const T& a, const T& b)
{
    return a < b ? a : b;
}

inline void ThrowIfError(HRESULT hr)
{
    if (FAILED(hr))
    {
        throw ref new Platform::COMException(hr);
    }
}

inline void ThrowException(HRESULT hr)
{
    assert(FAILED(hr));
    throw ref new Platform::COMException(hr);
}

template <class T> void SafeRelease(T **ppT)
{
    if (*ppT)
    {
        (*ppT)->Release();
        *ppT = NULL;
    }
}

#define DBG_MSG_PREFIX_TAG  (L"[CAdvanceMediaSource]")

#ifndef _DPRINTF
#if defined(_DEBUG) || (_FOR_TEST == 1)
#pragma message("Warning! _DPRINTF is defined!")
#define _DPRINTF(argument)  _dprintf##argument
#else
#define _DPRINTF(argument)
#endif
#endif

int _dprintf(LPCTSTR fmt, ...);