#include "SettingsService.h"

const WCHAR* SETTINGS_SECTION = L"Settings";
const WCHAR* SELECTED_MONITOR_ID = L"SelectedMonitorId";
std::wstring PathToFile;

SettingsService::SettingsService()
{
	// Get the current directory
	WCHAR Buffer[MAX_PATH];
	GetCurrentDirectory(MAX_PATH, Buffer);
	PathToFile = Buffer + std::wstring(L"\\settings.ini");
}

SettingsService::~SettingsService()
{
}

void SettingsService::SaveSelectedMonitorId(const int monitorId)
{
	SaveInt(SELECTED_MONITOR_ID, monitorId);
}

int SettingsService::LoadSelectedMonitorId()
{
	return LoadInt(SELECTED_MONITOR_ID, -1);
}

void SettingsService::SaveString(const std::wstring& key, const std::wstring& value)
{
	WritePrivateProfileString(SETTINGS_SECTION, key.c_str(), value.c_str(), PathToFile.c_str());
}

void SettingsService::SaveInt(const std::wstring& key, const int value)
{
	std::wstring strValue = std::to_wstring(value);
	SaveString(key.c_str(), strValue.c_str());
}

std::wstring SettingsService::LoadString(const std::wstring& key)
{
	WCHAR buffer[256];
	GetPrivateProfileString(SETTINGS_SECTION, key.c_str(), L"", buffer, sizeof(buffer) / sizeof(WCHAR), PathToFile.c_str());
	return std::wstring(buffer);
}

int SettingsService::LoadInt(const std::wstring& key, int defaultValue)
{
	std::wstring value = LoadString(key);
	if (value.empty())
	{
		return defaultValue;
	}

	return std::stoi(value);
}