#include "AutomationService.h"

AutomationService::AutomationService()
	: automation_(nullptr)
	, desktopElement_(nullptr)
{
	// Initialize COM library
	auto hr = CoInitializeEx(NULL, COINIT_MULTITHREADED);

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
			(void**)&automation_);

		if (SUCCEEDED(hr))
		{
			LocateDesktop();
		}
	}
}

AutomationService::~AutomationService()
{
	if (desktopElement_)
	{
		desktopElement_->Release();
		desktopElement_ = nullptr;
	}

	if (automation_)
	{
		automation_->Release();
		automation_ = nullptr;
	}

	CoUninitialize();
}

void AutomationService::LocateDesktop()
{
	auto hr = automation_->GetRootElement(&desktopElement_);
	if (!SUCCEEDED(hr))
	{
		OutputDebugString(L"Could not get desktop element!");
		desktopElement_ = nullptr;
	}
}