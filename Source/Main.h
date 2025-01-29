/*
  ==============================================================================

    Main.h
    Created: [current_date]
    Author:  Weston Clark

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include "FaderEngine.h"

//==============================================================================
class FaderKeysApplication : public juce::JUCEApplication
{
public:
    FaderKeysApplication() = default;

    const juce::String getApplicationName() override { return ProjectInfo::projectName; }
    const juce::String getApplicationVersion() override { return ProjectInfo::versionString; }
    bool moreThanOneInstanceAllowed() override { return true; }

    void initialise(const juce::String &) override;
    void shutdown() override;
    void systemRequestedQuit() override;
    void anotherInstanceStarted(const juce::String &) override;

    juce::ApplicationProperties &getAppProperties();

    bool isRegistered() const;
    bool registerSerialNumber(const juce::String& serialNumber);

private:
    juce::PropertiesFile::Options getPropertyFileOptions();

    std::unique_ptr<FaderEngine> faderEngine;
    std::unique_ptr<juce::ApplicationProperties> appProperties;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(FaderKeysApplication)
};
