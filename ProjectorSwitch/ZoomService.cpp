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

/// <summary>
/// Constructs a ZoomService object and initializes its member variables.
/// </summary>
/// <param name="automationService">Pointer to an AutomationService instance used by the ZoomService.</param>
/// <param name="processesService">Pointer to a ProcessesService instance used by the ZoomService.</param>
ZoomService::ZoomService(AutomationService* automationService, ProcessesService *processesService)
	: mediaWindowOriginalPosition_({ 0,0,0,0 })
	, mediaWindowWasMinimized_(false)
	, cachedDesktopWindow_(nullptr)
	, automationService_(automationService)
	, processesService_(processesService)	
{	
}

/// <summary>
/// Destroys the ZoomService object and releases associated resources.
/// </summary>
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

/// <summary>
/// Toggles the display state of the Zoom media window, optionally using a fade effect.
/// Handles error conditions such as Zoom not running or the media window not being found.
/// </summary>
/// <returns>
/// A DisplayWindowResult object indicating whether the operation was successful
/// and containing an error message if it failed.
/// </returns>
DisplayWindowResult ZoomService::Toggle()
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
		mediaWindowWasMinimized_ = IsIconic(hwnd) != FALSE;
		mediaWindowOriginalPosition_ = mediaWindowPos;
		InternalDisplay(hwnd, targetRect);
	}

	return result;
}

/// <summary>
/// Retrieves the rectangle of the primary monitor, preferring the work area if available.
/// </summary>
/// <returns>
/// A RECT structure representing the work area of the primary monitor if it is not empty;
/// otherwise, the full monitor rectangle. Returns an empty RECT if no primary monitor
/// is found.
/// </returns>
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

/// <summary>
/// Calculates the target rectangle for a media window by adjusting the given monitor rectangle
/// to account for the window's non-client borders.
/// </summary>
/// <param name="mediaMonitorRect">The rectangle representing the area of the monitor where
/// the media window is displayed.</param>
/// <param name="mediaWindowHandle">The handle to the media window whose borders are to
/// be considered.</param>
/// <returns>
/// A RECT structure representing the adjusted rectangle that includes the window's borders.
/// </returns>
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

	const RECT result
	{
		mediaMonitorRect.left - xBorder,
		mediaMonitorRect.top - yBorder,
		mediaMonitorRect.right + (xBorder * 2),
		mediaMonitorRect.bottom + (yBorder * 2)
	};

	return result;
}

/// <summary>
/// Hides or repositions the specified window, ensuring it is placed at a suitable location
/// if its original position is not set.
/// </summary>
/// <param name="windowHandle">Handle to the window to be hidden or repositioned.</param>
void ZoomService::InternalHide(const HWND windowHandle)
{
	// If it was originally minimized, minimize again, but make sure its normal position is on the primary monitor.
	if (mediaWindowWasMinimized_)
	{
		// Place the window's normal (restore) position on the primary monitor so future restores happen there.
		const RECT primaryMonitorRect = GetPrimaryMonitorRect();

		RECT restoreRect{};
		restoreRect.left = primaryMonitorRect.left + 10;
		restoreRect.top = primaryMonitorRect.top + 10;
		restoreRect.right = restoreRect.left + 450;
		restoreRect.bottom = restoreRect.top + 300;

		WINDOWPLACEMENT wp{};
		wp.length = sizeof(wp);
		if (GetWindowPlacement(windowHandle, &wp))
		{
			wp.showCmd = SW_SHOWNORMAL;               // define where it should restore to
			wp.rcNormalPosition = restoreRect;             // set restore rect on primary
			SetWindowPlacement(windowHandle, &wp);
		}

		// Drop out of topmost band before minimizing.
		SetWindowPos(
			windowHandle,
			HWND_NOTOPMOST,
			0, 0, 0, 0,
			SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE | SWP_NOSENDCHANGING);

		ShowWindowAsync(windowHandle, SW_MINIMIZE);
		mediaWindowWasMinimized_ = false;
		return;
	}

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

/// <summary>
/// Force the specified window to the foreground, even if our process is not foreground. This
/// helps to ensure the Zoom window is on top of all other windows, even if another
/// topmost window has focus.
/// </summary>
/// <param name="windowHandle"></param>
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

/// <summary>
/// Displays a window at a specified position and size with a smooth fade-in animation, handling DWM
/// transitions and window cloaking for a seamless visual effect.
/// </summary>
/// <param name="windowHandle">Handle to the window to be displayed and animated.</param>
/// <param name="targetRect">The target rectangle specifying the desired position and size
/// of the window, in screen coordinates.</param>
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

	// If minimized, restore first so SetWindowPos will take effect correctly.
	if (IsIconic(windowHandle))
	{
		ShowWindowAsync(windowHandle, SW_RESTORE);

		// Wait briefly for the window to leave the iconic state.
		DWORD waited = 0;
		constexpr DWORD maxWaitMs = 500;
		while (IsIconic(windowHandle) && waited < maxWaitMs)
		{
			Sleep(10);
			waited += 10;
		}
	}

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
		const ULONGLONG start = GetTickCount64();
		BYTE alpha;

		do
		{
			constexpr DWORD durationMs = 300;
			const ULONGLONG elapsed = GetTickCount64() - start;
			const ULONGLONG scaled = (elapsed >= durationMs) ? 255 : (elapsed * 255u) / durationMs;
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

/// <summary>
/// Retrieves the rectangle of the target monitor as specified in the settings.
/// </summary>
/// <returns>
/// A RECT structure representing the area of the selected monitor.
/// </returns>
RECT ZoomService::GetTargetMonitorRect()
{
	const SettingsService settingsService;
	return settingsService.LoadSelectedMonitorRect();
}

/// <summary>
/// Attempts to locate the Zoom media window and returns the result, including status and
/// any found elements.
/// </summary>
/// <returns>
/// A FindWindowsResult structure containing information about whether the desktop and Zoom
/// media window were found, if Zoom is running, and any error messages.
/// </returns>
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

/// <summary>
/// Attempts to locate the Zoom media window.
/// </summary>
/// <returns>An IUIAutomationElement representing the Zoom media window, or nullptr if not found.</returns>
IUIAutomationElement* ZoomService::LocateZoomMediaWindow() const
{
	if (cachedDesktopWindow_ == nullptr)
	{
		return nullptr;
	}

	// Find the Zoom media window by searching for the specific class name and name.
	// The class name and name may vary based on the Zoom version and configuration!
	
	IUIAutomation* pAutomation = automationService_->GetAutomationInterface();
	
	VariantWrapper varName;
	varName.SetString(L"Zoom Meeting");

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
