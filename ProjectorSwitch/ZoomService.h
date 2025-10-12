#pragma once
#include "AutomationService.h"
#include "FindWindowsResult.h"
#include "ProcessesService.h"
#include "DisplayWindowResult.h"

class ZoomService  // NOLINT(cppcoreguidelines-special-member-functions)
{
public:
	ZoomService(AutomationService *automationService, ProcessesService *processesService);
	~ZoomService();

	DisplayWindowResult Toggle();

private:
	RECT mediaWindowOriginalPosition_;
	bool mediaWindowWasMinimized_;
	IUIAutomationElement* cachedDesktopWindow_;	
	AutomationService* automationService_;
	ProcessesService* processesService_;		
		
	FindWindowsResult FindMediaWindow();
	IUIAutomationElement* LocateZoomMediaWindow() const;	
	void InternalHide(HWND windowHandle);

	static RECT GetTargetMonitorRect();
	static RECT GetPrimaryMonitorRect();
	static RECT CalculateTargetRect(RECT mediaMonitorRect, HWND mediaWindowHandle);
	static void InternalDisplay(HWND windowHandle, RECT targetRect);	
	static void ForceZoomWindowForeground(const HWND windowHandle);
};

