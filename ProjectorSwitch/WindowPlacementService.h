#pragma once
#include <WinUser.h>
#include "SettingsService.h"

class WindowPlacementService
{
private:	
	SettingsService* settingsService_;
	bool IsValidPlacement(WINDOWPLACEMENT& placement);

public:
	WindowPlacementService(SettingsService* settingsService)
		: settingsService_(settingsService)
	{
	}

	void SaveWindowPlace(HWND mainWindow);
	void RestoreWindowPlace(HWND mainWindow);
};

