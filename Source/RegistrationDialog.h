#pragma once
#include <JuceHeader.h>

class RegistrationDialog : public juce::Component
{
public:
    RegistrationDialog(std::function<void(const juce::String&, std::function<void(bool)>)> registrationFunc);

    static void show(std::function<void(const juce::String&, std::function<void(bool)>)> onRegister,
                    std::function<void()> onSuccess);

private:
    void attemptRegistration();
    void resized() override;
    void quit();

    std::function<void(const juce::String&, std::function<void(bool)>)> onRegister;
    juce::TextEditor serialNumberInput;
    juce::TextButton registerButton;
    juce::TextButton quitButton;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(RegistrationDialog)
};
