#pragma once
#include <atlstr.h>
#include <string>

class VariantWrapper
{
private:
	VARIANT variant;

public:
	VariantWrapper()
	{		
		VariantInit(&variant);
	}

	~VariantWrapper()
	{
		if (variant.vt != VT_EMPTY)
		{
			VariantClear(&variant);
		}
	}

	void SetString(const std::wstring& value)
	{
		variant.vt = VT_BSTR;
		variant.bstrVal = SysAllocString(value.c_str());
	}

	void SetBool(bool value)
	{
		variant.vt = VT_BOOL;
		variant.boolVal = value ? VARIANT_TRUE : VARIANT_FALSE;
	}

	void SetInt(int value)
	{
		variant.vt = VT_I4;
		variant.lVal = value;
	}

	void SetDouble(double value)
	{
		variant.vt = VT_R8;
		variant.dblVal = value;
	}

	void SetNull()
	{
		variant.vt = VT_NULL;
	}

	operator VARIANT* () { return &variant; }
};
