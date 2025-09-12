#include <Windows.h>
#include "WindowPlacementService.h"

/// Saves the current window placement of the specified window to persistent storage.
void WindowPlacementService::SaveWindowPlace(const HWND mainWindow) const
{
    WINDOWPLACEMENT placement;
    placement.length = sizeof(WINDOWPLACEMENT);

    GetWindowPlacement(mainWindow, &placement);
    settingsService_->SaveWindowPlacement(placement);
}

/// Validates that the provided WINDOWPLACEMENT structure has a valid normal position.
bool WindowPlacementService::IsValidPlacement(const WINDOWPLACEMENT& placement)
{
    return
        placement.rcNormalPosition.bottom - placement.rcNormalPosition.top > 0 &&
        placement.rcNormalPosition.right - placement.rcNormalPosition.left > 0;
}

/// Restores the window placement of the specified window from persistent storage.
void WindowPlacementService::RestoreWindowPlace(const HWND mainWindow) const
{
    WINDOWPLACEMENT placement;
    placement.length = sizeof(WINDOWPLACEMENT);

    GetWindowPlacement(mainWindow, &placement);

    const auto storedPlacement = settingsService_->LoadWindowPlacement();
    if (!IsValidPlacement(storedPlacement))
    {
        return;
    }

    const auto normalWidth = placement.rcNormalPosition.right - placement.rcNormalPosition.left;
    const auto normalHeight = placement.rcNormalPosition.bottom - placement.rcNormalPosition.top;

    placement.showCmd = storedPlacement.showCmd;
    placement.flags = 0;
    placement.rcNormalPosition.top = storedPlacement.rcNormalPosition.top;
    placement.rcNormalPosition.left = storedPlacement.rcNormalPosition.left;
    placement.rcNormalPosition.bottom = storedPlacement.rcNormalPosition.top + normalHeight;
    placement.rcNormalPosition.right = storedPlacement.rcNormalPosition.left + normalWidth;

    SetWindowPlacement(mainWindow, &placement);
}
