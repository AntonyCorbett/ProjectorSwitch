#pragma once
#include <windows.h>
#include <string>

struct DisplayConfigData
{
	DisplayConfigData()
	{
		Id = 0;
	}

	DisplayConfigData(UINT id, const WCHAR* friendlyName, const WCHAR* devicePath)
	{
		Id = id;
		FriendlyName = friendlyName;
		DevicePath = devicePath;
	}

	UINT Id;
	std::wstring FriendlyName;
	std::wstring DevicePath;
};

