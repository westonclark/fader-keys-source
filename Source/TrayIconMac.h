#pragma once

#if JUCE_MAC

class FaderEngine;

// Wraping native macOS status bar creation
namespace TrayIconMac
{
    // Creates the NSStatusItem, attaches a native macOS menu.
    void createStatusBarIcon(FaderEngine *engine);

    // Destroys the status item (call at shutdown).
    void removeStatusBarIcon();

    // Updates the menu title to reflect the current fineTune state
    void updateMenuTitle(bool fineTuneEnabled);
}

#endif
