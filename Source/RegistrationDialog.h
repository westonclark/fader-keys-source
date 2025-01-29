#pragma once
#include <JuceHeader.h>

class RegistrationDialog : public juce::Component
{
public:
    RegistrationDialog(std::function<bool(const juce::String&)> registrationFunc);

    static void show(std::function<bool(const juce::String&)> onRegister,
                    std::function<void()> onSuccess);

private:
    void attemptRegistration();
    void resized() override;

    std::function<bool(const juce::String&)> onRegister;
    juce::TextEditor serialNumberInput;
    juce::TextButton registerButton;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(RegistrationDialog)
};
