#pragma once
#include <windows.h>
#include <vector>
#include "MonitorData.h"
#include "DisplayConfigData.h"

class MonitorService
{
public:	
	[[nodiscard]] std::vector<MonitorData> GetMonitorsData() const;
	
private:
	[[nodiscard]] static std::vector<MONITORINFOEX> GetMonitorsInfo();
	[[nodiscard]] static std::vector<DisplayConfigData> GetDisplayConfigInfo();
};
