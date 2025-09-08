#pragma once
#include <uiautomation.h>

class AutomationConditionWrapper  // NOLINT(cppcoreguidelines-special-member-functions)
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

	IUIAutomationCondition* GetCondition() const
	{
		return condition_;
	}
};

