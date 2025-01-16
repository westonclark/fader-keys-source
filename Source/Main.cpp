/*
  ==============================================================================

    This file contains the basic startup code for a JUCE application.

  ==============================================================================
*/

#include <JuceHeader.h>
#include "FaderEngine.h"
#include "TrayIconComponent.h"
#include "GlobalKeyListener.h"

//==============================================================================
class FaderKeysApplication : public juce::JUCEApplication
{
public:
    FaderKeysApplication() {}

    const juce::String getApplicationName() override { return ProjectInfo::projectName; }
    const juce::String getApplicationVersion() override { return ProjectInfo::versionString; }
    bool moreThanOneInstanceAllowed() override { return true; }

    //==============================================================================
    void initialise(const juce::String &) override
    {
        // Create our core engine
        faderEngine = std::make_unique<FaderEngine>();

        // Start the global key listener (assuming you use the same global key approach as before).
        // The global key listener must know how to forward keycodes to faderEngine->handleGlobalKeycode.
        startGlobalKeyListener(faderEngine.get());

        // Create and show tray icon
        trayIcon = std::make_unique<TrayIconComponent>(*faderEngine);
    }

    void shutdown() override
    {
        stopGlobalKeyListener(); // from your GlobalKeyListener code
        trayIcon.reset();
        faderEngine.reset();
    }

    //==============================================================================
    void systemRequestedQuit() override
    {
        quit();
    }

    void anotherInstanceStarted(const juce::String &) override
    {
    }

private:
    std::unique_ptr<FaderEngine> faderEngine;
    std::unique_ptr<TrayIconComponent> trayIcon;
};

//==============================================================================
START_JUCE_APPLICATION(FaderKeysApplication)
