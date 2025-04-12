#include <atlbase.h>
#include "ZoomService.h"
#include "SettingsService.h"
#include "MonitorService.h"
#include "AutomationConditionWrapper.h"
#include "VariantWrapper.h"

std::wstring ZoomProcessName = L"Zoom.exe";

ZoomService::ZoomService(
	AutomationService* automationService, 
	ProcessesService *processesService)
{
	this->cachedDesktopWindow = nullptr;
	this->automationService = automationService;
	this->processesService = processesService;
	this->mediaWindowOriginalPosition = RECT();
}

ZoomService::~ZoomService()
{
	if (cachedDesktopWindow != nullptr)
	{
		cachedDesktopWindow->Release();
		cachedDesktopWindow = nullptr;
	}

	if (automationService != nullptr)
	{
		delete automationService;
		automationService = nullptr;
	}

	if (processesService != nullptr)
	{
		delete processesService;
		processesService = nullptr;
	}
}

DisplayWindowResult ZoomService::Display()
{
	DisplayWindowResult result;
		
	FindWindowsResult findWindowsResult = FindMediaWindow();

	if (!findWindowsResult.BespokeErrorMsg.empty())
	{
		result.AllOk = false;
		result.ErrorMessage = findWindowsResult.BespokeErrorMsg;
		return result;
	}

	if (!findWindowsResult.IsRunning)
	{
		result.AllOk = false;
		result.ErrorMessage = L"Zoom is not running!";
		return result;
	}

	if (findWindowsResult.Element == nullptr || !findWindowsResult.FoundMediaWindow)
	{
		result.AllOk = false;
		result.ErrorMessage = L"Could not find Zoom media window!";
		return result;
	}
		
	IUIAutomationElement* mediaWindow = findWindowsResult.Element;
	mediaWindow->get_CurrentBoundingRectangle(&mediaWindowOriginalPosition);

	UIA_HWND windowHandle;
	mediaWindow->get_CurrentNativeWindowHandle(&windowHandle);

	RECT mediaMonitorRect = GetTargetMonitorRect();
	if (IsRectEmpty(&mediaMonitorRect))
	{
		result.AllOk = false;
		result.ErrorMessage = L"Could not find target monitor!";
		return result;
	}

	InternalDisplay((HWND)windowHandle, mediaMonitorRect);

	mediaWindow->Release();
	mediaWindow = nullptr;

	result.AllOk = true;
	
	return result;
}

void ZoomService::Hide()
{
	FindWindowsResult findWindowsResult = FindMediaWindow();

	if (!findWindowsResult.BespokeErrorMsg.empty() ||
		!findWindowsResult.IsRunning ||
		!findWindowsResult.FoundMediaWindow)		
	{
		return;
	}

	IUIAutomationElement* mediaWindow = findWindowsResult.Element;

	UIA_HWND windowHandle;
	mediaWindow->get_CurrentNativeWindowHandle(&windowHandle);

	InternalHide((HWND)windowHandle);	
}

const void ZoomService::InternalHide(HWND windowHandle)
{	
	SetForegroundWindow(windowHandle);

	const bool topMost = false;

	SetWindowPos(
		windowHandle,
		HWND_NOTOPMOST,
		mediaWindowOriginalPosition.left,
		mediaWindowOriginalPosition.top,
		mediaWindowOriginalPosition.right - mediaWindowOriginalPosition.left,
		mediaWindowOriginalPosition.bottom - mediaWindowOriginalPosition.top,
		SWP_NOCOPYBITS | SWP_NOSENDCHANGING | SWP_SHOWWINDOW);
}

RECT ZoomService::GetPrimaryMonitorRect()
{
	MonitorService monitorService;

	std::vector<MonitorData> monitorData = monitorService.GetMonitorsData();
	for (std::vector<MonitorData>::iterator i = monitorData.begin(); i != monitorData.end(); ++i)
	{
		if (i->IsPrimary)
		{
			if (IsRectEmpty(&i->WorkRect))
			{
				return i->MonitorRect;
			}
			else
			{
				return i->WorkRect;
			}
		}
	}

	return RECT();
}

void ZoomService::InternalDisplay(HWND windowHandle, RECT mediaMonitorRect)
{
	ShowWindow(windowHandle, SW_NORMAL);
	SetForegroundWindow(windowHandle);

	const bool topMost = true;

	const int adjustmentTop = 54; // adjustment for titlebar	
	const int border = 8; // adjustment for borders

	SetWindowPos(
		windowHandle, 
		topMost ? HWND_TOPMOST : HWND_NOTOPMOST,
		mediaMonitorRect.left - border,
		mediaMonitorRect.top - adjustmentTop,
		(mediaMonitorRect.right - mediaMonitorRect.left) + (border * 2),
		(mediaMonitorRect.bottom - mediaMonitorRect.top) + adjustmentTop + (border * 2),		
		SWP_NOCOPYBITS | SWP_NOSENDCHANGING | SWP_SHOWWINDOW);
}

RECT ZoomService::GetTargetMonitorRect()
{
	SettingsService settingsService;

	int monitorId = settingsService.LoadSelectedMonitorId();
	if (monitorId == -1)
	{
		return RECT();
	}

	MonitorService monitorService;

	std::vector<MonitorData> monitorData = monitorService.GetMonitorsData();
	for (std::vector<MonitorData>::iterator i = monitorData.begin(); i != monitorData.end(); ++i)
	{
		if (i->Id == monitorId)
		{
			return i->MonitorRect;
		}
	}

	return RECT();
}

FindWindowsResult ZoomService::FindMediaWindow()
{
	FindWindowsResult result;

	if (automationService == nullptr)
	{
		result.BespokeErrorMsg = L"AutomationService is not initialized.";
		return result;
	}

	if (cachedDesktopWindow == nullptr)
	{
		IUIAutomationElement* desktop = automationService->DesktopElement();
		if (desktop == nullptr)
		{
			result.BespokeErrorMsg = L"Failed to get Desktop Element.";
			return result;
		}	

		cachedDesktopWindow = desktop;
	}

	result.FoundDesktop = true;

	std::vector<HANDLE> zoomProcesses = processesService->GetProcessesByName(ZoomProcessName);
	switch (zoomProcesses.size())
	{
	case 0:
		return result;
	case 1:		
		result.BespokeErrorMsg = L"Could not find Zoom's second window. Please use Zoom 'dual monitor' configuration";
		return result;
	}

	result.IsRunning = true;

	IUIAutomationElement* mediaWindow = LocateZoomMediaWindow();
	if (mediaWindow != nullptr)
	{
		result.Element = mediaWindow;
		result.FoundMediaWindow = true;
	}
	else
	{
		// todo: log error
	}

	return result;
}

IUIAutomationElement* ZoomService::LocateZoomMediaWindow()
{
	if (cachedDesktopWindow == nullptr)
	{
		return nullptr;
	}

	// Find the Zoom media window by searching for the specific class name and name.
	// The class name and name may vary based on the Zoom version and configuration.
	
	IUIAutomation* pAutomation = automationService->GetAutomationInterface();
	
	VariantWrapper varName;
	varName.SetString(L"Zoom Workplace");

	IUIAutomationCondition* nameCondition;
    pAutomation->CreatePropertyCondition(UIA_NamePropertyId, *varName, &nameCondition);
	AutomationConditionWrapper nameConditionWrapper(nameCondition);

	VariantWrapper varClassName;
	varClassName.SetString(L"ConfMultiTabContentWndClass");

	IUIAutomationCondition* classNameCondition;
	pAutomation->CreatePropertyCondition(UIA_ClassNamePropertyId, *varClassName, &classNameCondition);
	AutomationConditionWrapper classNameConditionWrapper(classNameCondition);
		
	IUIAutomationCondition* andCondition;	
	pAutomation->CreateAndCondition(
		nameConditionWrapper.GetCondition(), 
		classNameConditionWrapper.GetCondition(), 
		&andCondition);
	AutomationConditionWrapper andConditionWrapper(andCondition);

	IUIAutomationElement* mediaWindow = nullptr;
	cachedDesktopWindow->FindFirst(TreeScope_Children, andConditionWrapper.GetCondition(), &mediaWindow);
	
	return mediaWindow;
}
