#pragma once
#include <windows.h>
#include <string>
#include "DisplayConfigData.h"

struct MonitorData
{
    UINT Id = 0;
    bool IsPrimary = false;
    RECT MonitorRect{};
    RECT WorkRect{};
    std::wstring FriendlyName;
    std::wstring DevicePath;
    std::wstring DeviceName;

    MonitorData() = default;

    MonitorData(const MONITORINFOEX& monitorInfoEx, const DisplayConfigData& displayConfigData)
        : Id(displayConfigData.Id)
        , IsPrimary((monitorInfoEx.dwFlags& MONITORINFOF_PRIMARY) != 0)
        , MonitorRect(monitorInfoEx.rcMonitor)
        , WorkRect(monitorInfoEx.rcWork)
        , FriendlyName(displayConfigData.FriendlyName)
        , DevicePath(displayConfigData.DevicePath)
        , DeviceName(monitorInfoEx.szDevice)
    {
    }
};
