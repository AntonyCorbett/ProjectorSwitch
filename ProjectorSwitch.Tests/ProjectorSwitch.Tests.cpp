#include "pch.h"
#include "CppUnitTest.h"
#include "../ProjectorSwitch/DisplayWindowResult.h"
#include "../ProjectorSwitch/FindWindowsResult.h"
#include "../ProjectorSwitch/HandleDeleter.h"
#include "../ProjectorSwitch/VariantWrapper.h"
#include "../ProjectorSwitch/AutomationElementWrapper.h"
#include "../ProjectorSwitch/AutomationConditionWrapper.h"
#include "../ProjectorSwitch/SettingsService.h"

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

		TEST_METHOD(DisplayWindowResult_SetAllOk_IsTrue)
		{
			DisplayWindowResult result;
			result.AllOk = true;

			Assert::IsTrue(result.AllOk);
		}

		TEST_METHOD(DisplayWindowResult_SetErrorMessage_HasMessage)
		{
			DisplayWindowResult result;
			result.ErrorMessage = L"Zoom window not found";

			Assert::AreEqual(std::wstring{ L"Zoom window not found" }, result.ErrorMessage);
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

		TEST_METHOD(FindWindowsResult_SetFields_ReflectsChanges)
		{
			FindWindowsResult result;
			result.IsRunning = true;
			result.FoundDesktop = true;
			result.FoundMediaWindow = true;
			result.BespokeErrorMsg = L"custom error";

			Assert::IsTrue(result.IsRunning);
			Assert::IsTrue(result.FoundDesktop);
			Assert::IsTrue(result.FoundMediaWindow);
			Assert::AreEqual(std::wstring{ L"custom error" }, result.BespokeErrorMsg);
		}

		TEST_METHOD(VariantWrapper_DefaultConstruction_IsVtEmpty)
		{
			VariantWrapper wrapper;
			VARIANT* value = wrapper;

			Assert::AreEqual(static_cast<VARTYPE>(VT_EMPTY), value->vt);
		}

		TEST_METHOD(VariantWrapper_SetString_SetsBstrValue)
		{
			VariantWrapper wrapper;
			wrapper.SetString(L"Projector");
			VARIANT* value = wrapper;

			Assert::AreEqual(static_cast<VARTYPE>(VT_BSTR), value->vt);
			Assert::AreEqual(std::wstring{ L"Projector" }, std::wstring{ value->bstrVal });
		}

		TEST_METHOD(VariantWrapper_SetString_EmptyString_SetsBstrEmpty)
		{
			VariantWrapper wrapper;
			wrapper.SetString(L"");
			VARIANT* value = wrapper;

			Assert::AreEqual(static_cast<VARTYPE>(VT_BSTR), value->vt);
			Assert::AreEqual(std::wstring{}, std::wstring{ value->bstrVal });
		}

		TEST_METHOD(VariantWrapper_SetBool_SetsBooleanValue)
		{
			VariantWrapper wrapper;
			wrapper.SetBool(true);
			VARIANT* value = wrapper;

			Assert::AreEqual(static_cast<VARTYPE>(VT_BOOL), value->vt);
			Assert::AreEqual(static_cast<VARIANT_BOOL>(VARIANT_TRUE), value->boolVal);
		}

		TEST_METHOD(VariantWrapper_SetBoolFalse_SetsVariantFalse)
		{
			VariantWrapper wrapper;
			wrapper.SetBool(false);
			VARIANT* value = wrapper;

			Assert::AreEqual(static_cast<VARTYPE>(VT_BOOL), value->vt);
			Assert::AreEqual(static_cast<VARIANT_BOOL>(VARIANT_FALSE), value->boolVal);
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

	TEST_CLASS(AutomationWrapperTests)
	{
	public:
		TEST_METHOD(AutomationElementWrapper_NullElement_GetElementReturnsNull)
		{
			AutomationElementWrapper wrapper(nullptr);

			Assert::IsNull(wrapper.GetElement());
		}

		TEST_METHOD(AutomationConditionWrapper_NullCondition_GetConditionReturnsNull)
		{
			AutomationConditionWrapper wrapper(nullptr);

			Assert::IsNull(wrapper.GetCondition());
		}
	};

	TEST_CLASS(SettingsServiceTests)
	{
	private:
		static std::wstring GetSettingsFilePath()
		{
			WCHAR buffer[MAX_PATH];
			GetCurrentDirectory(MAX_PATH, buffer);
			return buffer + std::wstring(L"\\settings.ini");
		}

		static void DeleteSettingsFile()
		{
			DeleteFile(GetSettingsFilePath().c_str());
		}

	public:
		TEST_METHOD_INITIALIZE(Setup)
		{
			DeleteSettingsFile();
		}

		TEST_METHOD_CLEANUP(Cleanup)
		{
			DeleteSettingsFile();
		}

		TEST_METHOD(LoadMonitorRect_NoFile_ReturnsZeroRect)
		{
			const SettingsService service;
			const RECT rect = service.LoadSelectedMonitorRect();

			Assert::AreEqual(0L, rect.left);
			Assert::AreEqual(0L, rect.top);
			Assert::AreEqual(0L, rect.right);
			Assert::AreEqual(0L, rect.bottom);
		}

		TEST_METHOD(SaveAndLoadMonitorRect_RoundTrips)
		{
			SettingsService service;
			const RECT expected{ 100, 200, 1380, 1280 };
			service.SaveSelectedMonitorRect(expected);
			const RECT actual = service.LoadSelectedMonitorRect();

			Assert::AreEqual(expected.left, actual.left);
			Assert::AreEqual(expected.top, actual.top);
			Assert::AreEqual(expected.right, actual.right);
			Assert::AreEqual(expected.bottom, actual.bottom);
		}

		TEST_METHOD(SaveAndLoadMonitorRect_NegativeCoordinates_RoundTrips)
		{
			// Secondary monitors positioned to the left of the primary have negative coords.
			SettingsService service;
			const RECT expected{ -1920, -200, 0, 880 };
			service.SaveSelectedMonitorRect(expected);
			const RECT actual = service.LoadSelectedMonitorRect();

			Assert::AreEqual(expected.left, actual.left);
			Assert::AreEqual(expected.top, actual.top);
			Assert::AreEqual(expected.right, actual.right);
			Assert::AreEqual(expected.bottom, actual.bottom);
		}

		TEST_METHOD(SaveMonitorRect_OverwritesPreviousValue)
		{
			SettingsService service;
			const RECT first{ 0, 0, 1920, 1080 };
			const RECT second{ 1920, 0, 3840, 1080 };
			service.SaveSelectedMonitorRect(first);
			service.SaveSelectedMonitorRect(second);
			const RECT actual = service.LoadSelectedMonitorRect();

			Assert::AreEqual(second.left, actual.left);
			Assert::AreEqual(second.right, actual.right);
		}

		TEST_METHOD(LoadMonitorKey_NoFile_ReturnsEmpty)
		{
			const SettingsService service;

			Assert::AreEqual(std::wstring{}, service.LoadSelectedMonitorKey());
		}

		TEST_METHOD(SaveAndLoadMonitorKey_RoundTrips)
		{
			SettingsService service;
			const std::wstring expected = L"SERIAL:ABC123XYZ";
			service.SaveSelectedMonitorKey(expected);

			Assert::AreEqual(expected, service.LoadSelectedMonitorKey());
		}

		TEST_METHOD(SaveAndLoadWindowPlacement_AllFields_RoundTrips)
		{
			SettingsService service;
			WINDOWPLACEMENT expected{};
			expected.length = sizeof(WINDOWPLACEMENT);
			expected.showCmd = SW_SHOWNORMAL;
			expected.flags = 0;
			expected.ptMinPosition = { 10, 20 };
			expected.ptMaxPosition = { 30, 40 };
			expected.rcNormalPosition = { 100, 200, 900, 700 };

			service.SaveWindowPlacement(expected);
			const WINDOWPLACEMENT actual = service.LoadWindowPlacement();

			Assert::AreEqual(static_cast<UINT>(expected.showCmd), static_cast<UINT>(actual.showCmd));
			Assert::AreEqual(static_cast<UINT>(expected.flags), static_cast<UINT>(actual.flags));
			Assert::AreEqual(expected.ptMinPosition.x, actual.ptMinPosition.x);
			Assert::AreEqual(expected.ptMinPosition.y, actual.ptMinPosition.y);
			Assert::AreEqual(expected.ptMaxPosition.x, actual.ptMaxPosition.x);
			Assert::AreEqual(expected.ptMaxPosition.y, actual.ptMaxPosition.y);
			Assert::AreEqual(expected.rcNormalPosition.left, actual.rcNormalPosition.left);
			Assert::AreEqual(expected.rcNormalPosition.top, actual.rcNormalPosition.top);
			Assert::AreEqual(expected.rcNormalPosition.right, actual.rcNormalPosition.right);
			Assert::AreEqual(expected.rcNormalPosition.bottom, actual.rcNormalPosition.bottom);
		}
	};
}
