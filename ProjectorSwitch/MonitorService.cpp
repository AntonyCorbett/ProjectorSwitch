#include <windows.h>
#include <vector>
#include <stdexcept>
#include "MonitorService.h"
#include "MonitorData.h"
#include "DisplayConfigData.h"


std::vector<MonitorData> MonitorService::GetMonitorsData()
{
    std::vector<MonitorData> returnData;

    std::vector<MONITORINFOEX> monitorInfo = GetMonitorsInfo();
    std::vector<DisplayConfigData> displayInfo = GetDisplayConfigInfo();

	if (monitorInfo.size() != displayInfo.size())
	{
		throw std::runtime_error("Monitor and display config data size mismatch");
	}

	for (size_t i = 0; i < monitorInfo.size(); ++i)
	{
		MonitorData data(monitorInfo[i], displayInfo[i]);
		returnData.push_back(data);
	}

    return returnData;
}


BOOL MyEnumMonitorProc(HMONITOR monitor, HDC hdc, LPRECT rect, LPARAM lparam)
{
    std::vector<MONITORINFOEX>* monitors = (std::vector<MONITORINFOEX>*)lparam;

    MONITORINFOEX info;
    info.cbSize = sizeof(MONITORINFOEX);

    if (::GetMonitorInfo(monitor, &info))
    {
        monitors->push_back(info);
    }

    return true;
}

std::vector<MONITORINFOEX> MonitorService::GetMonitorsInfo()
{
	std::vector<MONITORINFOEX> monitors;

    EnumDisplayMonitors(NULL, NULL, MyEnumMonitorProc, (LPARAM) &monitors);

	return monitors;
}

std::vector<DisplayConfigData> MonitorService::GetDisplayConfigInfo()
{
    std::vector<DisplayConfigData> returnData;
        
    std::vector<DISPLAYCONFIG_PATH_INFO> paths;
    std::vector<DISPLAYCONFIG_MODE_INFO> modes;
    UINT32 flags = QDC_ONLY_ACTIVE_PATHS | QDC_VIRTUAL_MODE_AWARE;
    LONG result = ERROR_SUCCESS;

    do
    {
        // Determine how many path and mode structures to allocate
        UINT32 pathCount, modeCount;
        result = GetDisplayConfigBufferSizes(flags, &pathCount, &modeCount);

        if (result != ERROR_SUCCESS)
        {
			throw std::runtime_error("Failed to get display config buffer sizes");            
        }

        // Allocate the path and mode arrays
        paths.resize(pathCount);
        modes.resize(modeCount);

        // Get all active paths and their modes
        result = QueryDisplayConfig(flags, &pathCount, paths.data(), &modeCount, modes.data(), nullptr);

        // The function may have returned fewer paths/modes than estimated
        paths.resize(pathCount);
        modes.resize(modeCount);

        // It's possible that between the call to GetDisplayConfigBufferSizes and QueryDisplayConfig
        // that the display state changed, so loop on the case of ERROR_INSUFFICIENT_BUFFER.
    } while (result == ERROR_INSUFFICIENT_BUFFER);

    if (result != ERROR_SUCCESS)
    {
        throw std::runtime_error("Failed to query display config");
    }

    // For each active path
    for (auto& path : paths)
    {
        // Find the target (monitor) friendly name
        DISPLAYCONFIG_TARGET_DEVICE_NAME targetName = {};
        targetName.header.adapterId = path.targetInfo.adapterId;
        targetName.header.id = path.targetInfo.id;
        targetName.header.type = DISPLAYCONFIG_DEVICE_INFO_GET_TARGET_NAME;
        targetName.header.size = sizeof(targetName);
        result = DisplayConfigGetDeviceInfo(&targetName.header);

        if (result != ERROR_SUCCESS)
        {
            throw std::runtime_error("Failed to get monitor friendly name");
        }

        // Find the adapter device name
        DISPLAYCONFIG_ADAPTER_NAME adapterName = {};
        adapterName.header.adapterId = path.targetInfo.adapterId;
        adapterName.header.type = DISPLAYCONFIG_DEVICE_INFO_GET_ADAPTER_NAME;
        adapterName.header.size = sizeof(adapterName);

        result = DisplayConfigGetDeviceInfo(&adapterName.header);

        if (result != ERROR_SUCCESS)
        {
            throw std::runtime_error("Failed to get adapter device name");
        }
                
		returnData.push_back(DisplayConfigData(
			path.targetInfo.id,
            targetName.flags.friendlyNameFromEdid ? targetName.monitorFriendlyDeviceName : L"Unknown",
			adapterName.adapterDevicePath));        
    }

    return returnData;
}