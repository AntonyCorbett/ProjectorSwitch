#pragma once
#include <uiautomation.h>

class AutomationConditionWrapper
{
private:
	IUIAutomationCondition* condition;

public:
	AutomationConditionWrapper(IUIAutomationCondition* condition)
	{
		this->condition = condition;
	}

	~AutomationConditionWrapper()
	{
		if (condition != nullptr)
		{
			condition->Release();
			condition = nullptr;
		}
	}

	IUIAutomationCondition* GetCondition()
	{
		return condition;
	}
};

