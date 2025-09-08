#include <windows.h>
#include <vector>
#include <stdexcept>
#include "MonitorService.h"
#include "MonitorData.h"
#include "DisplayConfigData.h"

namespace
{
    BOOL CALLBACK EnumMonitorProc(HMONITOR monitor, HDC hdc, LPRECT rect, LPARAM lparam)
    {
        UNREFERENCED_PARAMETER(hdc);
        UNREFERENCED_PARAMETER(rect);

        // NOLINTNEXTLINE(performance-no-int-to-ptr)  // Win32 callback: LPARAM round-trips a pointer by API contract
        auto monitors = reinterpret_cast<std::vector<MONITORINFOEX>*>(lparam);

        MONITORINFOEX info{};
        info.cbSize = sizeof(MONITORINFOEX);

        if (::GetMonitorInfo(monitor, &info))
        {
            monitors->push_back(info);
        }

        return TRUE;
    }
}

// ReSharper disable once CppMemberFunctionMayBeStatic
std::vector<MonitorData> MonitorService::GetMonitorsData() const
{
    std::vector<MonitorData> returnData;

    auto monitorInfo = GetMonitorsInfo();
    auto displayInfo = GetDisplayConfigInfo();

    if (monitorInfo.size() != displayInfo.size())
    {
        throw std::runtime_error("Monitor and display config data size mismatch");
    }

    returnData.reserve(monitorInfo.size());
    for (size_t i = 0; i < monitorInfo.size(); ++i)
    {
        returnData.emplace_back(monitorInfo[i], displayInfo[i]);
    }

    return returnData;
}

std::vector<MONITORINFOEX> MonitorService::GetMonitorsInfo()
{
    std::vector<MONITORINFOEX> monitors;

    if (!EnumDisplayMonitors(nullptr, nullptr, EnumMonitorProc, reinterpret_cast<LPARAM>(&monitors)))
    {
        throw std::runtime_error("Failed to enumerate monitors");
    }

    return monitors;
}

std::vector<DisplayConfigData> MonitorService::GetDisplayConfigInfo()
{
    std::vector<DisplayConfigData> returnData;

    std::vector<DISPLAYCONFIG_PATH_INFO> paths;
    std::vector<DISPLAYCONFIG_MODE_INFO> modes;
    UINT32 flags = QDC_ONLY_ACTIVE_PATHS | QDC_VIRTUAL_MODE_AWARE;

	// ReSharper disable once CppInitializedValueIsAlwaysRewritten
    auto result = ERROR_SUCCESS;

    do
    {
        UINT32 pathCount;
        UINT32 modeCount;

        result = GetDisplayConfigBufferSizes(flags, &pathCount, &modeCount);

        if (result != ERROR_SUCCESS)
        {
            throw std::runtime_error("Failed to get display config buffer sizes");
        }

        paths.resize(pathCount);
        modes.resize(modeCount);

        result = QueryDisplayConfig(flags, &pathCount, paths.data(), &modeCount, modes.data(), nullptr);

        paths.resize(pathCount);
        modes.resize(modeCount);
    } while (result == ERROR_INSUFFICIENT_BUFFER);

    if (result != ERROR_SUCCESS)
    {
        throw std::runtime_error("Failed to query display config");
    }

    for (const auto& path : paths)
    {
        DISPLAYCONFIG_TARGET_DEVICE_NAME targetName{};
        targetName.header.adapterId = path.targetInfo.adapterId;
        targetName.header.id = path.targetInfo.id;
        targetName.header.type = DISPLAYCONFIG_DEVICE_INFO_GET_TARGET_NAME;
        targetName.header.size = sizeof(targetName);

        result = DisplayConfigGetDeviceInfo(&targetName.header);
        if (result != ERROR_SUCCESS)
        {
            throw std::runtime_error("Failed to get monitor friendly name");
        }

        DISPLAYCONFIG_ADAPTER_NAME adapterName{};
        adapterName.header.adapterId = path.targetInfo.adapterId;
        adapterName.header.type = DISPLAYCONFIG_DEVICE_INFO_GET_ADAPTER_NAME;
        adapterName.header.size = sizeof(adapterName);

        result = DisplayConfigGetDeviceInfo(&adapterName.header);
        if (result != ERROR_SUCCESS)
        {
            throw std::runtime_error("Failed to get adapter device name");
        }

        returnData.emplace_back(
            path.targetInfo.id,
            targetName.flags.friendlyNameFromEdid ? targetName.monitorFriendlyDeviceName : L"Unknown",
            adapterName.adapterDevicePath);
    }

    return returnData;
}