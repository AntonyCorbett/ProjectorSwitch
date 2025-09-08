#pragma once
#include <windows.h>

struct HandleDeleter
{
    void operator()(HANDLE handle) const
    {
        if (handle != INVALID_HANDLE_VALUE && handle != nullptr)
        {
            CloseHandle(handle);
        }
    }
};