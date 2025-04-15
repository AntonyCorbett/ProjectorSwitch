#pragma once
#include <windows.h>
#include <string>
#include <WinUser.h>

class SettingsService
{
public:
	SettingsService();
	~SettingsService();

	void SaveSelectedMonitorRect(RECT rect);
	RECT LoadSelectedMonitorRect();

	void SaveWindowPlacement(WINDOWPLACEMENT placement);
	WINDOWPLACEMENT LoadWindowPlacement();

private:
	std::wstring pathToFile_;

	void InternalSaveString(const std::wstring& section, const std::wstring& key, const std::wstring& value);
	void InternalSaveInt(const std::wstring& section, const std::wstring& key, const int value);

	std::wstring InternalLoadString(const std::wstring& section, const std::wstring& key);
	int InternalLoadInt(const std::wstring& section, const std::wstring& key, int defaultValue);
};

