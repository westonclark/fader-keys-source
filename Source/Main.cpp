/*
  ==============================================================================

    This file contains the basic startup code for a JUCE application.

  ==============================================================================
*/

#include <JuceHeader.h>
#include "FaderEngine.h"
#include "GlobalKeyListener.h"
#include "TrayIconMac.h"

//==============================================================================
class FaderKeysApplication : public juce::JUCEApplication
{
public:
    FaderKeysApplication()
    {
    }

    const juce::String getApplicationName() override { return ProjectInfo::projectName; }
    const juce::String getApplicationVersion() override { return ProjectInfo::versionString; }
    bool moreThanOneInstanceAllowed() override { return true; }

    //==============================================================================
    void initialise(const juce::String &) override
    {
        // Create the core engine
        faderEngine = std::make_unique<FaderEngine>();

        // Create and show tray icon
        TrayIconMac::createStatusBarIcon(faderEngine.get());

        // Start the global key listener
        startGlobalKeyListener(faderEngine.get());
    }

    void shutdown() override
    {
        TrayIconMac::removeStatusBarIcon();
        stopGlobalKeyListener();
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
};

//==============================================================================
START_JUCE_APPLICATION(FaderKeysApplication)
