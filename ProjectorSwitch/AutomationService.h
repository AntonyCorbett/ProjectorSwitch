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

	IUIAutomation* GetAutomationInterface() const
	{
		return automation_;
	}

	IUIAutomationElement* DesktopElement() const
	{
		return desktopElement_;
	}
};
