#pragma once
#include <string>
#include <uiautomation.h>

struct FindWindowsResult  // NOLINT(cppcoreguidelines-special-member-functions)
{
    IUIAutomationElement* Element;
    bool IsRunning;
    bool FoundDesktop;
    bool FoundMediaWindow;
    std::wstring BespokeErrorMsg;
    
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

