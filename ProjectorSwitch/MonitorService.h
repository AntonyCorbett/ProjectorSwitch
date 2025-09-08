#pragma once
#include <windows.h>
#include <vector>
#include "MonitorData.h"
#include "DisplayConfigData.h"

class MonitorService
{
public:	
	[[nodiscard]] std::vector<MonitorData> GetMonitorsData() const;

	static std::wstring TryGetMonitorSerialFromDevicePath(const std::wstring& devicePath);
	
private:
	[[nodiscard]] static std::vector<MONITORINFOEX> GetMonitorsInfo();
	[[nodiscard]] static std::vector<DisplayConfigData> GetDisplayConfigInfo();
};
