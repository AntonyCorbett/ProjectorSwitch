#include "AutomationService.h"

AutomationService::AutomationService()
	: automation_(nullptr)
	, desktopElement_(nullptr)
{
	// Initialize COM library
	HRESULT hr = CoInitializeEx(nullptr, COINIT_MULTITHREADED);

	if (SUCCEEDED(hr))
	{
		// Initialize COM library for UI Automation
		hr = CoInitializeSecurity(
			nullptr,
			-1,
			nullptr,
			nullptr,
			RPC_C_AUTHN_LEVEL_DEFAULT,
			RPC_C_IMP_LEVEL_IMPERSONATE,
			nullptr,
			EOAC_NONE,
			nullptr
		);
	}

	if (SUCCEEDED(hr))
	{
		hr = CoCreateInstance(
			CLSID_CUIAutomation,
			nullptr,
			CLSCTX_INPROC_SERVER,
			IID_IUIAutomation,
			reinterpret_cast<void**>(&automation_));

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
	HRESULT hr = automation_->GetRootElement(&desktopElement_);
	if (!SUCCEEDED(hr))
	{
		OutputDebugString(L"Could not get desktop element!");
		desktopElement_ = nullptr;
	}
}