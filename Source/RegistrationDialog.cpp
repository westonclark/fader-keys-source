#include "RegistrationDialog.h"

RegistrationDialog::RegistrationDialog(std::function<void(const juce::String&, std::function<void(bool)>)> registrationFunc)
    : onRegister(registrationFunc)
{
    serialNumberInput.setTextToShowWhenEmpty("Enter Serial Number", juce::Colours::grey);
    addAndMakeVisible(serialNumberInput);

    registerButton.setButtonText("Register");
    registerButton.onClick = [this] { attemptRegistration(); };
    addAndMakeVisible(registerButton);

    quitButton.setButtonText("Quit");
    quitButton.onClick = [this] { quit(); };
    addAndMakeVisible(quitButton);

    setSize(300, 140); // Adjusted height without title
}

void RegistrationDialog::attemptRegistration()
{
    // Disable the button while checking
    registerButton.setEnabled(false);

    onRegister(serialNumberInput.getText(), [this](bool success)
    {
        registerButton.setEnabled(true);

        if (success)
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
    });
}

void RegistrationDialog::quit()
{
    juce::JUCEApplication::getInstance()->systemRequestedQuit();
}

void RegistrationDialog::resized()
{
    auto bounds = getLocalBounds().reduced(20);

    serialNumberInput.setBounds(bounds.removeFromTop(30));
    bounds.removeFromTop(15);

    registerButton.setBounds(bounds.removeFromTop(30));
    bounds.removeFromTop(15);

    quitButton.setBounds(bounds.removeFromTop(30));
}

void RegistrationDialog::show(std::function<void(const juce::String&, std::function<void(bool)>)> onRegister,
                            std::function<void()> onSuccess)
{
    auto dialog = std::make_unique<RegistrationDialog>(
        [onRegister, onSuccess](const juce::String& serial, std::function<void(bool)> callback)
        {
            onRegister(serial, [callback, onSuccess](bool success)
            {
                if (success)
                    onSuccess();
                if (callback)
                    callback(success);
            });
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
