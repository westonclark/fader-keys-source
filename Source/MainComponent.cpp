#include "MainComponent.h"
#include <cctype> // For std::tolower

//==============================================================================
MainComponent::MainComponent()
{
    setSize(800, 600);

    // Add KeyListener to capture keystrokes
    setWantsKeyboardFocus(true);
    addKeyListener(this);

    // Setup MIDI devices
    setupMidiDevices();

    // Add MIDI output selector
    addAndMakeVisible(midiOutputSelector);
    midiOutputSelector.onChange = [this]
    {
        closeMidiDevices();
        setupMidiDevices();
    };
    midiOutputLabel.setText("MIDI Output:", juce::dontSendNotification);
    addAndMakeVisible(midiOutputLabel);

    // Add MIDI input selector
    addAndMakeVisible(midiInputSelector);
    midiInputSelector.onChange = [this]
    {
        closeMidiDevices();
        setupMidiDevices();
    };
    midiInputLabel.setText("MIDI Input:", juce::dontSendNotification);
    addAndMakeVisible(midiInputLabel);

    // Populate MIDI devices
    auto midiOutputs = juce::MidiOutput::getAvailableDevices();
    for (auto &output : midiOutputs)
    {
        midiOutputSelector.addItem(output.name, output.identifier.hashCode());
    }

    auto midiInputs = juce::MidiInput::getAvailableDevices();
    for (auto &input : midiInputs)
    {
        midiInputSelector.addItem(input.name, input.identifier.hashCode());
    }

    // Initialize faders
    initializeFaders();
}

MainComponent::~MainComponent()
{
    closeMidiDevices();
    removeKeyListener(this);
}

//==============================================================================
void MainComponent::paint(juce::Graphics &g)
{
    // Fill the background
    g.fillAll(getLookAndFeel().findColour(juce::ResizableWindow::backgroundColourId));
}

void MainComponent::resized()
{
    auto area = getLocalBounds();

    // Position MIDI selectors
    auto midiArea = area.removeFromTop(70); // Reserve 70 pixels at the top for MIDI selectors

    // Divide midiArea into sections for each label and selector
    auto midiOutputArea = midiArea.removeFromTop(24);
    midiOutputLabel.setBounds(midiOutputArea.removeFromLeft(80));
    midiOutputSelector.setBounds(midiOutputArea.reduced(5, 0));

    auto midiInputArea = midiArea.removeFromTop(24);
    midiInputLabel.setBounds(midiInputArea.removeFromLeft(80));
    midiInputSelector.setBounds(midiInputArea.reduced(5, 0));

    // Position faders in the remaining area
    auto faderArea = area;
    int faderWidth = faderArea.getWidth() / numFaders;
    for (int i = 0; i < numFaders; ++i)
    {
        auto singleFaderArea = faderArea.removeFromLeft(faderWidth);
        faderLabels[i].setBounds(singleFaderArea.removeFromTop(24));
        faders[i].setBounds(singleFaderArea.reduced(10, 10));
    }
}

// KeyListener callbacks
bool MainComponent::keyPressed(const juce::KeyPress &key, juce::Component *originatingComponent)
{
    int keyCode = key.getTextCharacter();
    keyCode = std::tolower(keyCode);

    // Check if key is mapped to nudge up
    if (keyToFaderIndexUp.find(keyCode) != keyToFaderIndexUp.end())
    {
        int faderIndex = keyToFaderIndexUp[keyCode];
        nudgeFader(faderIndex, 128); // Nudge up by 128 (adjust as needed)
        return true;
    }

    // Check if key is mapped to nudge down
    if (keyToFaderIndexDown.find(keyCode) != keyToFaderIndexDown.end())
    {
        int faderIndex = keyToFaderIndexDown[keyCode];
        nudgeFader(faderIndex, -128); // Nudge down by 128 (adjust as needed)
        return true;
    }

    return false;
}

// MidiInputCallback implementation
void MainComponent::handleIncomingMidiMessage(juce::MidiInput *source, const juce::MidiMessage &message)
{
    // **Optional: Check if the message is on the expected MIDI channel**
    int midiChannel = message.getChannel();
    if (midiChannel != 1)
        return; // Ignore messages not on channel 1

    if (message.isController())
    {
        int controllerNumber = message.getControllerNumber();
        int value = message.getControllerValue();

        // Check if controller number corresponds to a fader
        // HUI uses CC 0-7 for MSB and 32-39 for LSB
        for (int i = 0; i < numFaders; ++i)
        {
            int msbCC = i;      // CC 0-7
            int lsbCC = i + 32; // CC 32-39

            if (controllerNumber == msbCC || controllerNumber == lsbCC)
            {
                // Update fader value
                int msb = faderValues[i] >> 7;
                int lsb = faderValues[i] & 0x7F;

                if (controllerNumber == msbCC)
                    msb = value & 0x7F;
                else
                    lsb = value & 0x7F;

                int newValue = (msb << 7) | lsb;
                faderValues[i] = newValue;

                // Update GUI from the MIDI thread safely
                juce::MessageManager::callAsync([this, i, newValue]()
                                                { faders[i].setValue(newValue, juce::dontSendNotification); });

                break;
            }
        }
    }
}

// Slider Listener
void MainComponent::sliderValueChanged(juce::Slider *slider)
{
    for (int i = 0; i < numFaders; ++i)
    {
        if (&faders[i] == slider)
        {
            int newValue = static_cast<int>(slider->getValue());
            faderValues[i] = newValue;
            sendFaderMove(i, newValue);
            break;
        }
    }
}

// Overridden to handle visibility change
void MainComponent::visibilityChanged()
{
    if (isShowing() && !hasFocusBeenSet)
    {
        grabKeyboardFocus();
        hasFocusBeenSet = true;
    }
}

// Setup MIDI devices
void MainComponent::setupMidiDevices()
{
    // Open selected MIDI output device
    if (midiOutput == nullptr)
    {
        auto selectedOutputId = midiOutputSelector.getSelectedId();
        auto midiOutputs = juce::MidiOutput::getAvailableDevices();

        for (auto &output : midiOutputs)
        {
            if (output.identifier.hashCode() == selectedOutputId)
            {
                midiOutput = juce::MidiOutput::openDevice(output.identifier);
                if (midiOutput != nullptr)
                {
                    // Send HUI initialization message
                    uint8_t sysexInit[] = {0xF0, 0x00, 0x00, 0x66, 0x0F, 0x01, 0xF7};
                    midiOutput->sendMessageNow(juce::MidiMessage(sysexInit, sizeof(sysexInit)));
                }
                break;
            }
        }
    }

    // Open selected MIDI input device
    if (midiInput == nullptr)
    {
        auto selectedInputId = midiInputSelector.getSelectedId();
        auto midiInputs = juce::MidiInput::getAvailableDevices();

        for (auto &input : midiInputs)
        {
            if (input.identifier.hashCode() == selectedInputId)
            {
                midiInput = juce::MidiInput::openDevice(input.identifier, this);
                if (midiInput != nullptr)
                    midiInput->start();
                break;
            }
        }
    }
}

// Close MIDI devices
void MainComponent::closeMidiDevices()
{
    if (midiOutput != nullptr)
    {
        midiOutput.reset(); // Resets the unique_ptr, closing the device
    }

    if (midiInput != nullptr)
    {
        midiInput->stop();
        midiInput.reset(); // Resets the unique_ptr, closing the device
    }
}

// Helper methods
void MainComponent::initializeFaders()
{
    // Initialize fader values and components
    for (int i = 0; i < numFaders; ++i)
    {
        // Initialize fader value to middle position (8192)
        faderValues[i] = 8192;

        // Setup fader slider
        faders[i].setSliderStyle(juce::Slider::LinearVertical);
        faders[i].setRange(0, 16383, 1);
        faders[i].setValue(faderValues[i]);
        faders[i].addListener(this);
        addAndMakeVisible(faders[i]);

        // Setup fader label
        faderLabels[i].setText("Fader " + juce::String(i + 1), juce::dontSendNotification);
        faderLabels[i].setJustificationType(juce::Justification::centred);
        addAndMakeVisible(faderLabels[i]);

        // Prevent faders from stealing focus
        faders[i].setWantsKeyboardFocus(false);
    }

    // Define key mappings for nudging faders
    // Up keys

    keyToFaderIndexUp['q'] = 0;
    keyToFaderIndexUp['w'] = 1;
    keyToFaderIndexUp['e'] = 2;
    keyToFaderIndexUp['r'] = 3;
    keyToFaderIndexUp['t'] = 4;
    keyToFaderIndexUp['y'] = 5;
    keyToFaderIndexUp['u'] = 6;
    keyToFaderIndexUp['i'] = 7;

    // Down keys
    keyToFaderIndexDown['a'] = 0;
    keyToFaderIndexDown['s'] = 1;
    keyToFaderIndexDown['d'] = 2;
    keyToFaderIndexDown['f'] = 3;
    keyToFaderIndexDown['g'] = 4;
    keyToFaderIndexDown['h'] = 5;
    keyToFaderIndexDown['j'] = 6;
    keyToFaderIndexDown['k'] = 7;
}

void MainComponent::nudgeFader(int faderIndex, int delta)
{
    // Adjust fader value
    int newValue = faderValues[faderIndex] + delta;

    // Clamp value between 0 and 16383
    newValue = juce::jlimit(0, 16383, newValue);

    faderValues[faderIndex] = newValue;

    // Update GUI safely
    juce::MessageManager::callAsync([this, faderIndex, newValue]()
                                    { faders[faderIndex].setValue(newValue, juce::dontSendNotification); });

    // Send MIDI message
    sendFaderMove(faderIndex, newValue);
}

void MainComponent::sendFaderMove(int faderIndex, int value)
{
    if (faderIndex < 0 || faderIndex >= numFaders || midiOutput == nullptr)
        return;

    // HUI protocol uses 14-bit values but in a specific way
    // Convert the 14-bit value (0-16383) to HUI format
    int huiValue = (value * 16384) / 16383; // Scale to ensure full range
    int msb = (huiValue >> 7) & 0x7F;
    int lsb = huiValue & 0x7F;

    // HUI uses channel 1
    const int midiChannel = 1;

    // In HUI, faders use CC 0-7 for MSB and 32-39 for LSB
    int msbCC = faderIndex;      // CC 0-7
    int lsbCC = faderIndex + 32; // CC 32-39

    // Send LSB first, then MSB (HUI protocol order)
    midiOutput->sendMessageNow(juce::MidiMessage::controllerEvent(midiChannel, lsbCC, lsb));
    midiOutput->sendMessageNow(juce::MidiMessage::controllerEvent(midiChannel, msbCC, msb));
}
