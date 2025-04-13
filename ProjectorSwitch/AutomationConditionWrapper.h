#pragma once
#include <uiautomation.h>

class AutomationConditionWrapper
{
private:
	IUIAutomationCondition* condition_;

public:
	AutomationConditionWrapper(IUIAutomationCondition* condition)
		: condition_(condition)
	{		
	}

	~AutomationConditionWrapper()
	{
		if (condition_ != nullptr)
		{
			condition_->Release();
			condition_ = nullptr;
		}
	}

	IUIAutomationCondition* GetCondition()
	{
		return condition_;
	}
};

