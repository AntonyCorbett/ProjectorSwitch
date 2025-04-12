#pragma once
#include <string>
#include <vector>
#include <uiautomation.h>

struct FindWindowsResult
{
    bool IsRunning = false;
    bool FoundDesktop = false;
    bool FoundMediaWindow = false;
    std::wstring BespokeErrorMsg;
    IUIAutomationElement* Element;

    FindWindowsResult()
        : Element(nullptr)
    {

    }

    ~FindWindowsResult()
    {
        if (Element != nullptr)
        {
            Element->Release();
        }
    }
};

