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

	/// <summary>
	/// Saves main window position to settings
	/// </summary>
	/// <param name="hWnd">Main window handle</param>
	void SaveWindowPosition(const HWND hWnd)
	{
		SettingsService ss;
		WindowPlacementService wps(&ss);
		wps.SaveWindowPlace(hWnd);
	}

	/// <summary>
	/// Restores main window position from settings
	/// </summary>
	/// <param name="hwnd">Main window handle</param>
	void RestoreWindowPosition(const HWND hwnd)
	{
		SettingsService ss;
		WindowPlacementService wps(&ss);
		wps.RestoreWindowPlace(hwnd);
	}

	/// <summary>
	/// Handles resizing of the main window, rearranging controls as needed
	/// </summary>
	/// <param name="hwnd">Main window handle (unused)</param>
	/// <param name="lParam">lParam value (new width and height)</param>
	void HandleResize(HWND hwnd, const LPARAM lParam)
	{
		if (!BtnHandle || !ComboBoxHandle)
		{
			return;
		}

		const UINT width = LOWORD(lParam);
		const UINT height = HIWORD(lParam);
		UNREFERENCED_PARAMETER(height);

		RECT rcBtn = {};
		RECT rcCombo = {};
		GetClientRect(BtnHandle, &rcBtn);
		GetClientRect(ComboBoxHandle, &rcCombo);

		// arrange the button
		int x = static_cast<int>(width - (rcBtn.right - rcBtn.left)) / 2;
		int y = 10;
		MoveWindow(BtnHandle, x, y, rcBtn.right - rcBtn.left, rcBtn.bottom - rcBtn.top, TRUE);

		// arrange the combo
		x = static_cast<int>(width - (rcCombo.right - rcCombo.left)) / 2;
		y = rcBtn.bottom + 20;
		MoveWindow(ComboBoxHandle, x, y, rcCombo.right - rcCombo.left, rcCombo.bottom - rcCombo.top, TRUE);
	}

	/// <summary>
	/// Gets the RECT of the primary monitor from the monitor data list
	/// </summary>
	/// <param name="data">Monitor data</param>
	/// <returns>RECT of primary monitor or (0,0,0,0) if not found</returns>
	RECT GetPrimaryRectFromData(const std::vector<MonitorData>& data)
	{
		for (const auto& m : data)
		{
			if (m.IsPrimary)
			{
				return IsRectEmpty(&m.WorkRect) ? m.MonitorRect : m.WorkRect;
			}
		}

		return RECT{ 0, 0, 0, 0 };
	}

	/// <summary>
	/// Describes the position of a monitor RECT relative to the primary monitor RECT
	/// </summary>
	/// <param name="r">Monitor RECT</param>
	/// <param name="primary">Primary monitor RECT</param>
	/// <returns>String describing the position</returns>
	std::wstring DescribePosition(const RECT& r, const RECT& primary)
	{
		// For primary itself
		if (EqualRect(&r, &primary))
		{
			return L"primary";
		}

		if (r.right <= primary.left)
		{
			return L"left";
		}

		if (r.left >= primary.right)
		{
			return L"right";
		}

		if (r.bottom <= primary.top)
		{
			return L"above";
		}

		if (r.top >= primary.bottom)
		{
			return L"below";
		}

		return L"overlap";
	}

	/// <summary>
	/// Adds monitor entries to the combo box
	/// </summary>
	/// <param name="comboHandle">ComboBox handle</param>
	void AddMonitorsToCombo(const HWND comboHandle)
	{
		constexpr MonitorService ms;
		TheMonitorData = ms.GetMonitorsData();

		const RECT primaryRect = GetPrimaryRectFromData(TheMonitorData);

		for (const auto& i : TheMonitorData)
		{
			const RECT& r = i.MonitorRect;

			// Position relative to primary
			const std::wstring pos = DescribePosition(r, primaryRect);

			// If it's the primary monitor, show "primary"; otherwise show the relative position
			const std::wstring status = i.IsPrimary ? L"primary" : pos;

			std::wstring displayText = i.FriendlyName;
			displayText += L" (";
			displayText += status;
			displayText += L")";

			SendMessage(comboHandle, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(displayText.c_str()));
		}
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
			10, // x position 
			10, // y position 
			ButtonWidth,
			ButtonHeight,
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
		SettingsService ss;

		// Prefer robust key matching
		const std::wstring savedKey = ss.LoadSelectedMonitorKey();

		bool matched = false;

		if (!savedKey.empty())
		{
			const bool isSerialKey = savedKey.rfind(L"SERIAL:", 0) == 0;
			const bool isPathKey = savedKey.rfind(L"PATH:", 0) == 0;

			if (isSerialKey)
			{
				const std::wstring targetSerial = savedKey.substr(7);
				int index = 0;
				for (const auto& i : TheMonitorData)
				{
					const std::wstring serial = MonitorService::TryGetMonitorSerialFromDevicePath(i.DevicePath);
					if (!serial.empty() && _wcsicmp(serial.c_str(), targetSerial.c_str()) == 0)
					{
						SendMessage(comboHandle, CB_SETCURSEL, index, 0);
						matched = true;
						EnableWindow(BtnHandle, TRUE);
						break;
					}
					++index;
				}
			}
			else if (isPathKey)
			{
				const std::wstring targetPath = savedKey.substr(5);
				int index = 0;
				for (const auto& i : TheMonitorData)
				{
					if (_wcsicmp(i.DevicePath.c_str(), targetPath.c_str()) == 0)
					{
						SendMessage(comboHandle, CB_SETCURSEL, index, 0);
						matched = true;
						EnableWindow(BtnHandle, TRUE);
						break;
					}
					++index;
				}
			}
		}

		if (!matched)
		{
			// Legacy fallback using persisted RECT
			const RECT rect = ss.LoadSelectedMonitorRect();

			int index = 0;
			for (const auto& i : TheMonitorData)
			{
				if (EqualRect(&i.MonitorRect, &rect))
				{
					SendMessage(comboHandle, CB_SETCURSEL, index, 0);
					matched = true;
					EnableWindow(BtnHandle, TRUE);
					break;
				}
				++index;
			}
		}

		if (!matched)
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
		return CreateFont(
			16, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
			DEFAULT_CHARSET, OUT_OUTLINE_PRECIS,
			CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY, DEFAULT_PITCH,
			L"Segoe UI");
	}

	/// <summary>
	/// Sets a modern font for the controls
	/// </summary>	
	void SetModernFont()
	{
		if (!ModernFont)
		{
			ModernFont = CreateModernFont();
		}

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
		auto result = CreateWindowW(
			L"COMBOBOX",
			nullptr,
			WS_TABSTOP | WS_VISIBLE | WS_CHILD | CBS_DROPDOWNLIST,
			10,   // x position 
			100,  // y position 
			ComboBoxWidth,  // width
			ComboBoxHeight, // height
			parent, // Parent window
			reinterpret_cast<HMENU>(static_cast<UINT_PTR>(ComboBoxId)),  // NOLINT(performance-no-int-to-ptr)
			reinterpret_cast<HINSTANCE>(GetWindowLongPtr(parent, GWLP_HINSTANCE)), // NOLINT(performance-no-int-to-ptr)
			nullptr); // Pointer not needed.

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
		SettingsService ss;

		// Prefer serial-based identity if available; fallback to device path
		const std::wstring serial = MonitorService::TryGetMonitorSerialFromDevicePath(md.DevicePath);
		std::wstring key;
		if (!serial.empty())
		{
			key = L"SERIAL:" + serial;
		}
		else
		{
			key = L"PATH:" + md.DevicePath;
		}

		// Save both the robust key and the RECT for legacy behavior
		ss.SaveSelectedMonitorKey(key);
		ss.SaveSelectedMonitorRect(md.MonitorRect);
	}

	/// <summary>
	/// Window procedure for the main window
	/// </summary>
	/// <param name="hWnd">Main window handle</param>
	/// <param name="message">Message</param>
	/// <param name="wParam">wParam</param>
	/// <param name="lParam">lParam</param>
	/// <returns>LRESULT value</returns>
	LRESULT CALLBACK WndProc(const HWND hWnd, const UINT message, const WPARAM wParam, const LPARAM lParam)
	{
		switch (message)
		{
		case WM_SIZE:
		{
			HandleResize(hWnd, lParam);
			break;
		}

		case WM_GETMINMAXINFO:
		{
			const auto lpMMI = reinterpret_cast<LPMINMAXINFO>(lParam);  // NOLINT(performance-no-int-to-ptr)
			lpMMI->ptMinTrackSize.x = 100;
			lpMMI->ptMinTrackSize.y = 100;
			break;
		}

		case WM_CREATE:
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
		CurrentInstance = hInstance; // Store instance handle in our global variable

		MainWindowHandle = CreateWindowExW(
			WS_EX_TOPMOST,
			MainWindowClass,
			TitleBarCaption,			
			((WS_OVERLAPPEDWINDOW | WS_CLIPCHILDREN) & ~(WS_MINIMIZEBOX | WS_MAXIMIZEBOX | WS_THICKFRAME)),
			CW_USEDEFAULT,
			CW_USEDEFAULT,
			240, // width
			155, // height
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
