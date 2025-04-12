#include "AutomationService.h"

AutomationService::AutomationService()
{
	// Initialize COM library
	HRESULT hr = CoInitializeEx(NULL, COINIT_MULTITHREADED);

	if (SUCCEEDED(hr))
	{
		// Initialize COM library for UI Automation
		hr = CoInitializeSecurity(
			NULL,
			-1,
			NULL,
			NULL,
			RPC_C_AUTHN_LEVEL_DEFAULT,
			RPC_C_IMP_LEVEL_IMPERSONATE,
			NULL,
			EOAC_NONE,
			NULL
		);
	}

	if (SUCCEEDED(hr))
	{
		hr = CoCreateInstance(
			CLSID_CUIAutomation,
			NULL,
			CLSCTX_INPROC_SERVER,
			IID_IUIAutomation,
			(void**)&pAutomation
		);

		if (SUCCEEDED(hr))
		{
			LocateDesktop();
		}
	}
}

AutomationService::~AutomationService()
{
	if (pDesktopElement)
	{
		pDesktopElement->Release();
		pDesktopElement = nullptr;
	}

	if (pAutomation)
	{
		pAutomation->Release();
		pAutomation = nullptr;
	}

	CoUninitialize();
}

void AutomationService::LocateDesktop()
{
	HRESULT hr = pAutomation->GetRootElement(&pDesktopElement);
	if (!SUCCEEDED(hr))
	{
		// todo: error message
		pDesktopElement = nullptr;
	}
}