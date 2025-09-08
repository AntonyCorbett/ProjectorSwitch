#include "SettingsService.h"

namespace
{
	const std::wstring SettingsSection = L"SETTINGS";
	const std::wstring SelectedMonitorL = L"SelectedMonitorL";
	const std::wstring SelectedMonitorT = L"SelectedMonitorT";
	const std::wstring SelectedMonitorR = L"SelectedMonitorR";
	const std::wstring SelectedMonitorB = L"SelectedMonitorB";
	const std::wstring SelectedMonitorKey = L"SelectedMonitorKey";

	const std::wstring WindowSection = L"WINDOW";
	const std::wstring ShowCmd = L"ShowCmd";
	const std::wstring Flags = L"Flags";
	const std::wstring MinPosX = L"MinPosX";
	const std::wstring MinPosY = L"MinPosY";
	const std::wstring MaxPosX = L"MaxPosX";
	const std::wstring MaxPosY = L"MaxPosY";
	const std::wstring PosLeft = L"Left";
	const std::wstring PosTop = L"Top";
	const std::wstring PosRight = L"Right";
	const std::wstring PosBottom = L"Bottom";
}

SettingsService::SettingsService()
{
	// Get the current directory
	WCHAR buffer[MAX_PATH];
	GetCurrentDirectory(MAX_PATH, buffer);
	pathToFile_ = buffer + std::wstring(L"\\settings.ini");
}

SettingsService::~SettingsService() = default;

void SettingsService::SaveWindowPlacement(const WINDOWPLACEMENT& placement) const
{
	InternalSaveInt(WindowSection, ShowCmd, static_cast<int>(placement.showCmd));
	InternalSaveInt(WindowSection, Flags, static_cast<int>(placement.flags));
	InternalSaveInt(WindowSection, MinPosX, placement.ptMinPosition.x);
	InternalSaveInt(WindowSection, MinPosY, placement.ptMinPosition.y);
	InternalSaveInt(WindowSection, MaxPosX, placement.ptMaxPosition.x);
	InternalSaveInt(WindowSection, MaxPosY, placement.ptMaxPosition.y);
	InternalSaveInt(WindowSection, PosLeft, placement.rcNormalPosition.left);
	InternalSaveInt(WindowSection, PosTop, placement.rcNormalPosition.top);
	InternalSaveInt(WindowSection, PosRight, placement.rcNormalPosition.right);
	InternalSaveInt(WindowSection, PosBottom, placement.rcNormalPosition.bottom);
}

WINDOWPLACEMENT SettingsService::LoadWindowPlacement() const
{
	WINDOWPLACEMENT wp;
	wp.length = sizeof(wp);

	wp.showCmd = InternalLoadInt(WindowSection, ShowCmd, 0);
	wp.flags = InternalLoadInt(WindowSection, Flags, 0);
	wp.ptMinPosition.x = InternalLoadInt(WindowSection, MinPosX, 0);
	wp.ptMinPosition.y = InternalLoadInt(WindowSection, MinPosY, 0);
	wp.ptMaxPosition.x = InternalLoadInt(WindowSection, MaxPosX, 0);
	wp.ptMaxPosition.y = InternalLoadInt(WindowSection, MaxPosY, 0);
	wp.rcNormalPosition.left = InternalLoadInt(WindowSection, PosLeft, 0);
	wp.rcNormalPosition.top = InternalLoadInt(WindowSection, PosTop, 0);
	wp.rcNormalPosition.right = InternalLoadInt(WindowSection, PosRight, 0);
	wp.rcNormalPosition.bottom = InternalLoadInt(WindowSection, PosBottom, 0);

	return wp;
}

void SettingsService::SaveSelectedMonitorRect(const RECT rect) const
{
	InternalSaveInt(SettingsSection, SelectedMonitorL, rect.left);
	InternalSaveInt(SettingsSection, SelectedMonitorT, rect.top);
	InternalSaveInt(SettingsSection, SelectedMonitorR, rect.right);
	InternalSaveInt(SettingsSection, SelectedMonitorB, rect.bottom);
}

RECT SettingsService::LoadSelectedMonitorRect() const
{
	RECT rect;

	rect.left = InternalLoadInt(SettingsSection, SelectedMonitorL, 0);
	rect.top = InternalLoadInt(SettingsSection, SelectedMonitorT, 0);
	rect.right = InternalLoadInt(SettingsSection, SelectedMonitorR, 0);
	rect.bottom = InternalLoadInt(SettingsSection, SelectedMonitorB, 0);

	return rect;
}

void SettingsService::SaveSelectedMonitorKey(const std::wstring& key) const
{
	InternalSaveString(SettingsSection, SelectedMonitorKey, key);
}

std::wstring SettingsService::LoadSelectedMonitorKey() const
{
	return InternalLoadString(SettingsSection, SelectedMonitorKey);
}

void SettingsService::InternalSaveString(
	const std::wstring& section, const std::wstring& keyName, const std::wstring& keyValue) const
{
	WritePrivateProfileString(section.c_str(), keyName.c_str(), keyValue.c_str(), pathToFile_.c_str());
}

void SettingsService::InternalSaveInt(
	const std::wstring& section, const std::wstring& keyName, const int keyValue) const
{
	const std::wstring strValue = std::to_wstring(keyValue);
	InternalSaveString(section, keyName, strValue);
}

std::wstring SettingsService::InternalLoadString(
	const std::wstring& section, const std::wstring& keyName) const
{
	WCHAR buffer[256];
	GetPrivateProfileString(
		section.c_str(),
		keyName.c_str(),
		L"",
		buffer, sizeof(buffer) / sizeof(WCHAR),
		pathToFile_.c_str());

	return std::wstring{ buffer };
}

int SettingsService::InternalLoadInt(
	const std::wstring& section, const std::wstring& keyName, const int defaultKeyValue) const
{
	const auto value = InternalLoadString(section, keyName);
	if (value.empty())
	{
		return defaultKeyValue;
	}

	return std::stoi(value);
}
