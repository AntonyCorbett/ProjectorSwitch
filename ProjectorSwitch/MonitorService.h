#pragma once
#include <vector>
#include "monitorData.h"
#include "DisplayConfigData.h"

class MonitorService
{
public:	
	std::vector<MonitorData> GetMonitorsData();
	
private:
	std::vector<DISPLAY_DEVICE> GetDevicesInfo();
	std::vector<MONITORINFOEX> GetMonitorsInfo();
	std::vector<DisplayConfigData> GetDisplayConfigInfo();
};
