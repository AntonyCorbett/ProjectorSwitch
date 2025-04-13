#pragma once
#include <uiautomation.h>

class AutomationElementWrapper
{
private:
	IUIAutomationElement* element;

public:
	AutomationElementWrapper(IUIAutomationElement* element)
	{
		this->element = element;
	}

	~AutomationElementWrapper()
	{
		if (element)
		{
			element->Release();
			element = nullptr;
		}
	}

	IUIAutomationElement* GetElement()
	{
		return element;
	}
};

