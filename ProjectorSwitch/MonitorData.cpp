#include "MonitorData.h"
#include <vector>
#include <numeric>

namespace
{
    void ReplaceAll(std::wstring& str, const std::wstring& from, const std::wstring& to)
    {
        if (from.empty())
        {
            return;
        }

        size_t startPos = 0;
        while ((startPos = str.find(from, startPos)) != std::wstring::npos)
        {
            str.replace(startPos, from.length(), to);
            startPos += to.length(); // In case 'to' contains 'from', like replacing 'x' with 'yx'
        }
    }
}

std::wstring MonitorData::GetDisplayName(const std::wstring& format) const
{
    std::wstring result = format;

    ReplaceAll(result, L"{FriendlyName}", FriendlyName);
    ReplaceAll(result, L"{Position}", RelativePosition);
    ReplaceAll(result, L"{SerialNumber}", SerialNumber);
    ReplaceAll(result, L"{Key}", Key);
    ReplaceAll(result, L"{DeviceName}", DeviceName);

    // Handle the Rect placeholder
    if (result.find(L"{Rect}") != std::wstring::npos)
    {
        const std::wstring rectStr =
            std::to_wstring(MonitorRect.left) + L"," +
            std::to_wstring(MonitorRect.top) + L"," +
            std::to_wstring(MonitorRect.right) + L"," +
            std::to_wstring(MonitorRect.bottom);

        ReplaceAll(result, L"{Rect}", rectStr);
    }

    // Handle the Size placeholder
    if (result.find(L"{Size}") != std::wstring::npos)
    {
        const long width = MonitorRect.right - MonitorRect.left;
        const long height = MonitorRect.bottom - MonitorRect.top;
        const std::wstring sizeStr = std::to_wstring(width) + L"x" + std::to_wstring(height);
        ReplaceAll(result, L"{Size}", sizeStr);
    }

    // Clean up cases where a placeholder for an empty value might leave dangling parentheses or brackets.
    // For example, if SerialNumber is empty, "{FriendlyName} [{SerialNumber}]" becomes "{FriendlyName} []".
    ReplaceAll(result, L" ()", L"");
    ReplaceAll(result, L" []", L"");
    ReplaceAll(result, L" <>", L"");

    // Trim trailing space that might result from the above replacements
    const size_t end = result.find_last_not_of(L" \t");
    if (std::wstring::npos != end)
    {
        result = result.substr(0, end + 1);
    }

    return result;
}
