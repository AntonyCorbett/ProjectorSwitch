#pragma once
#include <string>

struct DisplayWindowResult
{
    bool AllOk;
    std::wstring ErrorMessage;

    DisplayWindowResult()
        : AllOk(false)        
    {
    }
};
