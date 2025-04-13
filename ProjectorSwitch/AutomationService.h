#pragma once
#include <uiautomation.h>

class AutomationService
{
private:
	IUIAutomation* automation_;
	IUIAutomationElement* desktopElement_;

	void LocateDesktop();

public:
	AutomationService();
	~AutomationService();

	IUIAutomation* GetAutomationInterface()
	{
		return automation_;
	}

	IUIAutomationElement* DesktopElement()
	{
		return desktopElement_;
	}
};
