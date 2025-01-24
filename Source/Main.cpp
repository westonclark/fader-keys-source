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
        // Initialize properties
        appProperties = std::make_unique<juce::ApplicationProperties>();
        appProperties->setStorageParameters(getPropertyFileOptions());

        auto *settings = getAppProperties().getUserSettings();

        // Load the stored sensitivity
        auto lastSensitivity = (FaderEngine::NudgeSensitivity)settings->getIntValue("nudgeSensitivity",
                                                                                    (int)FaderEngine::NudgeSensitivity::Medium);

        faderEngine = std::make_unique<FaderEngine>();
        faderEngine->setNudgeSensitivity(lastSensitivity);

        // update menu to reflect loaded sensitivity
        TrayIconMac::createStatusBarIcon(faderEngine.get());
        TrayIconMac::updateSensitivityMenu(lastSensitivity);

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

    //==============================================================================
    juce::PropertiesFile::Options getPropertyFileOptions()
    {
        juce::PropertiesFile::Options options;
        options.applicationName = getApplicationName();
        options.filenameSuffix = ".settings";
        options.folderName = "FaderKeys";                    // settings folder
        options.osxLibrarySubFolder = "Application Support"; // lives in ~/Library/Application Support
        return options;
    }

    juce::ApplicationProperties &getAppProperties()
    {
        if (!appProperties)
        {
            appProperties = std::make_unique<juce::ApplicationProperties>();
            appProperties->setStorageParameters(getPropertyFileOptions());
        }
        return *appProperties;
    }

private:
    std::unique_ptr<FaderEngine> faderEngine;
    std::unique_ptr<juce::ApplicationProperties> appProperties;
};

//==============================================================================
START_JUCE_APPLICATION(FaderKeysApplication)
