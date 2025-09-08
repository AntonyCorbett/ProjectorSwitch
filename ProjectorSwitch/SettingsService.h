#pragma once
#include <windows.h>
#include <string>
#include <WinUser.h>

class SettingsService
{
public:
	SettingsService();
	~SettingsService();

	void SaveSelectedMonitorRect(RECT rect) const;
	RECT LoadSelectedMonitorRect() const;

	// persist a stable monitor key (e.g., "SERIAL:xxxx" or "PATH:devicePath")
	void SaveSelectedMonitorKey(const std::wstring& key) const;
	std::wstring LoadSelectedMonitorKey() const;

	void SaveWindowPlacement(const WINDOWPLACEMENT& placement) const;
	WINDOWPLACEMENT LoadWindowPlacement() const;

private:
	std::wstring pathToFile_;

	void InternalSaveString(const std::wstring& section, const std::wstring& keyName, const std::wstring& keyValue) const;
	void InternalSaveInt(const std::wstring& section, const std::wstring& keyName, const int keyValue) const;

	std::wstring InternalLoadString(const std::wstring& section, const std::wstring& keyName) const;
	int InternalLoadInt(const std::wstring& section, const std::wstring& keyName, int defaultKeyValue) const;
};