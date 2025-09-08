#pragma once
#include <WinUser.h>
#include "SettingsService.h"

class WindowPlacementService
{
private:	
	SettingsService* settingsService_;
	static bool IsValidPlacement(const WINDOWPLACEMENT& placement);

public:
	WindowPlacementService(SettingsService* settingsService)
		: settingsService_(settingsService)
	{
	}

	void SaveWindowPlace(HWND mainWindow) const;
	void RestoreWindowPlace(HWND mainWindow) const;
};

