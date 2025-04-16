#pragma once
#include <Windows.h>
#include <TlHelp32.h>
#include <string>
#include <vector>
#include <memory>
#include "HandleDeleter.h"

class ProcessesService
{
public:
	std::vector<std::unique_ptr<void, HandleDeleter>> GetProcessesByName(std::wstring name);
};

