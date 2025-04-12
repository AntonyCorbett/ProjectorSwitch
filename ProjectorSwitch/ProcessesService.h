#pragma once
#include <Windows.h>
#include <TlHelp32.h>
#include <string>
#include <vector>

class ProcessesService
{
public:
	std::vector<HANDLE> GetProcessesByName(std::wstring name);
};

