// ProjectorSwitch.cpp : Defines the entry point for the application.

#include "framework.h"
#include <commctrl.h> // Include the header for common controls
#include <atlbase.h>
#include <string>
#include <memory>
#include <vector>
#include "ProjectorSwitch.h"
#include "MonitorService.h"
#include "SettingsService.h"
#include "ZoomService.h"
#include "WindowPlacementService.h"

constexpr int MaxLoadStringLength = 100;
constexpr int BaseDpi = 96;
constexpr int ButtonWidth = 200;
constexpr int ButtonHeight = 60;
constexpr int ComboBoxWidth = 200;
constexpr int ComboBoxHeight = 20;
constexpr int ButtonId = 10001;
constexpr int ComboBoxId = 10002;

#pragma comment(linker,"\"/manifestdependency:type='win32' \
name='Microsoft.Windows.Common-Controls' version='6.0.0.0' \
processorArchitecture='*' publicKeyToken='6595b64144ccf1df' language='*'\"")

#pragma comment(lib, "ComCtl32.lib") // Link the ComCtl32 library

namespace
{
	// Global Variables (internal linkage):
	HINSTANCE CurrentInstance;
	WCHAR TitleBarCaption[MaxLoadStringLength];
	WCHAR MainWindowClass[MaxLoadStringLength];
	HWND BtnHandle;
	HWND ComboBoxHandle;
	HFONT ModernFont;
	HWND MainWindowHandle;
	std::vector<MonitorData> TheMonitorData;
	std::unique_ptr<ZoomService> TheZoomService;
	const std::wstring AppName = L"ApcProjSw";
	UINT CurrentDpi = BaseDpi;

	/// <summary>
	/// Scales an integer value based on the current DPI.
	/// </summary>
	/// <param name="value">The value to scale.</param>
	/// <returns>The scaled value.</returns>
	int Scale(const int value)
	{
		return MulDiv(value, static_cast<int>(CurrentDpi), BaseDpi);
	}

	/// <summary>
	/// Saves main window position to settings
	/// </summary>
	/// <param name="hWnd">Main window handle</param>
	void SaveWindowPosition(const HWND hWnd)
	{
		SettingsService ss;
		const WindowPlacementService wps(&ss);
		wps.SaveWindowPlace(hWnd);
	}

	/// <summary>
	/// Restores main window position from settings
	/// </summary>
	/// <param name="hwnd">Main window handle</param>
	void RestoreWindowPosition(const HWND hwnd)
	{
		SettingsService ss;
		const WindowPlacementService wps(&ss);
		wps.RestoreWindowPlace(hwnd);
	}

	/// <summary>
	/// Handles resizing of the main window, rearranging controls as needed
	/// </summary>
	/// <param name="hwnd">Main window handle (unused)</param>
	/// <param name="lParam">lParam value (new width and height)</param>
	/// <summary>
	/// Handles resizing of the main window, rearranging controls as needed
	/// </summary>
	/// <param name="hwnd">Main window handle (unused)</param>
	/// <param name="lParam">lParam value (new width and height)</param>
	void HandleResize(HWND hwnd, const LPARAM lParam)
	{
		if (!BtnHandle || !ComboBoxHandle) return;

		const UINT width = LOWORD(lParam);
		const UINT height = HIWORD(lParam);
		UNREFERENCED_PARAMETER(height);

		// Always compute sizes from logical DIPs so they rescale with DPI
		const int btnW = Scale(ButtonWidth);
		const int btnH = Scale(ButtonHeight);
		const int comboW = Scale(ComboBoxWidth);
		const int comboH = Scale(ComboBoxHeight);

		// Center and position with scaled margins
		int x = static_cast<int>(width - btnW) / 2;
		int y = Scale(10);
		MoveWindow(BtnHandle, x, y, btnW, btnH, TRUE);

		x = static_cast<int>(width - comboW) / 2;
		y = y + btnH + Scale(20);
		MoveWindow(ComboBoxHandle, x, y, comboW, comboH, TRUE);
	}

	/// <summary>
	/// Adds monitor entries to the combo box
	/// </summary>
	/// <param name="comboHandle">ComboBox handle</param>
	void AddMonitorsToCombo(const HWND comboHandle)
	{
		constexpr MonitorService ms;
		TheMonitorData = ms.GetMonitorsData();

		for (const auto& i : TheMonitorData)
		{
			const std::wstring displayText = i.GetDisplayName();
			SendMessage(comboHandle, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(displayText.c_str()));
		}

		// Show ~8 items when dropped (no need to inflate the control's selection height)
		SendMessage(comboHandle, CB_SETMINVISIBLE, 8, 0);
	}

	/// <summary>
	/// Creates the toggle button
	/// </summary>
	/// <param name="parent">Parent handle</param>
	/// <returns>Button handle</returns>
	HWND CreateButton(const HWND parent)
	{
		return CreateWindowW(
			L"BUTTON",
			L"Toggle Zoom Window",
			WS_TABSTOP | WS_VISIBLE | WS_CHILD | WS_DISABLED,
			Scale(10), // x position 
			Scale(10), // y position 
			Scale(ButtonWidth),
			Scale(ButtonHeight),
			parent,
			reinterpret_cast<HMENU>(static_cast<UINT_PTR>(ButtonId)), // NOLINT(performance-no-int-to-ptr)
			reinterpret_cast<HINSTANCE>(GetWindowLongPtr(parent, GWLP_HINSTANCE)), // NOLINT(performance-no-int-to-ptr)
			nullptr); // Pointer not needed.
	}

	/// <summary>
	/// Responds to selection of a monitor in the combo box
	/// </summary>
	/// <param name="comboHandle">ComboBox handle</param>
	void SelectMonitor(const HWND comboHandle)
	{
		const SettingsService ss;
		const std::wstring savedKey = ss.LoadSelectedMonitorKey();
		const RECT savedRect = ss.LoadSelectedMonitorRect();

		const int index = MonitorService::FindMonitorIndex(TheMonitorData, savedKey, savedRect);

		if (index >= 0)
		{
			SendMessage(comboHandle, CB_SETCURSEL, index, 0);
			EnableWindow(BtnHandle, TRUE);
		}
		else
		{
			// clear
			SendMessage(comboHandle, CB_SETCURSEL, static_cast<WPARAM>(-1), 0);
		}
	}

	/// <summary>
	/// Creates a screen font similar to modern Windows UI
	/// </summary>
	/// <returns>Font handle</returns>
	HFONT CreateModernFont()
	{
		// ~10pt Segoe UI; negative => character height
		constexpr int pointSize = 10;
		const int lfHeight = -MulDiv(pointSize, static_cast<int>(CurrentDpi), 72);
		return CreateFont(
			lfHeight, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
			DEFAULT_CHARSET, OUT_OUTLINE_PRECIS,
			CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY, DEFAULT_PITCH,
			L"Segoe UI");
	}

	/// <summary>
	/// Sets a modern font for the controls
	/// </summary>	
	void SetModernFont()
	{
		if (ModernFont)
		{
			DeleteObject(ModernFont);
		}
		ModernFont = CreateModernFont();

		if (ModernFont)
		{
			// WM_SETFONT is meaningful for child controls
			if (BtnHandle) SendMessage(BtnHandle, WM_SETFONT, reinterpret_cast<WPARAM>(ModernFont), TRUE);
			if (ComboBoxHandle) SendMessage(ComboBoxHandle, WM_SETFONT, reinterpret_cast<WPARAM>(ModernFont), TRUE);
		}
	}

	/// <summary>
	/// Creates the combo box for monitor selection
	/// </summary>
	/// <param name="parent">Parent handle</param>
	/// <returns>ComboBox handle</returns>
	HWND CreateComboBox(const HWND parent)
	{
		const auto result = CreateWindowW(
			L"COMBOBOX",
			nullptr,
			WS_TABSTOP | WS_VISIBLE | WS_CHILD | CBS_DROPDOWNLIST | WS_VSCROLL,
			Scale(10),   // x
			Scale(100),  // y
			Scale(ComboBoxWidth),
			Scale(ComboBoxHeight), // single-line selection; dropdown height handled by CB_SETMINVISIBLE
			parent,
			reinterpret_cast<HMENU>(static_cast<UINT_PTR>(ComboBoxId)),  // NOLINT(performance-no-int-to-ptr)
			reinterpret_cast<HINSTANCE>(GetWindowLongPtr(parent, GWLP_HINSTANCE)), // NOLINT(performance-no-int-to-ptr)
			nullptr);

		AddMonitorsToCombo(result);
		SelectMonitor(result);
		return result;
	}

	/// <summary>
	/// Toggle location of Zoom secondary window
	/// </summary>
	void ToggleZoomWindow()
	{
		if (TheZoomService)
		{
			TheZoomService->Toggle(true);

			// Keep window topmost after toggling
			SetWindowPos(MainWindowHandle, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);
		}
	}

	/// <summary>
	/// Saves the selected monitor ID to settings
	/// </summary>
	/// <param name="selectedIndex">Index in ComboBox</param>
	void SaveSelectedMonitorId(const int selectedIndex)
	{
		if (selectedIndex < 0 || static_cast<size_t>(selectedIndex) >= TheMonitorData.size())
		{
			return;
		}

		const MonitorData& md = TheMonitorData[static_cast<size_t>(selectedIndex)];
		
		// Save both the robust key and the RECT for legacy behavior
		const SettingsService ss;
		ss.SaveSelectedMonitorKey(md.Key);
		ss.SaveSelectedMonitorRect(md.MonitorRect);
	}

	/// <summary>
	/// Window procedure for the main window
	/// </summary>
	/// <param name="hWnd">Main window handle</param>
	/// <param name="message">Message</param>
	/// <param name="wParam">wParam</param>
	/// <param name="lParam">lParam</param>
	/// <returns>Result</returns>
	LRESULT CALLBACK WndProc(const HWND hWnd, const UINT message, const WPARAM wParam, const LPARAM lParam)
	{
		switch (message)
		{
		case WM_DPICHANGED:
		{
			CurrentDpi = HIWORD(wParam);
			SetModernFont();

			const auto* const prcNewWindow = reinterpret_cast<RECT*>(lParam);  // NOLINT(performance-no-int-to-ptr)
			SetWindowPos(hWnd,
				nullptr,
				prcNewWindow->left,
				prcNewWindow->top,
				prcNewWindow->right - prcNewWindow->left,
				prcNewWindow->bottom - prcNewWindow->top,
				SWP_NOZORDER | SWP_NOACTIVATE);

			// Recompute layout at the new DPI using current client size
			RECT rcClient{};
			if (GetClientRect(hWnd, &rcClient))
			{
				const LPARAM sizeParam = MAKELPARAM(rcClient.right - rcClient.left, rcClient.bottom - rcClient.top);
				HandleResize(hWnd, sizeParam);
			}
			break;
		}
		case WM_SIZE:
		{
			HandleResize(hWnd, lParam);
			break;
		}

		case WM_GETMINMAXINFO:
		{
			const auto lpMmi = reinterpret_cast<LPMINMAXINFO>(lParam);  // NOLINT(performance-no-int-to-ptr)
			lpMmi->ptMinTrackSize.x = Scale(100);
			lpMmi->ptMinTrackSize.y = Scale(100);
			break;
		}

		case WM_CREATE:
			CurrentDpi = GetDpiForWindow(hWnd);
			BtnHandle = CreateButton(hWnd);
			ComboBoxHandle = CreateComboBox(hWnd);
			SetModernFont();
			TheZoomService = std::unique_ptr<ZoomService>(new ZoomService(new AutomationService(), new ProcessesService()));
			break;

		case WM_COMMAND:
		{
			switch (HIWORD(wParam))
			{
			case BN_CLICKED:
				if (LOWORD(wParam) == ButtonId)
				{
					ToggleZoomWindow();
				}
				break;

			case CBN_SELCHANGE:
			{
				if (LOWORD(wParam) == ComboBoxId)
				{
					const int selectedIndex = static_cast<int>(SendMessage(ComboBoxHandle, CB_GETCURSEL, 0, 0));
					if (selectedIndex != CB_ERR)
					{
						SaveSelectedMonitorId(selectedIndex);
						EnableWindow(BtnHandle, TRUE);
					}
				}
				break;
			}

			default:
				break;
			}
		}
		break;

		case WM_PAINT:
		{
			PAINTSTRUCT ps;
			const HDC hdc = BeginPaint(hWnd, &ps);
			UNREFERENCED_PARAMETER(hdc);
			EndPaint(hWnd, &ps);
		}
		break;

		case WM_DESTROY:
			SaveWindowPosition(hWnd);
			if (ModernFont)
			{
				DeleteObject(ModernFont);
				ModernFont = nullptr;
			}
			PostQuitMessage(0);
			break;

		default:
			return DefWindowProc(hWnd, message, wParam, lParam);
		}

		return 0;
	}

	ATOM ProjectSwitchRegisterClass(const HINSTANCE hInstance)
	{
		// ReSharper disable once CppInitializedValueIsAlwaysRewritten
		WNDCLASSEXW windowsClassEx{};

		windowsClassEx.cbSize = sizeof(WNDCLASSEX);

		windowsClassEx.style = CS_HREDRAW | CS_VREDRAW;
		windowsClassEx.lpfnWndProc = WndProc;
		windowsClassEx.cbClsExtra = 0;
		windowsClassEx.cbWndExtra = 0;
		windowsClassEx.hInstance = hInstance;
		windowsClassEx.hIcon = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_PROJECTOR_SWITCH));
		windowsClassEx.hCursor = LoadCursor(nullptr, IDC_ARROW);
		windowsClassEx.hbrBackground = reinterpret_cast<HBRUSH>(COLOR_WINDOW + 1);  // NOLINT(performance-no-int-to-ptr)
		windowsClassEx.lpszMenuName = nullptr;
		windowsClassEx.lpszClassName = MainWindowClass;
		windowsClassEx.hIconSm = nullptr;

		return RegisterClassExW(&windowsClassEx);
	}

	BOOL InitInstance(const HINSTANCE hInstance, const int nCmdShow)
	{
		CurrentInstance = hInstance;

		// Use system DPI to compute a reasonable initial size; WM_CREATE updates CurrentDpi to window DPI.
		CurrentDpi = GetDpiForSystem();

		// Desired client size in DIPs based on content layout
		constexpr int topMarginDip = 10;
		constexpr int spacingDip = 20;
		constexpr int bottomDip = 20;
		constexpr int clientDipW = 240;
		constexpr int clientDipH = topMarginDip + ButtonHeight + spacingDip + ComboBoxHeight + bottomDip; // 10 + 60 + 20 + 20 + 10 = 120

		// Convert to pixels at the current DPI
		const int clientPxW = MulDiv(clientDipW, static_cast<int>(CurrentDpi), BaseDpi);
		const int clientPxH = MulDiv(clientDipH, static_cast<int>(CurrentDpi), BaseDpi);

		constexpr DWORD exStyle = WS_EX_TOPMOST;
		constexpr DWORD style = ((WS_OVERLAPPEDWINDOW | WS_CLIPCHILDREN) & ~(WS_MINIMIZEBOX | WS_MAXIMIZEBOX | WS_THICKFRAME));

		RECT rc = { 0, 0, clientPxW, clientPxH };

		// Account for non-client (caption/borders) at this DPI
		using PFN_AdjustWindowRectExForDpi = BOOL(WINAPI*)(LPRECT, DWORD, BOOL, DWORD, UINT);
		if (auto user32 = GetModuleHandleW(L"user32.dll"))
		{
			const auto pAdjust = reinterpret_cast<PFN_AdjustWindowRectExForDpi>(  // NOLINT(clang-diagnostic-cast-function-type-strict)
				GetProcAddress(user32, "AdjustWindowRectExForDpi"));

			if (pAdjust) pAdjust(&rc, style, FALSE, exStyle, CurrentDpi);
			else AdjustWindowRectEx(&rc, style, FALSE, exStyle);
		}
		else
		{
			AdjustWindowRectEx(&rc, style, FALSE, exStyle);
		}

		const int winW = rc.right - rc.left;
		const int winH = rc.bottom - rc.top;

		MainWindowHandle = CreateWindowExW(
			exStyle,
			MainWindowClass,
			TitleBarCaption,
			style,
			CW_USEDEFAULT,
			CW_USEDEFAULT,
			winW,
			winH,
			nullptr,
			nullptr,
			hInstance,
			nullptr);

		if (!MainWindowHandle)
		{
			return FALSE;
		}

		RestoreWindowPosition(MainWindowHandle);

		ShowWindow(MainWindowHandle, nCmdShow);
		UpdateWindow(MainWindowHandle);

		return TRUE;
	}
} // end anonymous namespace

// Main entry point for application
int APIENTRY wWinMain(
	_In_ HINSTANCE hInstance,
	_In_opt_ HINSTANCE hPrevInstance,
	_In_ LPWSTR lpCmdLine,
	_In_ int nCmdShow)
{
	UNREFERENCED_PARAMETER(hPrevInstance);
	UNREFERENCED_PARAMETER(lpCmdLine);

	// Set DPI awareness to per-monitor v2
	SetProcessDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);

	// options to disable DPI awareness if desired (as an alternative to PMV2 above):
	// Option 1: completely DPI-unaware (bitmap-scaled, blur on >100%)
	// SetProcessDpiAwarenessContext(DPI_AWARENESS_CONTEXT_UNAWARE);
	// Option 2: DPI-unaware (GDI-scaled) on Win10+ for slightly better scaling when moving across monitors
	// SetProcessDpiAwarenessContext(DPI_AWARENESS_CONTEXT_UNAWARE_GDISCALED);

	CHandle appMutex(CreateMutex(nullptr, TRUE, AppName.c_str()));
	if (GetLastError() == ERROR_ALREADY_EXISTS)
	{
		return FALSE;
	}

	// Initialize common controls
	// ReSharper disable once CppInitializedValueIsAlwaysRewritten
	INITCOMMONCONTROLSEX commonCtrlsEx{};
	commonCtrlsEx.dwSize = sizeof(INITCOMMONCONTROLSEX);
	commonCtrlsEx.dwICC = ICC_WIN95_CLASSES;
	InitCommonControlsEx(&commonCtrlsEx);

	// Initialize global strings
	LoadStringW(hInstance, IDS_APP_TITLE, TitleBarCaption, MaxLoadStringLength);
	LoadStringW(hInstance, IDC_PROJECTOR_SWITCH, MainWindowClass, MaxLoadStringLength);
	ProjectSwitchRegisterClass(hInstance);

	// Perform application initialization:
	if (!InitInstance(hInstance, nCmdShow))
	{
		return FALSE;
	}

	const HACCEL hAccelTable = LoadAccelerators(hInstance, MAKEINTRESOURCE(IDC_PROJECTOR_SWITCH));

	MSG msg;

	// Main message loop:
	while (GetMessage(&msg, nullptr, 0, 0))
	{
		// Use the main window handle for accelerator translation
		if (!TranslateAccelerator(MainWindowHandle, hAccelTable, &msg))
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
	}

	return static_cast<int>(msg.wParam);
}