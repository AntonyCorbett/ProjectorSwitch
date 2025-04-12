// ProjectorSwitch.cpp : Defines the entry point for the application.

#include "framework.h"
#include <commctrl.h> // Include the header for common controls
#include <strsafe.h>
#include <string>
#include "ProjectorSwitch.h"
#include "MonitorService.h"
#include "SettingsService.h"
#include "ZoomService.h"

const int MAX_LOADSTRING = 100;

#pragma comment(linker,"\"/manifestdependency:type='win32' \
name='Microsoft.Windows.Common-Controls' version='6.0.0.0' \
processorArchitecture='*' publicKeyToken='6595b64144ccf1df' language='*'\"")

#pragma comment(lib, "ComCtl32.lib") // Link the ComCtl32 library

// Global Variables:
HINSTANCE hInst;                                // current instance
WCHAR szTitle[MAX_LOADSTRING];                  // The title bar text
WCHAR szWindowClass[MAX_LOADSTRING];            // the main window class name
HWND hwndButton;
HWND hwndCombo;
std::vector<MonitorData> monitorData;
ZoomService* zoomService = nullptr;
DisplayWindowResult* displayWindowResult = nullptr;

int ButtonId = 10001;
int ComboBoxId = 10002;

// Forward declarations of functions included in this code module:
ATOM                MyRegisterClass(HINSTANCE hInstance);
BOOL                InitInstance(HINSTANCE, int);
LRESULT CALLBACK    WndProc(HWND, UINT, WPARAM, LPARAM);

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
	LoadStringW(hInstance, IDS_APP_TITLE, szTitle, MAX_LOADSTRING);
	LoadStringW(hInstance, IDC_PROJECTOR_SWITCH, szWindowClass, MAX_LOADSTRING);
	MyRegisterClass(hInstance);

	// Perform application initialization:
	if (!InitInstance(hInstance, nCmdShow))
	{
		return FALSE;
	}

	HACCEL hAccelTable = LoadAccelerators(hInstance, MAKEINTRESOURCE(IDC_PROJECTOR_SWITCH));

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

ATOM MyRegisterClass(HINSTANCE hInstance)
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
	wcex.lpszClassName = szWindowClass;
	
	return RegisterClassExW(&wcex);
}

BOOL InitInstance(HINSTANCE hInstance, int nCmdShow)
{
	hInst = hInstance; // Store instance handle in our global variable

	HWND hWnd = CreateWindowW(
		szWindowClass, 
		szTitle, 
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

	ShowWindow(hWnd, nCmdShow);
	UpdateWindow(hWnd);

	return TRUE;
}

static HWND CreateButton(HWND parent)
{
	HWND result = CreateWindowW(
		L"BUTTON",
		L"ZOOM",		
		WS_TABSTOP | WS_VISIBLE | WS_CHILD,
		10, // x position 
		10, // y position 
		60, // Button width
		60, // Button height
		parent, // Parent window
		(HMENU)(UINT_PTR)(ButtonId), // Identifier for the button
		(HINSTANCE)GetWindowLongPtr(parent, GWLP_HINSTANCE),
		NULL); // Pointer not needed.

	return result;
}

static void AddMonitorsToCombo(HWND hwndComboBox)
{
	MonitorService ms;
	monitorData = ms.GetMonitorsData();

	for (std::vector<MonitorData>::iterator i = monitorData.begin(); i != monitorData.end(); ++i)
	{
		SendMessage(hwndComboBox, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(i->FriendlyName.c_str()));
	}	
}

static void SelectMonitor(HWND hwndCombo)
{
	SettingsService ss;
	int id = ss.LoadSelectedMonitorId();

	int index = 0;
	bool found = false;
	for (std::vector<MonitorData>::iterator i = monitorData.begin(); i != monitorData.end(); ++i)
	{
		if (i->Id == id)
		{
			SendMessage(hwndCombo, CB_SETCURSEL, index, 0);
			found = true;
			break;
		}

		++index;
	}

	if (!found)
	{
		SendMessage(hwndCombo, CB_SETCURSEL, -1, 0);
	}
}

static HWND CreateComboBox(HWND parent)
{
	HWND result = CreateWindowW(
		L"COMBOBOX",
		NULL,
		WS_TABSTOP | WS_VISIBLE | WS_CHILD | CBS_DROPDOWNLIST,
		10, // x position 
		100, // y position 
		100, // width
		20, // height
		parent, // Parent window
		(HMENU)(UINT_PTR)ComboBoxId, // Identifier for the button
		(HINSTANCE)GetWindowLongPtr(parent, GWLP_HINSTANCE),
		NULL); // Pointer not needed.

	AddMonitorsToCombo(result);
	SelectMonitor(result);

	return result;
}

static WCHAR* RectToStr(RECT rc)
{
	static WCHAR buffer[128];
	StringCchPrintfW(buffer, sizeof(buffer) / sizeof(WCHAR), L"(%d,%d) (%d,%d)", rc.left, rc.top, rc.right, rc.bottom);
	return buffer;
}

static HFONT CreateModernFont()
{
	HFONT hFont = ::CreateFont(
		16, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, 
		DEFAULT_CHARSET, OUT_OUTLINE_PRECIS,
		CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY, DEFAULT_PITCH,
		L"Segoe UI");

	return hFont;
}

static void SaveSelectedMonitorId(int selectedIndex)
{
	MonitorData md = monitorData[selectedIndex];
	SettingsService ss;
	ss.SaveSelectedMonitorId(md.Id);
}

static void SetModernFont(HWND mainWindow)
{
	HFONT hFont = CreateModernFont();
	SendMessage(mainWindow, WM_SETFONT, WPARAM(hFont), TRUE);
	SendMessage(hwndButton, WM_SETFONT, WPARAM(hFont), TRUE);
	SendMessage(hwndCombo, WM_SETFONT, WPARAM(hFont), TRUE);
}

static void ToggleZoomWindow()
{
	if (displayWindowResult != nullptr && displayWindowResult->AllOk)
	{
		// Hide the window
		zoomService->Hide();
		delete displayWindowResult;
		displayWindowResult = nullptr;		
	}
	else
	{
		// show the window				
		displayWindowResult = new DisplayWindowResult(zoomService->Display());
	}	
}

LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message)
	{
	case WM_SIZE:
	{		
		UINT width = LOWORD(lParam);
		UINT height = HIWORD(lParam);

		RECT rcBtn = {};
		RECT rcCombo = {};
		GetClientRect(hwndButton, &rcBtn);
		GetClientRect(hwndCombo, &rcCombo);

		// arrange the button
		int x = (int)(width - (rcBtn.right - rcBtn.left)) / 2;
		int y = 10;
		MoveWindow(hwndButton, x, y, rcBtn.right - rcBtn.left, rcBtn.bottom - rcBtn.top, 1);

		// arrange the combo		
		x = (int)(width - (rcCombo.right - rcCombo.left)) / 2;
		y = rcBtn.bottom + 20;
		MoveWindow(hwndCombo, x, y, rcCombo.right - rcCombo.left, rcCombo.bottom - rcCombo.top, 1);

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
		hwndButton = CreateButton(hWnd);
		hwndCombo = CreateComboBox(hWnd);
		SetModernFont(hWnd);
		zoomService = new ZoomService(new AutomationService(), new ProcessesService());
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
				int selectedIndex = (int)SendMessage(hwndCombo, CB_GETCURSEL, 0, 0);
				if (selectedIndex != CB_ERR)
				{
					SaveSelectedMonitorId(selectedIndex);
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
		// //TODO: Add any drawing code that uses hdc here...
		EndPaint(hWnd, &ps);
	}
	break;
	case WM_DESTROY:
		delete zoomService;
		zoomService = nullptr;
		PostQuitMessage(0);
		break;
	default:
		return DefWindowProc(hWnd, message, wParam, lParam);
	}
	return 0;
}

