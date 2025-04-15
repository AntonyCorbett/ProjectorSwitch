#include "SettingsService.h"

const std::wstring SETTINGS_SECTION = L"SETTINGS";
const std::wstring SELECTED_MONITOR_DEVICE_NAME = L"SelectedMonitor";

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
	InternalSaveInt(WINDOW_SECTION, SHOW_CMD, placement.showCmd);
	InternalSaveInt(WINDOW_SECTION, FLAGS, placement.flags);
	InternalSaveInt(WINDOW_SECTION, MIN_POS_X, placement.ptMinPosition.x);
	InternalSaveInt(WINDOW_SECTION, MIN_POS_Y, placement.ptMinPosition.y);
	InternalSaveInt(WINDOW_SECTION, MAX_POS_X, placement.ptMaxPosition.x);
	InternalSaveInt(WINDOW_SECTION, MAX_POS_Y, placement.ptMaxPosition.y);
	InternalSaveInt(WINDOW_SECTION, POS_LEFT, placement.rcNormalPosition.left);
	InternalSaveInt(WINDOW_SECTION, POS_TOP, placement.rcNormalPosition.top);
	InternalSaveInt(WINDOW_SECTION, POS_RIGHT, placement.rcNormalPosition.right);
	InternalSaveInt(WINDOW_SECTION, POS_BOTTOM, placement.rcNormalPosition.bottom);
}

WINDOWPLACEMENT SettingsService::LoadWindowPlacement()
{
	WINDOWPLACEMENT wp;
	wp.length = sizeof(wp);

	wp.showCmd = InternalLoadInt(WINDOW_SECTION, SHOW_CMD, 0);
	wp.flags = InternalLoadInt(WINDOW_SECTION, FLAGS, 0);
	wp.ptMinPosition.x = InternalLoadInt(WINDOW_SECTION, MIN_POS_X, 0);
	wp.ptMinPosition.y = InternalLoadInt(WINDOW_SECTION, MIN_POS_Y, 0);
	wp.ptMaxPosition.x = InternalLoadInt(WINDOW_SECTION, MAX_POS_X, 0);
	wp.ptMaxPosition.y = InternalLoadInt(WINDOW_SECTION, MAX_POS_Y, 0);
	wp.rcNormalPosition.left = InternalLoadInt(WINDOW_SECTION, POS_LEFT, 0);
	wp.rcNormalPosition.top = InternalLoadInt(WINDOW_SECTION, POS_TOP, 0);
	wp.rcNormalPosition.right = InternalLoadInt(WINDOW_SECTION, POS_RIGHT, 0);
	wp.rcNormalPosition.bottom = InternalLoadInt(WINDOW_SECTION, POS_BOTTOM, 0);

	return wp;
}

void SettingsService::SaveSelectedMonitor(std::wstring monitorDeviceName)
{
	InternalSaveString(SETTINGS_SECTION, SELECTED_MONITOR_DEVICE_NAME, monitorDeviceName);
}

std::wstring SettingsService::LoadSelectedMonitor()
{
	return InternalLoadString(SETTINGS_SECTION, SELECTED_MONITOR_DEVICE_NAME);
}

void SettingsService::InternalSaveString(const std::wstring& section, const std::wstring& key, const std::wstring& value)
{
	WritePrivateProfileString(section.c_str(), key.c_str(), value.c_str(), pathToFile_.c_str());
}

void SettingsService::InternalSaveInt(const std::wstring& section, const std::wstring& key, const int value)
{
	std::wstring strValue = std::to_wstring(value);
	InternalSaveString(section.c_str(), key.c_str(), strValue.c_str());
}

std::wstring SettingsService::InternalLoadString(const std::wstring& section, const std::wstring& key)
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

int SettingsService::InternalLoadInt(const std::wstring& section, const std::wstring& key, int defaultValue)
{
	auto value = InternalLoadString(section, key);
	if (value.empty())
	{
		return defaultValue;
	}

	return std::stoi(value);
}
