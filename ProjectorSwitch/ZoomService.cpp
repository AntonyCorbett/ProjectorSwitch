#include <atlbase.h>
#include "ZoomService.h"
#include "SettingsService.h"
#include "MonitorService.h"
#include "AutomationConditionWrapper.h"
#include "VariantWrapper.h"

std::wstring ZoomProcessName = L"Zoom.exe";

ZoomService::ZoomService(AutomationService* automationService, ProcessesService *processesService)
	: cachedDesktopWindow_(nullptr)
	, automationService_(automationService)
	, processesService_(processesService)
	, mediaWindowOriginalPosition_({ 0 })
{	
}

ZoomService::~ZoomService()
{
	if (cachedDesktopWindow_ != nullptr)
	{
		cachedDesktopWindow_->Release();
		cachedDesktopWindow_ = nullptr;
	}

	if (automationService_ != nullptr)
	{
		delete automationService_;
		automationService_ = nullptr;
	}

	if (processesService_ != nullptr)
	{
		delete processesService_;
		processesService_ = nullptr;
	}
}

DisplayWindowResult ZoomService::Display()
{
	DisplayWindowResult result;
		
	auto findWindowsResult = FindMediaWindow();

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
		
	auto mediaWindow = findWindowsResult.Element;
	mediaWindow->get_CurrentBoundingRectangle(&mediaWindowOriginalPosition_);

	UIA_HWND windowHandle;
	mediaWindow->get_CurrentNativeWindowHandle(&windowHandle);

	auto mediaMonitorRect = GetTargetMonitorRect();
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
	auto findWindowsResult = FindMediaWindow();

	if (!findWindowsResult.BespokeErrorMsg.empty() ||
		!findWindowsResult.IsRunning ||
		!findWindowsResult.FoundMediaWindow)		
	{
		return;
	}

	auto mediaWindow = findWindowsResult.Element;

	UIA_HWND windowHandle;
	mediaWindow->get_CurrentNativeWindowHandle(&windowHandle);

	InternalHide((HWND)windowHandle);	
}

const void ZoomService::InternalHide(HWND windowHandle)
{	
	SetForegroundWindow(windowHandle);

	const auto topMost = false;

	SetWindowPos(
		windowHandle,
		HWND_NOTOPMOST,
		mediaWindowOriginalPosition_.left,
		mediaWindowOriginalPosition_.top,
		mediaWindowOriginalPosition_.right - mediaWindowOriginalPosition_.left,
		mediaWindowOriginalPosition_.bottom - mediaWindowOriginalPosition_.top,
		SWP_NOCOPYBITS | SWP_NOSENDCHANGING | SWP_SHOWWINDOW);
}

RECT ZoomService::GetPrimaryMonitorRect()
{
	MonitorService monitorService;

	auto monitorData = monitorService.GetMonitorsData();
	for (auto& i : monitorData)
	{
		if (i.IsPrimary)
		{
			if (IsRectEmpty(&i.WorkRect))
			{
				return i.MonitorRect;
			}
			else
			{
				return i.WorkRect;
			}
		}
	}

	return RECT();
}

void ZoomService::InternalDisplay(HWND windowHandle, RECT mediaMonitorRect)
{
	ShowWindow(windowHandle, SW_NORMAL);
	SetForegroundWindow(windowHandle);

	const auto topMost = true;

	const auto adjustmentTop = 54; // adjustment for titlebar	
	const auto border = 8; // adjustment for borders

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

	auto monitorId = settingsService.LoadSelectedMonitorId();
	if (monitorId == -1)
	{
		return RECT();
	}

	MonitorService monitorService;

	auto monitorData = monitorService.GetMonitorsData();
	for (auto& i : monitorData)
	{
		if (i.Id == monitorId)
		{
			return i.MonitorRect;
		}
	}

	return RECT();
}

FindWindowsResult ZoomService::FindMediaWindow()
{
	FindWindowsResult result;

	if (automationService_ == nullptr)
	{
		result.BespokeErrorMsg = L"AutomationService is not initialized.";
		return result;
	}

	if (cachedDesktopWindow_ == nullptr)
	{
		IUIAutomationElement* desktop = automationService_->DesktopElement();
		if (desktop == nullptr)
		{
			result.BespokeErrorMsg = L"Failed to get Desktop Element.";
			return result;
		}	

		cachedDesktopWindow_ = desktop;
	}

	result.FoundDesktop = true;

	auto zoomProcesses = processesService_->GetProcessesByName(ZoomProcessName);
	switch (zoomProcesses.size())
	{
	case 0:
		return result;
	case 1:		
		result.BespokeErrorMsg = L"Could not find Zoom's second window. Please use Zoom 'dual monitor' configuration";
		return result;
	}

	result.IsRunning = true;

	auto mediaWindow = LocateZoomMediaWindow();
	if (mediaWindow != nullptr)
	{
		result.Element = mediaWindow;
		result.FoundMediaWindow = true;
	}
	else
	{
		OutputDebugString(L"Could not get Zoom window!");
	}

	return result;
}

IUIAutomationElement* ZoomService::LocateZoomMediaWindow()
{
	if (cachedDesktopWindow_ == nullptr)
	{
		return nullptr;
	}

	// Find the Zoom media window by searching for the specific class name and name.
	// The class name and name may vary based on the Zoom version and configuration.
	
	auto pAutomation = automationService_->GetAutomationInterface();
	
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
	cachedDesktopWindow_->FindFirst(TreeScope_Children, andConditionWrapper.GetCondition(), &mediaWindow);
	
	return mediaWindow;
}
