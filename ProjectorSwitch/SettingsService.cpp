#include "SettingsService.h"

namespace
{
	const std::wstring SettingsSection = L"SETTINGS";
	const std::wstring SelectedMonitorL = L"SelectedMonitorL";
	const std::wstring SelectedMonitorT = L"SelectedMonitorT";
	const std::wstring SelectedMonitorR = L"SelectedMonitorR";
	const std::wstring SelectedMonitorB = L"SelectedMonitorB";
	const std::wstring SelectedMonitorKey = L"SelectedMonitorKey";

	const std::wstring WindowSection = L"WINDOW";
	const std::wstring ShowCmd = L"ShowCmd";
	const std::wstring Flags = L"Flags";
	const std::wstring MinPosX = L"MinPosX";
	const std::wstring MinPosY = L"MinPosY";
	const std::wstring MaxPosX = L"MaxPosX";
	const std::wstring MaxPosY = L"MaxPosY";
	const std::wstring PosLeft = L"Left";
	const std::wstring PosTop = L"Top";
	const std::wstring PosRight = L"Right";
	const std::wstring PosBottom = L"Bottom";
}

/// <summary>
/// Constructs a SettingsService object and initializes the path to the settings
/// file in the current directory.
/// </summary>
SettingsService::SettingsService()
{
	// Get the current directory
	WCHAR buffer[MAX_PATH];
	GetCurrentDirectory(MAX_PATH, buffer);
	pathToFile_ = buffer + std::wstring(L"\\settings.ini");
}

SettingsService::~SettingsService() = default;

/// <summary>
/// Saves the window placement settings to persistent storage.
/// </summary>
/// <param name="placement">A reference to a WINDOWPLACEMENT structure containing the
/// window's position, size, and state information to be saved.</param>
void SettingsService::SaveWindowPlacement(const WINDOWPLACEMENT& placement) const
{
	InternalSaveInt(WindowSection, ShowCmd, static_cast<int>(placement.showCmd));
	InternalSaveInt(WindowSection, Flags, static_cast<int>(placement.flags));
	InternalSaveInt(WindowSection, MinPosX, placement.ptMinPosition.x);
	InternalSaveInt(WindowSection, MinPosY, placement.ptMinPosition.y);
	InternalSaveInt(WindowSection, MaxPosX, placement.ptMaxPosition.x);
	InternalSaveInt(WindowSection, MaxPosY, placement.ptMaxPosition.y);
	InternalSaveInt(WindowSection, PosLeft, placement.rcNormalPosition.left);
	InternalSaveInt(WindowSection, PosTop, placement.rcNormalPosition.top);
	InternalSaveInt(WindowSection, PosRight, placement.rcNormalPosition.right);
	InternalSaveInt(WindowSection, PosBottom, placement.rcNormalPosition.bottom);
}

/// <summary>
/// Loads and returns the window placement settings from persistent storage.
/// </summary>
/// <returns>A WINDOWPLACEMENT structure containing the loaded window
/// position, size, and display state.</returns>
WINDOWPLACEMENT SettingsService::LoadWindowPlacement() const
{
	WINDOWPLACEMENT wp;
	wp.length = sizeof(wp);

	wp.showCmd = InternalLoadInt(WindowSection, ShowCmd, 0);
	wp.flags = InternalLoadInt(WindowSection, Flags, 0);
	wp.ptMinPosition.x = InternalLoadInt(WindowSection, MinPosX, 0);
	wp.ptMinPosition.y = InternalLoadInt(WindowSection, MinPosY, 0);
	wp.ptMaxPosition.x = InternalLoadInt(WindowSection, MaxPosX, 0);
	wp.ptMaxPosition.y = InternalLoadInt(WindowSection, MaxPosY, 0);
	wp.rcNormalPosition.left = InternalLoadInt(WindowSection, PosLeft, 0);
	wp.rcNormalPosition.top = InternalLoadInt(WindowSection, PosTop, 0);
	wp.rcNormalPosition.right = InternalLoadInt(WindowSection, PosRight, 0);
	wp.rcNormalPosition.bottom = InternalLoadInt(WindowSection, PosBottom, 0);

	return wp;
}

/// <summary>
/// Saves the coordinates of the selected monitor rectangle to persistent settings.
/// </summary>
/// <param name="rect">The RECT structure containing the coordinates
/// (left, top, right, bottom) of the selected monitor.</param>
void SettingsService::SaveSelectedMonitorRect(const RECT rect) const
{
	InternalSaveInt(SettingsSection, SelectedMonitorL, rect.left);
	InternalSaveInt(SettingsSection, SelectedMonitorT, rect.top);
	InternalSaveInt(SettingsSection, SelectedMonitorR, rect.right);
	InternalSaveInt(SettingsSection, SelectedMonitorB, rect.bottom);
}

/// <summary>
/// Loads the rectangle coordinates of the selected monitor from settings.
/// </summary>
/// <returns>A RECT structure containing the left, top, right, and bottom
/// coordinates of the selected monitor as loaded from the settings.</returns>
RECT SettingsService::LoadSelectedMonitorRect() const
{
	RECT rect;

	rect.left = InternalLoadInt(SettingsSection, SelectedMonitorL, 0);
	rect.top = InternalLoadInt(SettingsSection, SelectedMonitorT, 0);
	rect.right = InternalLoadInt(SettingsSection, SelectedMonitorR, 0);
	rect.bottom = InternalLoadInt(SettingsSection, SelectedMonitorB, 0);

	return rect;
}

/// <summary>
/// Saves the key of the selected monitor to persistent settings.
/// </summary>
/// <param name="key">The monitor Key value.</param>
void SettingsService::SaveSelectedMonitorKey(const std::wstring& key) const
{
	InternalSaveString(SettingsSection, SelectedMonitorKey, key);
}

/// <summary>
/// Loads the key of the selected monitor from persistent settings.
/// </summary>
/// <returns>The monitor Key value.</returns>
std::wstring SettingsService::LoadSelectedMonitorKey() const
{
	return InternalLoadString(SettingsSection, SelectedMonitorKey);
}

/// <summary>
/// Saves a string value to a specified section and key in the settings file.
/// </summary>
/// <param name="section">The section in the settings file where the key-value pair will be stored.</param>
/// <param name="keyName">The name of the key to which the value will be assigned.</param>
/// <param name="keyValue">The string value to be saved under the specified key.</param>
void SettingsService::InternalSaveString(
	const std::wstring& section, const std::wstring& keyName, const std::wstring& keyValue) const
{
	WritePrivateProfileString(section.c_str(), keyName.c_str(), keyValue.c_str(), pathToFile_.c_str());
}

/// <summary>
/// Saves an integer value to a specified section and key in the settings file.
/// </summary>
/// <param name="section">The section in the settings file where the key-value pair will be stored.</param>
/// <param name="keyName">The name of the key to which the value will be assigned.</param>
/// <param name="keyValue">The integer value to be saved under the specified key.</param>
void SettingsService::InternalSaveInt(
	const std::wstring& section, const std::wstring& keyName, const int keyValue) const
{
	const std::wstring strValue = std::to_wstring(keyValue);
	InternalSaveString(section, keyName, strValue);
}

/// <summary>
/// Retrieves a string value from a configuration file for a given section and key.
/// </summary>
/// <param name="section">The name of the section in the configuration file.</param>
/// <param name="keyName">The name of the key whose value is to be retrieved.</param>
/// <returns>The string value associated with the specified section and key. Returns
/// an empty string if the key is not found.</returns>
std::wstring SettingsService::InternalLoadString(
	const std::wstring& section, const std::wstring& keyName) const
{
	WCHAR buffer[256];
	GetPrivateProfileString(
		section.c_str(),
		keyName.c_str(),
		L"",
		buffer, sizeof(buffer) / sizeof(WCHAR),
		pathToFile_.c_str());

	return std::wstring{ buffer };
}

/// <summary>
/// Loads an integer value from a configuration file for a given section and key.
/// </summary>
/// <param name="section">The name of the section in the configuration file.</param>
/// <param name="keyName">The name of the key whose value is to be retrieved.</param>
/// <param name="defaultKeyValue">The default value to return if the key is not found.</param>
/// <returns>The integer value associated with the specified section and key.
/// Returns the default value if the key is not found.</returns>
int SettingsService::InternalLoadInt(
	const std::wstring& section, const std::wstring& keyName, const int defaultKeyValue) const
{
	const auto value = InternalLoadString(section, keyName);
	if (value.empty())
	{
		return defaultKeyValue;
	}

	return std::stoi(value);
}
