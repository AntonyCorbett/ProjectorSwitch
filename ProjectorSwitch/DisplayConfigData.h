#pragma once
#include <windows.h>
#include <string>

struct DisplayConfigData
{
	DisplayConfigData()
		: Id(0)		
	{		
	}

	DisplayConfigData(const UINT id, const std::wstring &friendlyName, const std::wstring &devicePath)
	{
		Id = id;
		FriendlyName = friendlyName;
		DevicePath = devicePath;
	}

	UINT Id;
	std::wstring FriendlyName;
	std::wstring DevicePath;
};

