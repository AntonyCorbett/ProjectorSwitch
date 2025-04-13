#include "SettingsService.h"

const std::wstring SETTINGS_SECTION = L"SETTINGS";
const std::wstring SELECTED_MONITOR_ID = L"SelectedMonitorId";

const std::wstring WINDOW_SECTION = L"WINDOW";
const std::wstring SHOW_CMD = L"ShowCmd";
const std::wstring FLAGS = L"Flags";
const std::wstring MIN_POS_X = L"MinPosX";
const std::wstring MIN_POS_Y = L"MinPosY";
const std::wstring MAX_POS_X = L"MaxPosX";
const std::wstring MAX_POS_Y = L"MaxPosY";
const std::wstring POS_LEFT =L"Left";
const std::wstring POS_TOP = L"Top";
const std::wstring POS_RIGHT = L"Right";
const std::wstring POS_BOTTOM = L"Bottom";

SettingsService::SettingsService()
{
	// Get the current directory
	WCHAR Buffer[MAX_PATH];
	GetCurrentDirectory(MAX_PATH, Buffer);
	pathToFile_ = Buffer + std::wstring(L"\\settings.ini");
}

SettingsService::~SettingsService()
{
}

void SettingsService::SaveWindowPlacement(WINDOWPLACEMENT placement)
{
	SaveInt(WINDOW_SECTION, SHOW_CMD, placement.showCmd);
	SaveInt(WINDOW_SECTION, FLAGS, placement.flags);
	SaveInt(WINDOW_SECTION, MIN_POS_X, placement.ptMinPosition.x);
	SaveInt(WINDOW_SECTION, MIN_POS_Y, placement.ptMinPosition.y);
	SaveInt(WINDOW_SECTION, MAX_POS_X, placement.ptMaxPosition.x);
	SaveInt(WINDOW_SECTION, MAX_POS_Y, placement.ptMaxPosition.y);
	SaveInt(WINDOW_SECTION, POS_LEFT, placement.rcNormalPosition.left);
	SaveInt(WINDOW_SECTION, POS_TOP, placement.rcNormalPosition.top);
	SaveInt(WINDOW_SECTION, POS_RIGHT, placement.rcNormalPosition.right);
	SaveInt(WINDOW_SECTION, POS_BOTTOM, placement.rcNormalPosition.bottom);
}

WINDOWPLACEMENT SettingsService::LoadWindowPlacement()
{
	WINDOWPLACEMENT wp;
	wp.length = sizeof(wp);

	wp.showCmd = LoadInt(WINDOW_SECTION, SHOW_CMD, 0);
	wp.flags = LoadInt(WINDOW_SECTION, FLAGS, 0);
	wp.ptMinPosition.x = LoadInt(WINDOW_SECTION, MIN_POS_X, 0);
	wp.ptMinPosition.y = LoadInt(WINDOW_SECTION, MIN_POS_Y, 0);
	wp.ptMaxPosition.x = LoadInt(WINDOW_SECTION, MAX_POS_X, 0);
	wp.ptMaxPosition.y = LoadInt(WINDOW_SECTION, MAX_POS_Y, 0);
	wp.rcNormalPosition.left = LoadInt(WINDOW_SECTION, POS_LEFT, 0);
	wp.rcNormalPosition.top = LoadInt(WINDOW_SECTION, POS_TOP, 0);
	wp.rcNormalPosition.right = LoadInt(WINDOW_SECTION, POS_RIGHT, 0);
	wp.rcNormalPosition.bottom = LoadInt(WINDOW_SECTION, POS_BOTTOM, 0);

	return wp;
}

void SettingsService::SaveSelectedMonitorId(const int monitorId)
{
	SaveInt(SETTINGS_SECTION, SELECTED_MONITOR_ID, monitorId);
}

int SettingsService::LoadSelectedMonitorId()
{
	return LoadInt(SETTINGS_SECTION, SELECTED_MONITOR_ID, -1);
}

void SettingsService::SaveString(const std::wstring& section, const std::wstring& key, const std::wstring& value)
{
	WritePrivateProfileString(section.c_str(), key.c_str(), value.c_str(), pathToFile_.c_str());
}

void SettingsService::SaveInt(const std::wstring& section, const std::wstring& key, const int value)
{
	std::wstring strValue = std::to_wstring(value);
	SaveString(section.c_str(), key.c_str(), strValue.c_str());
}

std::wstring SettingsService::LoadString(const std::wstring& section, const std::wstring& key)
{
	WCHAR buffer[256];
	GetPrivateProfileString(
		section.c_str(), 
		key.c_str(), 
		L"", 
		buffer, sizeof(buffer) / sizeof(WCHAR), 
		pathToFile_.c_str());

	return std::wstring(buffer);
}

int SettingsService::LoadInt(const std::wstring& section, const std::wstring& key, int defaultValue)
{
	auto value = LoadString(section, key);
	if (value.empty())
	{
		return defaultValue;
	}

	return std::stoi(value);
}
