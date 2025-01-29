#include "RegistrationDialog.h"

RegistrationDialog::RegistrationDialog(std::function<bool(const juce::String&)> registrationFunc)
    : onRegister(registrationFunc)
{
    serialNumberInput.setTextToShowWhenEmpty("Enter Serial Number", juce::Colours::grey);
    addAndMakeVisible(serialNumberInput);

    registerButton.setButtonText("Register");
    registerButton.onClick = [this] { attemptRegistration(); };
    addAndMakeVisible(registerButton);

    setSize(300, 100);
}

void RegistrationDialog::attemptRegistration()
{
    if (onRegister(serialNumberInput.getText()))
    {
        // If registration is successful, close the window
        if (auto* dw = findParentComponentOfClass<juce::DialogWindow>())
            dw->exitModalState(1);  // 1 indicates success
    }
    else
    {
        juce::AlertWindow::showMessageBoxAsync(
            juce::MessageBoxIconType::WarningIcon,
            "Invalid Serial Number",
            "Please enter a valid serial number.");
    }
}

void RegistrationDialog::resized()
{
    auto bounds = getLocalBounds().reduced(10);
    serialNumberInput.setBounds(bounds.removeFromTop(30));
    bounds.removeFromTop(10);
    registerButton.setBounds(bounds.removeFromTop(30));
}

void RegistrationDialog::show(std::function<bool(const juce::String&)> onRegister,
                            std::function<void()> onSuccess)
{
    auto dialog = std::make_unique<RegistrationDialog>(
        [onRegister, onSuccess](const juce::String& serial)
        {
            if (onRegister(serial))
            {
                onSuccess();
                return true;
            }
            return false;
        });

    juce::DialogWindow::LaunchOptions options;
    options.content.setOwned(dialog.release());
    options.dialogTitle = "Register Fader Keys";
    options.dialogBackgroundColour = juce::Colours::darkgrey;
    options.escapeKeyTriggersCloseButton = false;
    options.useNativeTitleBar = true;
    options.resizable = false;

    if (auto* window = options.launchAsync())
    {
        window->setAlwaysOnTop(true);
        window->toFront(true);
        window->setVisible(true);
    }
}

// Rest of the implementation remains the same as your original RegistrationComponent
