#pragma once
#include <JuceHeader.h>

class RegistrationManager
{
public:
    RegistrationManager(juce::ApplicationProperties& properties);

    bool isRegistered() const;
    // bool registerSerialNumber(const juce::String& serialNumber);
    void registerSerialNumberAsync(const juce::String& serialNumber,
                                   std::function<void(bool)> callback);

private:
    bool validateSerialNumber(const juce::String& serialNumber);

    juce::ApplicationProperties& appProperties;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(RegistrationManager)
};
