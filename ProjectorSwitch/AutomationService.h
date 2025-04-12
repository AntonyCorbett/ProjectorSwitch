#pragma once
#include <uiautomation.h>

class AutomationService
{
private:
	IUIAutomation* pAutomation = nullptr;
		
	IUIAutomationElement* pDesktopElement = nullptr;

	void LocateDesktop();

public:
	AutomationService();
	~AutomationService();

	IUIAutomation* GetAutomationInterface()
	{
		return pAutomation;
	}

	IUIAutomationElement* DesktopElement()
	{
		return pDesktopElement;
	}
};
