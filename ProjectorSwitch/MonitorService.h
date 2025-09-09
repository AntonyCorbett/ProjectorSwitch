#pragma once
#include <windows.h>
#include <vector>
#include "MonitorData.h"
#include "DisplayConfigData.h"

class MonitorService
{
public:
	[[nodiscard]] std::vector<MonitorData> GetMonitorsData() const;
	[[nodiscard]] static int FindMonitorIndex(const std::vector<MonitorData>& monitors, const std::wstring& key, const RECT& rect);

private:
	[[nodiscard]] static std::wstring TryGetMonitorSerialFromDevicePath(const std::wstring& devicePath);
	[[nodiscard]] static std::vector<MONITORINFOEX> GetMonitorsInfo();
	[[nodiscard]] static std::vector<DisplayConfigData> GetDisplayConfigInfo();
};