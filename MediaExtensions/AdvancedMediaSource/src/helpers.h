#pragma once

#include "pch.h"

#define RET_IFFAIL(expr) if((expr) != S_OK) { return; }
#define RETFALSE_IFFAIL(expr) if((expr) != S_OK) { return false; }

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
