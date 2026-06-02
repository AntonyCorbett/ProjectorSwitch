#pragma once
#include <string>
#include <uiautomation.h>

struct FindWindowsResult  // NOLINT(clang-diagnostic-padded)
{
    std::wstring BespokeErrorMsg;
    IUIAutomationElement* Element;
    bool IsRunning;
    bool FoundDesktop;
    bool FoundMediaWindow;

    FindWindowsResult()
        : Element(nullptr)
        , IsRunning(false)
        , FoundDesktop(false)
        , FoundMediaWindow(false)
    {
    }

    FindWindowsResult(const FindWindowsResult&) = delete;
    FindWindowsResult& operator=(const FindWindowsResult&) = delete;

    FindWindowsResult(FindWindowsResult&& other) noexcept
        : BespokeErrorMsg(std::move(other.BespokeErrorMsg))
        , Element(other.Element)
        , IsRunning(other.IsRunning)
        , FoundDesktop(other.FoundDesktop)
        , FoundMediaWindow(other.FoundMediaWindow)
    {
        other.Element = nullptr;
    }

    FindWindowsResult& operator=(FindWindowsResult&& other) noexcept
    {
        if (this != &other)
        {
            if (Element != nullptr)
                Element->Release();
            BespokeErrorMsg = std::move(other.BespokeErrorMsg);
            Element = other.Element;
            IsRunning = other.IsRunning;
            FoundDesktop = other.FoundDesktop;
            FoundMediaWindow = other.FoundMediaWindow;
            other.Element = nullptr;
        }
        return *this;
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

