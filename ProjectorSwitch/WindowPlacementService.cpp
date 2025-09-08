#include <Windows.h>
#include "WindowPlacementService.h"

void WindowPlacementService::SaveWindowPlace(const HWND mainWindow) const
{
    WINDOWPLACEMENT placement;
    placement.length = sizeof(WINDOWPLACEMENT);

    GetWindowPlacement(mainWindow, &placement);
    settingsService_->SaveWindowPlacement(placement);
}

bool WindowPlacementService::IsValidPlacement(const WINDOWPLACEMENT& placement)
{
    return
        placement.rcNormalPosition.bottom - placement.rcNormalPosition.top > 0 &&
        placement.rcNormalPosition.right - placement.rcNormalPosition.left > 0;
}

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
