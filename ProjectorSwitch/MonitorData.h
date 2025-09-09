#pragma once
#include <Windows.h>
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
	std::wstring SerialNumber;
	std::wstring Key;

	MonitorData() = default;

	MonitorData(
		const MONITORINFOEX& monitorInfoEx,
		const DisplayConfigData& displayConfigData,
		const std::wstring& serialNumber)
		: Id(displayConfigData.Id)
		, IsPrimary((monitorInfoEx.dwFlags& MONITORINFOF_PRIMARY) != 0)
		, MonitorRect(monitorInfoEx.rcMonitor)
		, WorkRect(monitorInfoEx.rcWork)
		, FriendlyName(displayConfigData.FriendlyName)
		, DevicePath(displayConfigData.DevicePath)
		, DeviceName(monitorInfoEx.szDevice)
		, SerialNumber(serialNumber)
	{
		if (!serialNumber.empty())
		{
			Key = L"SERIAL:" + serialNumber;
		}
		else
		{
			Key = L"PATH:" + DevicePath;
		}
	}
};
