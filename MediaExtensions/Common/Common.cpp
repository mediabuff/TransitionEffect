#include "pch.h"
#include "Common.h"

//////////////////////////////////////////////////////////////////////////

int _dprintf(LPCTSTR fmt, ...)
{
    _TCHAR buffer[4096];
    _TCHAR buffer2[4096];
    va_list marker;
    va_start(marker, fmt);
    int argments = _vstprintf(buffer, fmt, marker);
    _stprintf(buffer2, _T("%s %s"), DBG_MSG_PREFIX_TAG, buffer);
    if (buffer2[_tcslen(buffer2) - 1] != '\n')
        _tcscat(buffer2, _T("\n"));
    OutputDebugString(buffer2);
    va_end(marker);
    return argments;
}

Platform::String ^ GetMonthName(DWORD month)
{
    Platform::String ^ month_string;
    switch (month)
    {
    case 1:
        month_string = L"January";
        break;
    case 2:
        month_string = L"February";
        break;
    case 3:
        month_string = L"March";
        break;
    case 4:
        month_string = L"April";
        break;
    case 5:
        month_string = L"May";
        break;
    case 6:
        month_string = L"June";
        break;
    case 7:
        month_string = L"July";
        break;
    case 8:
        month_string = L"August";
        break;
    case 9:
        month_string = L"September";
        break;
    case 10:
        month_string = L"October";
        break;
    case 11:
        month_string = L"November";
        break;
    case 12:
        month_string = L"December";
        break;
    }

    return month_string;
}