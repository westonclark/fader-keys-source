#pragma once

#if JUCE_MAC

#include "FaderEngine.h"

// Wraping native macOS status bar creation
namespace TrayIconMac
{
    void createStatusBarIcon(FaderEngine *engine);

    void removeStatusBarIcon();

    void updateSensitivityMenu(FaderEngine::NudgeSensitivity sensitivity);

    void updateCapsLockState(bool capsLockOn);
}

#endif
