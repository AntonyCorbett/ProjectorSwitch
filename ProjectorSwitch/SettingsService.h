#pragma once
#include <windows.h>
#include <string>

class SettingsService
{
public:
	SettingsService();
	~SettingsService();

	void SaveSelectedMonitorId(const int monitorId);
	int LoadSelectedMonitorId();

private:
	void SaveString(const std::wstring& key, const std::wstring& value);
	void SaveInt(const std::wstring& key, const int value);

	std::wstring LoadString(const std::wstring& key);
	int LoadInt(const std::wstring& key, int defaultValue);
};

