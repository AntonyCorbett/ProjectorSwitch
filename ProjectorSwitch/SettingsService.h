#pragma once
#include <windows.h>
#include <string>
#include <WinUser.h>

class SettingsService
{
public:
	SettingsService();
	~SettingsService();

	void SaveSelectedMonitorId(const int monitorId);
	int LoadSelectedMonitorId();

	void SaveWindowPlacement(WINDOWPLACEMENT placement);
	WINDOWPLACEMENT LoadWindowPlacement();

private:
	std::wstring pathToFile_;

	void SaveString(const std::wstring& section, const std::wstring& key, const std::wstring& value);
	void SaveInt(const std::wstring& section, const std::wstring& key, const int value);

	std::wstring LoadString(const std::wstring& section, const std::wstring& key);
	int LoadInt(const std::wstring& section, const std::wstring& key, int defaultValue);
};

