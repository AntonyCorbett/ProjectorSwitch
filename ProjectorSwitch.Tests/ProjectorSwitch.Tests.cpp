#include "pch.h"
#include "CppUnitTest.h"
#include "../ProjectorSwitch/DisplayWindowResult.h"
#include "../ProjectorSwitch/FindWindowsResult.h"
#include "../ProjectorSwitch/HandleDeleter.h"
#include "../ProjectorSwitch/VariantWrapper.h"

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace ProjectorSwitchTests
{
	TEST_CLASS(ProjectorSwitchTests)
	{
	public:
		TEST_METHOD(DisplayWindowResult_DefaultState_IsNotOkAndEmptyMessage)
		{
			const DisplayWindowResult result;

			Assert::IsFalse(result.AllOk);
			Assert::AreEqual(std::wstring{}, result.ErrorMessage);
		}

		TEST_METHOD(FindWindowsResult_DefaultState_IsExpected)
		{
			const FindWindowsResult result;

			Assert::IsNull(result.Element);
			Assert::IsFalse(result.IsRunning);
			Assert::IsFalse(result.FoundDesktop);
			Assert::IsFalse(result.FoundMediaWindow);
			Assert::AreEqual(std::wstring{}, result.BespokeErrorMsg);
		}

		TEST_METHOD(VariantWrapper_SetString_SetsBstrValue)
		{
			VariantWrapper wrapper;
			wrapper.SetString(L"Projector");
			VARIANT* value = wrapper;

			Assert::AreEqual(static_cast<VARTYPE>(VT_BSTR), value->vt);
			Assert::AreEqual(std::wstring{ L"Projector" }, std::wstring{ value->bstrVal });
		}

		TEST_METHOD(VariantWrapper_SetBool_SetsBooleanValue)
		{
			VariantWrapper wrapper;
			wrapper.SetBool(true);
			VARIANT* value = wrapper;

			Assert::AreEqual(static_cast<VARTYPE>(VT_BOOL), value->vt);
			Assert::AreEqual(static_cast<VARIANT_BOOL>(VARIANT_TRUE), value->boolVal);
		}

		TEST_METHOD(VariantWrapper_SetInt_SetsIntValue)
		{
			VariantWrapper wrapper;
			wrapper.SetInt(42);
			VARIANT* value = wrapper;

			Assert::AreEqual(static_cast<VARTYPE>(VT_I4), value->vt);
			Assert::AreEqual(42L, value->lVal);
		}

		TEST_METHOD(VariantWrapper_SetDouble_SetsDoubleValue)
		{
			VariantWrapper wrapper;
			wrapper.SetDouble(12.5);
			VARIANT* value = wrapper;

			Assert::AreEqual(static_cast<VARTYPE>(VT_R8), value->vt);
			Assert::AreEqual(12.5, value->dblVal, 0.0001);
		}

		TEST_METHOD(VariantWrapper_SetNull_SetsNullType)
		{
			VariantWrapper wrapper;
			wrapper.SetNull();
			VARIANT* value = wrapper;

			Assert::AreEqual(static_cast<VARTYPE>(VT_NULL), value->vt);
		}

		TEST_METHOD(HandleDeleter_ValidHandle_ClosesHandle)
		{
			const HANDLE handle = CreateEvent(nullptr, TRUE, FALSE, nullptr);
			Assert::IsNotNull(handle);

			const HandleDeleter deleter;
			deleter(handle);

			SetLastError(0);
			const BOOL closedAgain = CloseHandle(handle);
			Assert::AreEqual(FALSE, closedAgain);
			Assert::AreEqual(static_cast<DWORD>(ERROR_INVALID_HANDLE), GetLastError());
		}

		TEST_METHOD(HandleDeleter_NullAndInvalidHandle_DoesNothing)
		{
			const HandleDeleter deleter;

			deleter(nullptr);
			deleter(INVALID_HANDLE_VALUE);
		}
	};
}
