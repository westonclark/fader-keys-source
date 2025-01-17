#pragma once

#if JUCE_MAC // or #ifdef __APPLE__—use the same condition from your build config

class FaderEngine;

// Wrap native macOS status bar creation
namespace TrayIconMac
{
    // Creates the NSStatusItem, attaches a native macOS menu.
    // Call this once at startup, passing your FaderEngine pointer so
    // the menu item can toggle “fine tune” as needed.
    void createStatusBarIcon(FaderEngine *engine);

    // Destroys the status item (call at shutdown).
    void removeStatusBarIcon();

    // Optionally, call this if you need to update the menu title etc.
    // for dynamic changes
    void updateMenuTitle(bool fineTuneEnabled);
}

#endif
