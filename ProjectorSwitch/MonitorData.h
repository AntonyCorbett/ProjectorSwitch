#pragma once
#include <windows.h>
#include <string>
#include "DisplayConfigData.h"

struct MonitorData
{
	MonitorData()
	{
		::ZeroMemory(this, sizeof(MonitorData));
		Id = 0;
		MonitorRect = { 0 };
		WorkRect = { 0 };
		IsPrimary = false;
	}

	MonitorData(MONITORINFOEX monitorInfoEx, DisplayConfigData displayConfigData)
	{
		::ZeroMemory(this, sizeof(MonitorData));
		Id = displayConfigData.Id;
		FriendlyName = displayConfigData.FriendlyName;
		DevicePath = displayConfigData.DevicePath;
		DeviceName = monitorInfoEx.szDevice;
		MonitorRect = monitorInfoEx.rcMonitor;
		WorkRect = monitorInfoEx.rcWork;
		IsPrimary = (monitorInfoEx.dwFlags & MONITORINFOF_PRIMARY) != 0;
	}

	RECT MonitorRect;
	RECT WorkRect;
	UINT Id;
	std::wstring FriendlyName;
	std::wstring DevicePath;
	std::wstring DeviceName;
	bool IsPrimary;
};

