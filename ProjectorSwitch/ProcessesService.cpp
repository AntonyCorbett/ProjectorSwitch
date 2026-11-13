#include <string>  
#include <atlbase.h>
#include <memory>
#include "ProcessesService.h"  
#include "HandleDeleter.h"


/// Retrieves a list of process handles for all processes matching the specified name.
// ReSharper disable once CppMemberFunctionMayBeStatic
std::vector<std::unique_ptr<void, HandleDeleter>> ProcessesService::GetProcessesByName(const std::wstring& name)
{
	std::vector<std::unique_ptr<void, HandleDeleter>> result;

	// Create toolhelp snapshot.  
	const HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
	if (snapshot == INVALID_HANDLE_VALUE)
	{
		OutputDebugString(L"Could not get process snapshot!");
		return result;
	}

	const CHandle snapshotHandle(snapshot);

	PROCESSENTRY32 process;
	ZeroMemory(&process, sizeof(process));
	process.dwSize = sizeof(process);

	// enumerate all processes.  
	if (Process32First(snapshotHandle, &process))
	{
		do
		{
			if (_wcsicmp(process.szExeFile, name.c_str()) == 0)
			{
				// Process found, add to result. Request only the minimal access right: asking for
				// PROCESS_ALL_ACCESS fails (ERROR_ACCESS_DENIED) when the target runs at a higher
				// integrity level (e.g. Zoom relaunched elevated by its updater), which made a
				// running Zoom look as though it was not running.
				HANDLE processHandle = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, FALSE, process.th32ProcessID);
				if (processHandle != nullptr)
				{
					result.push_back(std::unique_ptr<void, HandleDeleter>(processHandle));
				}
				else
				{
					const std::wstring msg = L"OpenProcess failed for PID " + std::to_wstring(process.th32ProcessID) +
						L", error " + std::to_wstring(GetLastError()) + L"\n";
					OutputDebugString(msg.c_str());
				}
			}
		} while (Process32Next(snapshotHandle, &process));
	}

	return result;
}
