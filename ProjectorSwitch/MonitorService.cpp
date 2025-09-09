#include <windows.h>
#include <vector>
#include <stdexcept>
#include <SetupAPI.h>
#include <string>
#include <unordered_map>

#pragma comment(lib, "SetupAPI.lib")

#include "MonitorService.h"
#include "MonitorData.h"
#include "DisplayConfigData.h"

namespace
{
    BOOL CALLBACK EnumMonitorProc(const HMONITOR monitor, const HDC hdc, const LPRECT rect, const LPARAM lparam)
    {
        UNREFERENCED_PARAMETER(hdc);
        UNREFERENCED_PARAMETER(rect);

        // NOLINTNEXTLINE(performance-no-int-to-ptr)  // Win32 callback: LPARAM carries a pointer by API contract
        auto* monitors = reinterpret_cast<std::vector<MONITORINFOEX>*>(lparam);
        MONITORINFOEX info{};
        info.cbSize = sizeof(MONITORINFOEX);
        if (::GetMonitorInfo(monitor, &info))
        {
            monitors->push_back(info);
        }

        return TRUE;
    }

    std::wstring Trim(const std::wstring& s)
    {
        const size_t start = s.find_first_not_of(L" \t\r\n");
        if (start == std::wstring::npos)
        {
            return L"";
        }

        const size_t end = s.find_last_not_of(L" \t\r\n");
        return s.substr(start, end - start + 1);
    }

    bool IsLikelyValidSerial(const std::wstring& s)
    {
        const auto t = Trim(s);
        if (t.empty())
        {
            return false;
        }

        if (t == L"00000000" || t == L"FFFFFFFF")
        {
            return false;
        }

        // Reject strings that are all identical characters or whitespace/punctuation only
        bool hasAlnum = false;
        for (const wchar_t c : t)
        {
            if (iswalnum(c))
            {
                hasAlnum = true;
                break;
            }
        }

        return hasAlnum;
    }

    // Extract ASCII string from an 18-byte descriptor with type tag at offset 3
    std::wstring ExtractAsciiDescriptor(const BYTE* desc)
    {
        // bytes [5..17] are ASCII text terminated by 0x0A or 0x00
        wchar_t wbuf[14] = {};
        for (int j = 0; j < 13; ++j)
        {
            const BYTE ch = desc[5 + j];
            if (ch == 0x0A || ch == 0x00)
            {
                break;
            }

            // Guard against non-printable
            if (ch < 0x20 || ch > 0x7E)
            {
                wbuf[j] = L'?';
                continue;
            }

            wbuf[j] = static_cast<wchar_t>(ch);
        }

        return Trim(std::wstring(wbuf));
    }

    std::wstring ParseEdidSerialFromBlock(const BYTE* edid128)
    {
        if (!edid128)
        {
            return L"";
        }

        constexpr size_t kDescriptorCount = 4u;

        for (size_t i = 0; i < kDescriptorCount; ++i)
        {
            // Detailed timing/descriptor blocks at 0x36..0x7D (4 blocks * 18 bytes)
            constexpr size_t base = 0x36;
            constexpr size_t kDescriptorSize = 18u;

            const BYTE* desc = edid128 + base + i * kDescriptorSize;
            if (desc[0] == 0x00 && desc[1] == 0x00 && desc[2] == 0x00)
            {
                // 0xFF: Monitor Serial Number; 0xFE: ASCII String (sometimes used by vendors)
                if (desc[3] == 0xFF)
                {
                    auto s = ExtractAsciiDescriptor(desc);
                    if (IsLikelyValidSerial(s))
                    {
                        return s;
                    }
                }
                else if (desc[3] == 0xFE)
                {
                    auto s = ExtractAsciiDescriptor(desc);
                    if (IsLikelyValidSerial(s))
                    {
                        return s;
                    }
                }
            }
        }

        return L"";
    }

    std::wstring ParseEdidSerial(const BYTE* edid, const DWORD size)
    {
        if (!edid || size < 128)
        {
            return L"";
        }

        // Try base block descriptors
        auto s = ParseEdidSerialFromBlock(edid);
        if (IsLikelyValidSerial(s))
        {
            return s;
        }

        // Try extension blocks if present (scan each 128-byte block similarly)
        const BYTE extCount = edid[0x7E];
        for (int i = 0; i < extCount; ++i)
        {
            const size_t off = 128ull * (i + 1);
            if (off + 128 <= size)
            {
                auto se = ParseEdidSerialFromBlock(edid + off);
                if (IsLikelyValidSerial(se))
                {
                    return se;
                }
            }
        }

        // Fallback: 4-byte serial at bytes 12..15 (often 0) rendered hex
        if (size >= 16)
        {
            DWORD ser = 0;
            memcpy(&ser, edid + 12, sizeof(DWORD));
            if (ser != 0 && ser != 0xFFFFFFFF)
            {
                wchar_t wbuf[16];
                (void)swprintf_s(wbuf, L"%08X", ser);
                return std::wstring{ wbuf };
            }
        }

        return L"";
    }

    // Simple cache to avoid repeated registry reads
    std::unordered_map<std::wstring, std::wstring> serialCache;

    /// <summary>
    /// Describes the position of a monitor RECT relative to the primary monitor's RECT
    /// </summary>
    /// <param name="r">Monitor RECT</param>
    /// <param name="primary">Primary monitor RECT</param>
    /// <returns>String describing the position</returns>
    std::wstring DescribePosition(const RECT& r, const RECT& primary)
    {
        // For primary itself
        if (EqualRect(&r, &primary))
        {
            return L"primary";
        }

        if (r.right <= primary.left)
        {
            return L"left";
        }

        if (r.left >= primary.right)
        {
            return L"right";
        }

        if (r.bottom <= primary.top)
        {
            return L"above";
        }

        if (r.top >= primary.bottom)
        {
            return L"below";
        }

        return L"overlap";
    }
}

// ReSharper disable once CppMemberFunctionMayBeStatic
std::vector<MonitorData> MonitorService::GetMonitorsData() const
{
    std::vector<MonitorData> returnData;

    auto monitorInfo = GetMonitorsInfo();
    auto displayInfo = GetDisplayConfigInfo();

    if (monitorInfo.size() != displayInfo.size())
    {
        throw std::runtime_error("Monitor and display config data size mismatch");
    }

    // First pass: create all MonitorData objects
    returnData.reserve(monitorInfo.size());
    for (size_t i = 0; i < monitorInfo.size(); ++i)
    {
        const auto serial = TryGetMonitorSerialFromDevicePath(displayInfo[i].DevicePath);
        returnData.emplace_back(monitorInfo[i], displayInfo[i], serial);
    }

    // Second pass: determine relative positions
    RECT primaryRect{};
    for (const auto& md : returnData)
    {
        if (md.IsPrimary)
        {
            primaryRect = IsRectEmpty(&md.WorkRect) ? md.MonitorRect : md.WorkRect;
            break;
        }
    }

    if (primaryRect.right > 0)
    {
        for (auto& md : returnData)
        {
            // Explicitly check for the primary monitor first.
            if (md.IsPrimary)
            {
                md.RelativePosition = L"primary";
            }
            else
            {
                md.RelativePosition = DescribePosition(md.MonitorRect, primaryRect);
            }
        }
    }

    return returnData;
}

std::vector<MONITORINFOEX> MonitorService::GetMonitorsInfo()
{
    std::vector<MONITORINFOEX> monitors;

    if (!EnumDisplayMonitors(nullptr, nullptr, EnumMonitorProc, reinterpret_cast<LPARAM>(&monitors)))
    {
        throw std::runtime_error("Failed to enumerate monitors");
    }

    return monitors;
}

std::vector<DisplayConfigData> MonitorService::GetDisplayConfigInfo()
{
    std::vector<DisplayConfigData> returnData;

    std::vector<DISPLAYCONFIG_PATH_INFO> paths;
    std::vector<DISPLAYCONFIG_MODE_INFO> modes;
    constexpr UINT32 flags = QDC_ONLY_ACTIVE_PATHS | QDC_VIRTUAL_MODE_AWARE;

    long result;

    do
    {
        UINT32 pathCount;
        UINT32 modeCount;

        result = GetDisplayConfigBufferSizes(flags, &pathCount, &modeCount);
        if (result != ERROR_SUCCESS)
        {
            throw std::runtime_error("Failed to get display config buffer sizes");
        }

        paths.resize(pathCount);
        modes.resize(modeCount);

        result = QueryDisplayConfig(flags, &pathCount, paths.data(), &modeCount, modes.data(), nullptr);

        paths.resize(pathCount);
        modes.resize(modeCount);
    } while (result == ERROR_INSUFFICIENT_BUFFER);

    if (result != ERROR_SUCCESS)
    {
        throw std::runtime_error("Failed to query display config");
    }

    for (const auto& path : paths)
    {
        DISPLAYCONFIG_TARGET_DEVICE_NAME targetName{};
        targetName.header.adapterId = path.targetInfo.adapterId;
        targetName.header.id = path.targetInfo.id;
        targetName.header.type = DISPLAYCONFIG_DEVICE_INFO_GET_TARGET_NAME;
        targetName.header.size = sizeof(targetName);

        result = DisplayConfigGetDeviceInfo(&targetName.header);
        if (result != ERROR_SUCCESS)
        {
            throw std::runtime_error("Failed to get monitor friendly name");
        }

        returnData.emplace_back(
            path.targetInfo.id,
            targetName.flags.friendlyNameFromEdid ? targetName.monitorFriendlyDeviceName : L"Unknown",
            targetName.monitorDevicePath);
    }

    return returnData;
}

std::wstring MonitorService::TryGetMonitorSerialFromDevicePath(const std::wstring& devicePath)
{
    if (devicePath.empty())
    {
        return L"";
    }

    // Cached?
    const auto it = serialCache.find(devicePath);
    if (it != serialCache.end())
    {
        return it->second;
    }

    const HDEVINFO hSet = SetupDiCreateDeviceInfoList(nullptr, nullptr);
    if (hSet == INVALID_HANDLE_VALUE)
    {
        return L"";
    }

    std::wstring serial;

    SP_DEVICE_INTERFACE_DATA ifData{};
    ifData.cbSize = sizeof(SP_DEVICE_INTERFACE_DATA);
    if (SetupDiOpenDeviceInterfaceW(hSet, devicePath.c_str(), 0, &ifData))
    {
        DWORD required = 0;
        SetupDiGetDeviceInterfaceDetailW(hSet, &ifData, nullptr, 0, &required, nullptr);
        std::vector<BYTE> buf(required);
        auto* detail = reinterpret_cast<SP_DEVICE_INTERFACE_DETAIL_DATA_W*>(buf.data());
        detail->cbSize = sizeof(SP_DEVICE_INTERFACE_DETAIL_DATA_W);
        SP_DEVINFO_DATA devInfo{};
        devInfo.cbSize = sizeof(SP_DEVINFO_DATA);

        if (SetupDiGetDeviceInterfaceDetailW(hSet, &ifData, detail, required, nullptr, &devInfo))
        {
            // Device Parameters holds EDID for most monitors
            const HKEY hKey = SetupDiOpenDevRegKey(hSet, &devInfo, DICS_FLAG_GLOBAL, 0, DIREG_DEV, KEY_READ);
            if (hKey && hKey != INVALID_HANDLE_VALUE)
            {
                DWORD type = 0;
                DWORD size = 0;
                if (RegQueryValueExW(hKey, L"EDID", nullptr, &type, nullptr, &size) == ERROR_SUCCESS &&
                    type == REG_BINARY && size > 0)
                {
                    std::vector<BYTE> edid(size);
                    if (RegQueryValueExW(hKey, L"EDID", nullptr, &type, edid.data(), &size) == ERROR_SUCCESS)
                    {
                        const auto parsed = ParseEdidSerial(edid.data(), size);
                        if (IsLikelyValidSerial(parsed))
                        {
                            serial = parsed;
                        }
                    }
                }

                RegCloseKey(hKey);
            }
        }
    }

    SetupDiDestroyDeviceInfoList(hSet);

    // Cache even empty to avoid repeated attempts
    serialCache[devicePath] = serial;
    return serial;
}

int MonitorService::FindMonitorIndex(const std::vector<MonitorData>& monitors, const std::wstring& key, const RECT& rect)
{
    if (!key.empty())
    {
        int index = 0;
        for (const auto& monitor : monitors)
        {
            if (_wcsicmp(monitor.Key.c_str(), key.c_str()) == 0)
            {
                return index;
            }
            ++index;
        }
    }

    // Legacy fallback using persisted RECT
    if (rect.right > 0) // Basic check for a non-empty rect
    {
        int index = 0;
        for (const auto& monitor : monitors)
        {
            if (EqualRect(&monitor.MonitorRect, &rect))
            {
                return index;
            }
            ++index;
        }
    }

    return -1; // Not found
}
