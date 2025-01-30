#include "RegistrationDialog.h"

RegistrationDialog::RegistrationDialog(std::function<void(const juce::String&, std::function<void(bool)>)> registrationFunc)
    : onRegister(registrationFunc)
{
    errorLabel.setColour(juce::Label::textColourId, juce::Colours::red);
    errorLabel.setJustificationType(juce::Justification::centred);
    addAndMakeVisible(errorLabel);

    serialNumberInput.setTextToShowWhenEmpty("Enter Serial Number", juce::Colours::grey);
    addAndMakeVisible(serialNumberInput);

    registerButton.setButtonText("Register");
    registerButton.onClick = [this] { attemptRegistration(); };
    addAndMakeVisible(registerButton);

    quitButton.setButtonText("Quit");
    quitButton.onClick = [this] { quit(); };
    addAndMakeVisible(quitButton);

    setSize(300, 160);
}

void RegistrationDialog::attemptRegistration()
{
    errorLabel.setText("", juce::dontSendNotification);
    registerButton.setEnabled(false);

    onRegister(serialNumberInput.getText(), [this](bool success)
    {
        registerButton.setEnabled(true);

        if (success)
        {
            if (auto* dw = findParentComponentOfClass<juce::DialogWindow>())
                dw->exitModalState(1);
        }
        else
        {
            errorLabel.setText("Invalid serial number. Please try again.", juce::dontSendNotification);
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

    errorLabel.setBounds(bounds.removeFromTop(20));
    bounds.removeFromTop(10);

    serialNumberInput.setBounds(bounds.removeFromTop(30));
    bounds.removeFromTop(15);

    registerButton.setBounds(bounds.removeFromTop(30));
    bounds.removeFromTop(15);

    quitButton.setBounds(bounds.removeFromTop(30));
}

void RegistrationDialog::show(std::function<void(const juce::String&, std::function<void(bool)>)> onRegister)
{
    auto dialog = std::make_unique<RegistrationDialog>(onRegister);

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
