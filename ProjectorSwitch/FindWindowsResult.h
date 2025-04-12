#pragma once
#include <string>
#include <vector>
#include <uiautomation.h>

struct FindWindowsResult
{
    bool IsRunning = false;
    bool FoundDesktop = false;
    bool FoundMediaWindow = false;
    std::wstring BespokeErrorMsg = L"";
    std::vector<IUIAutomationElement*> Elements;

    ~FindWindowsResult() 
    {
        for (auto element : Elements) 
        {
            if (element != nullptr) 
            {
                element->Release();
            }
        }
        Elements.clear();
    }
};

