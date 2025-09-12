#include <atlbase.h>
#include <dwmapi.h>
#pragma comment(lib, "Dwmapi.lib")  // link DWM
#include "ZoomService.h"
#include "SettingsService.h"
#include "MonitorService.h"
#include "AutomationConditionWrapper.h"
#include "VariantWrapper.h"

namespace
{
	const std::wstring ZoomProcessName = L"Zoom.exe";
}

ZoomService::ZoomService(AutomationService* automationService, ProcessesService *processesService)
	: mediaWindowOriginalPosition_({ 0,0,0,0 })
	, cachedDesktopWindow_(nullptr)
	, automationService_(automationService)
	, processesService_(processesService)	
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

DisplayWindowResult ZoomService::Toggle(const bool fade)
{
	DisplayWindowResult result;

	const FindWindowsResult findWindowsResult = FindMediaWindow();

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

	const auto mediaMonitorRect = GetTargetMonitorRect();
	if (IsRectEmpty(&mediaMonitorRect))
	{
		result.AllOk = false;
		result.ErrorMessage = L"Could not find target monitor!";
		return result;
	}

	UIA_HWND uiaHwnd{};
	const HRESULT hrNativeHwnd = findWindowsResult.Element->get_CurrentNativeWindowHandle(&uiaHwnd);
	if (FAILED(hrNativeHwnd) || uiaHwnd == nullptr)
	{
		result.AllOk = false;
		result.ErrorMessage = L"Could not get native window handle for Zoom media window.";
		return result;
	}
	const HWND hwnd = static_cast<HWND>(uiaHwnd);
	if (!IsWindow(hwnd))
	{
		result.AllOk = false;
		result.ErrorMessage = L"Native window handle is not a valid window.";
		return result;
	}

	RECT mediaWindowPos;
	const HRESULT hrRect = findWindowsResult.Element->get_CurrentBoundingRectangle(&mediaWindowPos);
	if (FAILED(hrRect))
	{
		result.AllOk = false;
		result.ErrorMessage = L"Could not get position of Zoom media window.";
		return result;
	}

	const RECT targetRect = CalculateTargetRect(mediaMonitorRect, hwnd);

	if (EqualRect(&targetRect, &mediaWindowPos))
	{
		// already displayed
		InternalHide(hwnd);
	}
	else
	{
		mediaWindowOriginalPosition_ = mediaWindowPos;

		if (fade)
		{
			InternalDisplay(hwnd, targetRect);
		}
		else
		{
			InternalDisplaySimple(hwnd, targetRect);
		}
	}

	return result;
}

RECT ZoomService::GetPrimaryMonitorRect()
{
	constexpr MonitorService monitorService;

	const std::vector<MonitorData> monitorData = monitorService.GetMonitorsData();
	for (auto& i : monitorData)
	{
		if (i.IsPrimary)
		{
			if (IsRectEmpty(&i.WorkRect))
			{
				return i.MonitorRect;
			}

			return i.WorkRect;
		}
	}

	return RECT{};
}

RECT ZoomService::CalculateTargetRect(const RECT mediaMonitorRect, const HWND mediaWindowHandle)
{
	RECT mediaWindowClientRect;
	GetClientRect(mediaWindowHandle, &mediaWindowClientRect);

	RECT mediaWindowRect;
	GetWindowRect(mediaWindowHandle, &mediaWindowRect);

	const auto clientWidth = mediaWindowClientRect.right - mediaWindowClientRect.left;
	const auto clientHeight = mediaWindowClientRect.bottom - mediaWindowClientRect.top;

	const auto windowWidth = mediaWindowRect.right - mediaWindowRect.left;
	const auto windowHeight = mediaWindowRect.bottom - mediaWindowRect.top;

	const auto xBorder = (windowWidth - clientWidth) / 2;
	const auto yBorder = (windowHeight - clientHeight) / 2;

	RECT result;
	result.left = mediaMonitorRect.left - xBorder;
	result.top = mediaMonitorRect.top - yBorder;
	result.right = mediaMonitorRect.right + (xBorder * 2);
	result.bottom = mediaMonitorRect.bottom + (yBorder * 2);

	return result;
}

void ZoomService::InternalHide(const HWND windowHandle)
{
	SetForegroundWindow(windowHandle);

	if (IsRectEmpty(&mediaWindowOriginalPosition_))
	{
		// fabricate a suitable location on the primary monitor
		const RECT primaryMonitorRect = GetPrimaryMonitorRect();

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

// Force foreground/top of the topmost band with an RAII guard to ensure detachment (try/finally semantics).
void ZoomService::ForceZoomWindowForeground(const HWND windowHandle)
{
	const HWND fg = GetForegroundWindow();
	const DWORD thisThread = GetCurrentThreadId();
	const DWORD fgThread = fg ? GetWindowThreadProcessId(fg, nullptr) : 0;
	const DWORD targetThread = GetWindowThreadProcessId(windowHandle, nullptr);

	struct AttachGuard  // NOLINT(cppcoreguidelines-special-member-functions)
	{
		DWORD Current{};
		DWORD ForegroundThread{};
		DWORD TargetThread{};
		bool AttachedForeground{ false };
		bool AttachedTarget{ false };

		~AttachGuard()
		{
			if (AttachedTarget)
			{
				AttachThreadInput(Current, TargetThread, FALSE);
			}

			if (AttachedForeground)
			{
				AttachThreadInput(Current, ForegroundThread, FALSE);
			}
		}
	} guard{ thisThread, fgThread, targetThread };

	if (fgThread && (fgThread != thisThread))
	{
		guard.AttachedForeground = AttachThreadInput(thisThread, fgThread, TRUE) != FALSE;
	}

	if (targetThread && (targetThread != thisThread))
	{
		guard.AttachedTarget = AttachThreadInput(thisThread, targetThread, TRUE) != FALSE;
	}

	BringWindowToTop(windowHandle);
	SetForegroundWindow(windowHandle);

	// Assert z-order again without moving/sizing.
	SetWindowPos(
		windowHandle,
		HWND_TOPMOST,
		0, 0, 0, 0,
		SWP_NOMOVE | SWP_NOSIZE | SWP_NOOWNERZORDER | SWP_NOSENDCHANGING);
}

void ZoomService::InternalDisplay(const HWND windowHandle, const RECT targetRect)
{
	if (!IsWindow(windowHandle))
	{
		return;
	}

	const int width = targetRect.right - targetRect.left;
	const int height = targetRect.bottom - targetRect.top;

	// Disable DWM transitions (min/restore/max animations) during our manual animation.
	BOOL disableTransitions = TRUE;
	DwmSetWindowAttribute(windowHandle, DWMWA_TRANSITIONS_FORCEDISABLED, &disableTransitions, sizeof(disableTransitions));

	// Cloak to avoid intermediate frames while moving/sizing (Win8+). Fallback to hide.
	const bool canCloak = SUCCEEDED(DwmSetWindowAttribute(windowHandle, DWMWA_CLOAK, &disableTransitions, sizeof(disableTransitions)));
	if (!canCloak)
	{
		ShowWindow(windowHandle, SW_HIDE);
	}

	// Reposition/resize while hidden/cloaked.
	SetWindowPos(
		windowHandle,
		HWND_TOPMOST,
		targetRect.left,
		targetRect.top,
		width,
		height,
		SWP_NOCOPYBITS | SWP_NOSENDCHANGING | SWP_NOACTIVATE);

	// Prepare fade-in: temporarily apply layered style and set alpha to 0.
	const LONG_PTR exStyle = GetWindowLongPtr(windowHandle, GWL_EXSTYLE);
	const bool hadLayered = (exStyle & WS_EX_LAYERED) != 0;
	if (!hadLayered)
	{
		SetWindowLongPtr(windowHandle, GWL_EXSTYLE, exStyle | WS_EX_LAYERED);
	}

	// If SetLayeredWindowAttributes fails, we'll still show without fade.
	const BOOL setAlpha0Ok = SetLayeredWindowAttributes(windowHandle, 0, 0 /* alpha */, LWA_ALPHA);

	// Uncloak or show and allow activation (no NOACTIVATE) so we can become topmost of the topmost band.
	if (canCloak)
	{
		constexpr BOOL uncloak = FALSE;
		DwmSetWindowAttribute(windowHandle, DWMWA_CLOAK, &uncloak, sizeof(uncloak));
		ShowWindow(windowHandle, SW_SHOW);
	}
	else
	{
		ShowWindow(windowHandle, SW_SHOW);
	}

	ForceZoomWindowForeground(windowHandle);

	// If alpha was set successfully, animate to full opacity.
	if (setAlpha0Ok)
	{
		const DWORD start = GetTickCount();
		BYTE alpha;

		do
		{
			constexpr DWORD durationMs = 300;
			const DWORD elapsed = GetTickCount() - start;
			const DWORD scaled = (elapsed >= durationMs) ? 255 : (elapsed * 255u) / durationMs;
			alpha = static_cast<BYTE>(scaled);
			SetLayeredWindowAttributes(windowHandle, 0, alpha, LWA_ALPHA);
			if (alpha < 255)
			{
				Sleep(10); // yield a bit for smoothness
			}
		} while (alpha < 255);

		// Return window to its original style if we added WS_EX_LAYERED.
		if (!hadLayered)
		{
			// Ensure fully opaque before removing the layered style.
			SetLayeredWindowAttributes(windowHandle, 0, 255, LWA_ALPHA);
			SetWindowLongPtr(windowHandle, GWL_EXSTYLE, exStyle);
		}
	}

	// Ensure a clean repaint after showing.
	RedrawWindow(windowHandle, nullptr, nullptr, RDW_INVALIDATE | RDW_ALLCHILDREN | RDW_UPDATENOW);

	// Re-enable DWM transitions.
	disableTransitions = FALSE;
	DwmSetWindowAttribute(windowHandle, DWMWA_TRANSITIONS_FORCEDISABLED, &disableTransitions, sizeof(disableTransitions));
}

void ZoomService::InternalDisplaySimple(const HWND windowHandle, const RECT targetRect)
{
	ShowWindow(windowHandle, SW_NORMAL);
	SetForegroundWindow(windowHandle);

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
	const SettingsService settingsService;
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

	const std::vector<std::unique_ptr<void, HandleDeleter>> zoomProcesses = 
		processesService_->GetProcessesByName(ZoomProcessName);

	if (zoomProcesses.empty())
	{
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

IUIAutomationElement* ZoomService::LocateZoomMediaWindow() const
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
