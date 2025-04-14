#pragma once
#include "AutomationService.h"
#include "FindWindowsResult.h"
#include "ProcessesService.h"
#include "DisplayWindowResult.h"

class ZoomService
{
public:
	ZoomService(AutomationService *automationService, ProcessesService *processesService);
	~ZoomService();

	DisplayWindowResult Display();
	void Hide();
	bool IsDisplayed();

private:
	RECT mediaWindowOriginalPosition_;
	IUIAutomationElement* cachedDesktopWindow_;
	AutomationService* automationService_;
	ProcessesService* processesService_;	
		
	FindWindowsResult FindMediaWindow();
	IUIAutomationElement* LocateZoomMediaWindow();
	RECT GetTargetMonitorRect();
	RECT GetPrimaryMonitorRect();
	void InternalDisplay(HWND windowHandle, RECT mediaMonitorRect);
	const void InternalHide(HWND windowHandle);
	RECT CalculateTargetRect(RECT mediaMonitorRect);
};

