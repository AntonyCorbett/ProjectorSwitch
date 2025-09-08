#pragma once
#include <atlstr.h>
#include <string>

class VariantWrapper  // NOLINT(cppcoreguidelines-special-member-functions)
{
private:
	VARIANT variant_;

public:
	VariantWrapper()
	{		
		VariantInit(&variant_);
	}

	~VariantWrapper()
	{
		if (variant_.vt != VT_EMPTY)
		{
			VariantClear(&variant_);
		}
	}

	void SetString(const std::wstring& value)
	{
		variant_.vt = VT_BSTR;
		variant_.bstrVal = SysAllocString(value.c_str());
	}

	void SetBool(bool value)
	{
		variant_.vt = VT_BOOL;
		variant_.boolVal = value ? VARIANT_TRUE : VARIANT_FALSE;
	}

	void SetInt(int value)
	{
		variant_.vt = VT_I4;
		variant_.lVal = value;
	}

	void SetDouble(double value)
	{
		variant_.vt = VT_R8;
		variant_.dblVal = value;
	}

	void SetNull()
	{
		variant_.vt = VT_NULL;
	}

	operator VARIANT* () { return &variant_; }
};
