#include <string>  
#include <atlbase.h>
#include <memory>
#include "ProcessesService.h"  
#include "HandleDeleter.h"

std::vector<std::unique_ptr<void, HandleDeleter>> ProcessesService::GetProcessesByName(std::wstring name)
{
	std::vector<std::unique_ptr<void, HandleDeleter>> result;

	DWORD pid = 0;

	// Create toolhelp snapshot.  
	HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
	if (snapshot == INVALID_HANDLE_VALUE)
	{
		OutputDebugString(L"Could not get process snapshot!");
		return result;
	}

	CHandle snapshotHandle(snapshot);

	PROCESSENTRY32 process;
	ZeroMemory(&process, sizeof(process));
	process.dwSize = sizeof(process);

	// enumerate all processes.  
	if (Process32First(snapshotHandle, &process))
	{
		do
		{
			if (std::wstring(process.szExeFile) == name)
			{
				// Process found, add to result.  
				HANDLE processHandle = OpenProcess(PROCESS_ALL_ACCESS, FALSE, process.th32ProcessID);
				if (processHandle != NULL)
				{					
					result.push_back(std::unique_ptr<void, HandleDeleter>(processHandle));
				}
			}
		} while (Process32Next(snapshotHandle, &process));
	}

	return result;
}
