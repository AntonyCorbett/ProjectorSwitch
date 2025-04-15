#pragma once
#include <string>
#include <vector>
#include <uiautomation.h>

struct FindWindowsResult
{
    bool IsRunning;
    bool FoundDesktop;
    bool FoundMediaWindow;
    std::wstring BespokeErrorMsg;
    IUIAutomationElement* Element;

    FindWindowsResult()
        : Element(nullptr)
        , IsRunning(false)
        , FoundDesktop(false)
        , FoundMediaWindow(false)        
    {
    }

    ~FindWindowsResult()
    {
        if (Element != nullptr)
        {
            Element->Release();
            Element = nullptr;
        }
    }
};

