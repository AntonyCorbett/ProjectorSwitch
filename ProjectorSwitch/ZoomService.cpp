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

DisplayWindowResult ZoomService::Toggle()
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

	auto mediaMonitorRect = GetTargetMonitorRect();
	if (IsRectEmpty(&mediaMonitorRect))
	{
		result.AllOk = false;
		result.ErrorMessage = L"Could not find target monitor!";
		return result;
	}

	UIA_HWND windowHandle;
	findWindowsResult.Element->get_CurrentNativeWindowHandle(&windowHandle);

	RECT mediaWindowPos;
	findWindowsResult.Element->get_CurrentBoundingRectangle(&mediaWindowPos);
	RECT targetRect = CalculateTargetRect(mediaMonitorRect, (HWND)windowHandle);

	if (EqualRect(&targetRect, &mediaWindowPos))
	{
		// already displayed
		InternalHide((HWND)windowHandle);		
	}
	else
	{
		mediaWindowOriginalPosition_ = mediaWindowPos;
		InternalDisplay((HWND)windowHandle, targetRect);
	}

	return result;
}

RECT ZoomService::GetPrimaryMonitorRect()
{
	MonitorService monitorService;

	std::vector<MonitorData> monitorData = monitorService.GetMonitorsData();
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

RECT ZoomService::CalculateTargetRect(RECT mediaMonitorRect, HWND mediaWindowHandle)
{
	RECT mediaWindowClientRect;
	GetClientRect(mediaWindowHandle, &mediaWindowClientRect);

	RECT mediaWindowRect;
	GetWindowRect(mediaWindowHandle, &mediaWindowRect);

	auto clientWidth = mediaWindowClientRect.right - mediaWindowClientRect.left;
	auto clientHeight = mediaWindowClientRect.bottom - mediaWindowClientRect.top;

	auto windowWidth = mediaWindowRect.right - mediaWindowRect.left;
	auto windowHeight = mediaWindowRect.bottom - mediaWindowRect.top;

	auto xBorder = (windowWidth - clientWidth) / 2;
	auto yBorder = (windowHeight - clientHeight) / 2;

	RECT result;
	result.left = mediaMonitorRect.left - xBorder;
	result.top = mediaMonitorRect.top - yBorder;
	result.right = mediaMonitorRect.right + (xBorder * 2);
	result.bottom = mediaMonitorRect.bottom + (yBorder * 2);

	return result;
}

const void ZoomService::InternalHide(HWND windowHandle)
{
	SetForegroundWindow(windowHandle);

	const auto topMost = false;

	if (IsRectEmpty(&mediaWindowOriginalPosition_))
	{
		// fabricate a suitable location on the primary monitor
		RECT primaryMonitorRect = GetPrimaryMonitorRect();
		mediaWindowOriginalPosition_.left = primaryMonitorRect.left + 10;
		mediaWindowOriginalPosition_.top = primaryMonitorRect.top + 10;
		mediaWindowOriginalPosition_.right = mediaWindowOriginalPosition_.left + 450;
		mediaWindowOriginalPosition_.bottom = mediaWindowOriginalPosition_.top + 300;
	}

	SetWindowPos(
		windowHandle,
		HWND_NOTOPMOST,
		mediaWindowOriginalPosition_.left,
		mediaWindowOriginalPosition_.top,
		mediaWindowOriginalPosition_.right - mediaWindowOriginalPosition_.left,
		mediaWindowOriginalPosition_.bottom - mediaWindowOriginalPosition_.top,
		SWP_NOCOPYBITS | SWP_NOSENDCHANGING | SWP_SHOWWINDOW);
}

void ZoomService::InternalDisplay(HWND windowHandle, RECT targetRect)
{
	ShowWindow(windowHandle, SW_NORMAL);
	SetForegroundWindow(windowHandle);

	// I have tried  DeferWindowPos (see commented out code) but the move still flickers
	
	//HDWP deferHandle = BeginDeferWindowPos(1);
	//if (deferHandle)
	//{
	//	deferHandle = DeferWindowPos(
	//		deferHandle,
	//		windowHandle,
	//		NULL,
	//		targetRect.left,
	//		targetRect.top,
	//		targetRect.right - targetRect.left,
	//		targetRect.bottom - targetRect.top,
	//		SWP_NOCOPYBITS | SWP_SHOWWINDOW);

	//	if (deferHandle)
	//	{
	//		EndDeferWindowPos(deferHandle);
	//	}
	//}

	SetWindowPos(
		windowHandle, 
		HWND_TOPMOST,
		targetRect.left,
		targetRect.top,
		targetRect.right - targetRect.left,
		targetRect.bottom - targetRect.top,		
		SWP_NOCOPYBITS | SWP_NOSENDCHANGING | SWP_SHOWWINDOW);	
}

RECT ZoomService::GetTargetMonitorRect()
{
	SettingsService settingsService;
	return settingsService.LoadSelectedMonitorRect();
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

	std::vector<HANDLE> zoomProcesses = processesService_->GetProcessesByName(ZoomProcessName);
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
	
	IUIAutomation* pAutomation = automationService_->GetAutomationInterface();
	
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
