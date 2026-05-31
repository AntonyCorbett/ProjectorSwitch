# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project Overview

ProjectorSwitch is a Windows-only native C++ application that moves the Zoom media/projector window to a designated monitor and toggles it back to its original position. It supports both a GUI mode and headless command-line operation.

## Build

This is a Visual Studio 2022 solution (toolset v145, C++20, Unicode). Build via the IDE or MSBuild:

```
msbuild ProjectorSwitch.sln /p:Configuration=Release /p:Platform=x64
```

There are no tests, no linter, and no package manager scripts — it is a pure native Win32 C++ project. The only external dependency fetched automatically is the NuGet package `Microsoft.Windows.CppWinRT` (v2.0.220531.1).

## External Dependencies

Two sibling repositories must exist alongside this one:

- `../ApcLogger` — logging library
- `../ApcMonitorCore` — monitor/display enumeration library

The `.vcxproj` include paths and linker inputs reference these relative paths directly.

## Architecture

The application is structured around a set of single-responsibility service classes:

| Component | Files | Responsibility |
|---|---|---|
| **Main / GUI** | `ProjectorSwitch.cpp` | Win32 window, DPI scaling, dropdown monitor selector, toggle button, CLI argument parsing |
| **ZoomService** | `ZoomService.h/.cpp` | Core logic: find the Zoom media window via UI Automation, move it to the target monitor, restore it |
| **AutomationService** | `AutomationService.h/.cpp` | Thin wrapper around Windows UI Automation COM API |
| **ProcessesService** | `ProcessesService.h/.cpp` | Enumerate running processes via Toolhelp32 to locate the Zoom process |
| **SettingsService** | `SettingsService.h/.cpp` | Read/write `settings.ini` (selected monitor rectangle/key and saved window placement) |
| **WindowPlacementService** | `WindowPlacementService.h/.cpp` | Save and restore `WINDOWPLACEMENT` state |

Key wrapper/utility headers: `HandleDeleter.h`, `AutomationElementWrapper.h`, `AutomationConditionWrapper.h`, `VariantWrapper.h`.

### Control Flow

1. `WinMain` parses CLI flags (`--toggle`, `--no-gui`, `--monitor <key|index>`, `--help`).
2. Loads persisted monitor selection from `SettingsService`.
3. If GUI mode: creates a Win32 dialog with a monitor dropdown (populated from `ApcMonitorCore`) and a toggle button.
4. On toggle: `ZoomService` uses `ProcessesService` to confirm Zoom is running, then `AutomationService` to locate the Zoom media window element, then Win32 APIs (`SetWindowPos`, `ShowWindow`) to move/restore it. Original position is tracked via `WindowPlacementService`.
5. Result and selection are persisted back via `SettingsService`.

### Linked Libraries

`ComCtl32.lib` is the only non-standard system library linked; it provides the common controls (dropdown, button) used in the GUI.
