// ProjectorSwitch.cpp : Defines the entry point for the application.

#include "framework.h"
#include <commctrl.h> // Include the header for common controls
#include <strsafe.h>
#include <string>
#include <memory>
#include "ProjectorSwitch.h"
#include "MonitorService.h"
#include "SettingsService.h"
#include "ZoomService.h"
#include "WindowPlacementService.h"

constexpr int MAX_LOADSTRING = 100;
constexpr int ButtonWidth = 60;
constexpr int ButtonHeight = 60;
constexpr int ComboBoxWidth = 100;
constexpr int ComboBoxHeight = 20;
constexpr int ButtonId = 10001;
constexpr int ComboBoxId = 10002;

#pragma comment(linker,"\"/manifestdependency:type='win32' \
name='Microsoft.Windows.Common-Controls' version='6.0.0.0' \
processorArchitecture='*' publicKeyToken='6595b64144ccf1df' language='*'\"")

#pragma comment(lib, "ComCtl32.lib") // Link the ComCtl32 library

// Global Variables:
HINSTANCE CurrentInstance;
WCHAR TitleBarCaption[MAX_LOADSTRING];
WCHAR MainWindowClass[MAX_LOADSTRING];
HWND BtnHandle;
HWND ComboBoxHandle;
HFONT ModernFont;
std::vector<MonitorData> TheMonitorData;
std::unique_ptr<ZoomService> TheZoomService;
std::unique_ptr<DisplayWindowResult> TheDisplayWindowResult;

// Forward declarations of functions included in this code module:
ATOM ProjectSwitchRegisterClass(HINSTANCE hInstance);
BOOL InitInstance(HINSTANCE, int);
LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);

int APIENTRY wWinMain(_In_ HINSTANCE hInstance,
	_In_opt_ HINSTANCE hPrevInstance,
	_In_ LPWSTR    lpCmdLine,
	_In_ int       nCmdShow)
{
	UNREFERENCED_PARAMETER(hPrevInstance);
	UNREFERENCED_PARAMETER(lpCmdLine);

	// Initialize common controls
	INITCOMMONCONTROLSEX icex;
	icex.dwSize = sizeof(INITCOMMONCONTROLSEX);
	icex.dwICC = ICC_WIN95_CLASSES;
	InitCommonControlsEx(&icex);

	// Initialize global strings
	LoadStringW(hInstance, IDS_APP_TITLE, TitleBarCaption, MAX_LOADSTRING);
	LoadStringW(hInstance, IDC_PROJECTOR_SWITCH, MainWindowClass, MAX_LOADSTRING);
	ProjectSwitchRegisterClass(hInstance);

	// Perform application initialization:
	if (!InitInstance(hInstance, nCmdShow))
	{
		return FALSE;
	}

	auto hAccelTable = LoadAccelerators(hInstance, MAKEINTRESOURCE(IDC_PROJECTOR_SWITCH));

	MSG msg;

	// Main message loop:
	while (GetMessage(&msg, nullptr, 0, 0))
	{
		if (!TranslateAccelerator(msg.hwnd, hAccelTable, &msg))
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
	}

	return (int)msg.wParam;
}

ATOM ProjectSwitchRegisterClass(HINSTANCE hInstance)
{
	WNDCLASSEXW wcex;

	wcex.cbSize = sizeof(WNDCLASSEX);

	wcex.style = CS_HREDRAW | CS_VREDRAW;
	wcex.lpfnWndProc = WndProc;
	wcex.cbClsExtra = 0;
	wcex.cbWndExtra = 0;
	wcex.hInstance = hInstance;
	wcex.hIcon = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_PROJECTOR_SWITCH));
	wcex.hCursor = LoadCursor(nullptr, IDC_ARROW);
	wcex.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);	
	wcex.lpszMenuName = nullptr;
	wcex.lpszClassName = MainWindowClass;
	wcex.hIconSm = nullptr;
	
	return RegisterClassExW(&wcex);
}

void SaveWindowPosition(HWND hWnd)
{
	SettingsService ss;
	WindowPlacementService wps(&ss);
	wps.SaveWindowPlace(hWnd);
}

void RestoreWindowPosition(HWND hwnd)
{
	SettingsService ss;
	WindowPlacementService wps(&ss);
	wps.RestoreWindowPlace(hwnd);
}

BOOL InitInstance(HINSTANCE hInstance, int nCmdShow)
{
	CurrentInstance = hInstance; // Store instance handle in our global variable

	auto hWnd = CreateWindowW(
		MainWindowClass, 
		TitleBarCaption,
		(WS_OVERLAPPEDWINDOW | WS_CLIPCHILDREN) ^ WS_MINIMIZEBOX ^ WS_MAXIMIZEBOX ^ WS_THICKFRAME,
		CW_USEDEFAULT, 
		CW_USEDEFAULT, 		
		140,
		155, 
		nullptr, 
		nullptr, 
		hInstance, 
		nullptr);

	if (!hWnd)
	{
		return FALSE;
	}

	RestoreWindowPosition(hWnd);

	ShowWindow(hWnd, nCmdShow);
	UpdateWindow(hWnd);

	return TRUE;
}

static HWND CreateButton(HWND parent)
{
	return CreateWindowW(
		L"BUTTON",
		L"ZOOM",		
		WS_TABSTOP | WS_VISIBLE | WS_CHILD | WS_DISABLED,
		10, // x position 
		10, // y position 
		ButtonWidth, // Button width
		ButtonHeight, // Button height
		parent, // Parent window
		(HMENU)(UINT_PTR)(ButtonId), // Identifier for the button
		(HINSTANCE)GetWindowLongPtr(parent, GWLP_HINSTANCE),
		NULL); // Pointer not needed.
}

static void AddMonitorsToCombo(HWND ComboBoxHandleBox)
{
	MonitorService ms;
	TheMonitorData = ms.GetMonitorsData();

	for (auto& i : TheMonitorData)	
	{
		SendMessage(ComboBoxHandleBox, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(i.FriendlyName.c_str()));
	}	
}

static void SelectMonitor(HWND ComboBoxHandle)
{
	SettingsService ss;
	std::wstring monitorDeviceName = ss.LoadSelectedMonitor();

	auto index = 0;
	auto found = false;
	for (auto& i : TheMonitorData)	
	{
		if (i.DeviceName == monitorDeviceName)
		{
			SendMessage(ComboBoxHandle, CB_SETCURSEL, index, 0);
			found = true;
			EnableWindow(BtnHandle, true);
			break;
		}

		++index;
	}

	if (!found)
	{
		// clear
		SendMessage(ComboBoxHandle, CB_SETCURSEL, -1, 0);
	}	
}

static HWND CreateComboBox(HWND parent)
{
	auto result = CreateWindowW(
		L"COMBOBOX",
		NULL,
		WS_TABSTOP | WS_VISIBLE | WS_CHILD | CBS_DROPDOWNLIST,
		10, // x position 
		100, // y position 
		ComboBoxWidth, // width
		ComboBoxHeight, // height
		parent, // Parent window
		(HMENU)(UINT_PTR)ComboBoxId, // Identifier for the button
		(HINSTANCE)GetWindowLongPtr(parent, GWLP_HINSTANCE),
		NULL); // Pointer not needed.

	AddMonitorsToCombo(result);
	SelectMonitor(result);

	return result;
}

static HFONT CreateModernFont()
{
	return CreateFont(
		16, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, 
		DEFAULT_CHARSET, OUT_OUTLINE_PRECIS,
		CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY, DEFAULT_PITCH,
		L"Segoe UI");
}

static void SaveSelectedMonitorId(int selectedIndex)
{
	MonitorData md = TheMonitorData[selectedIndex];
	SettingsService ss;
	ss.SaveSelectedMonitor(md.DeviceName);
}

static void SetModernFont(HWND mainWindow)
{
	if (!ModernFont)
	{
		ModernFont = CreateModernFont();
		SendMessage(mainWindow, WM_SETFONT, WPARAM(ModernFont), TRUE);
		SendMessage(BtnHandle, WM_SETFONT, WPARAM(ModernFont), TRUE);
		SendMessage(ComboBoxHandle, WM_SETFONT, WPARAM(ModernFont), TRUE);
	}
}

static void ToggleZoomWindow()
{
	if ((TheDisplayWindowResult != nullptr && TheDisplayWindowResult->AllOk) || TheZoomService->IsDisplayed())
	{
		// Hide the window
		TheZoomService->Hide();
		TheDisplayWindowResult.reset();
	}
	else
	{
		// show the window				
		TheDisplayWindowResult = std::unique_ptr<DisplayWindowResult>(new DisplayWindowResult(TheZoomService->Display()));
	}	
}

void HandleResize(HWND hwnd, LPARAM lParam)
{
	UINT width = LOWORD(lParam);
	UINT height = HIWORD(lParam);

	RECT rcBtn = {};
	RECT rcCombo = {};
	GetClientRect(BtnHandle, &rcBtn);
	GetClientRect(ComboBoxHandle, &rcCombo);

	// arrange the button
	int x = (int)(width - (rcBtn.right - rcBtn.left)) / 2;
	int y = 10;
	MoveWindow(BtnHandle, x, y, rcBtn.right - rcBtn.left, rcBtn.bottom - rcBtn.top, 1);

	// arrange the combo		
	x = (int)(width - (rcCombo.right - rcCombo.left)) / 2;
	y = rcBtn.bottom + 20;
	MoveWindow(ComboBoxHandle, x, y, rcCombo.right - rcCombo.left, rcCombo.bottom - rcCombo.top, 1);
}

LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
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
		LPMINMAXINFO lpMMI = (LPMINMAXINFO)lParam;
		lpMMI->ptMinTrackSize.x = 100;
		lpMMI->ptMinTrackSize.y = 100;
		break;
	}

	case WM_CREATE:
		BtnHandle = CreateButton(hWnd);		
		ComboBoxHandle = CreateComboBox(hWnd);
		SetModernFont(hWnd);
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
				int selectedIndex = (int)SendMessage(ComboBoxHandle, CB_GETCURSEL, 0, 0);
				if (selectedIndex != CB_ERR)
				{
					SaveSelectedMonitorId(selectedIndex);
					EnableWindow(BtnHandle, true);
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
		HDC hdc = BeginPaint(hWnd, &ps);
		// Add any drawing code that uses hdc here...
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

