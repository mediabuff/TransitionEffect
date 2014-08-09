#include "pch.h"
#include "Common.h"

int _dprintf(LPCTSTR fmt, ...)
{
    TCHAR buffer[4096];
    TCHAR buffer2[4096];
    va_list marker;
    va_start(marker, fmt);
    int argments = vswprintf(buffer, fmt, marker);
    swprintf(buffer2, L"%s %s", DBG_MSG_PREFIX_TAG, buffer);
    if (buffer2[wcslen(buffer2) - 1] != '\n')
        wcscat(buffer2, L"\n");
    OutputDebugString(buffer2);
    va_end(marker);
    return argments;
}