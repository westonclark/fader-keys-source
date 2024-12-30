#include "MainComponent.h"
#include <cctype>

//==============================================================================
MainComponent::MainComponent()
{
    setSize(800, 600);

    // Add KeyListener to capture keystrokes
    setWantsKeyboardFocus(true);
    addKeyListener(this);

    // Setup MIDI devices
    setupMidiDevices();

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

    auto faderArea = area;
    int faderWidth = faderArea.getWidth() / numFaders;
    for (int i = 0; i < numFaders; ++i)
    {
        auto singleFaderArea = faderArea.removeFromLeft(faderWidth);
        faderLabels[i].setBounds(singleFaderArea.removeFromTop(24));
        faders[i].setBounds(singleFaderArea.reduced(10, 10));
    }
}

bool MainComponent::keyPressed(const juce::KeyPress &key, juce::Component *originatingComponent)
{
    int keyCode = key.getTextCharacter();
    keyCode = std::tolower(keyCode);

    if (keyToFaderIndexUp.find(keyCode) != keyToFaderIndexUp.end())
    {
        int faderIndex = keyToFaderIndexUp[keyCode];
        nudgeFader(faderIndex, 320);
        return true;
    }

    if (keyToFaderIndexDown.find(keyCode) != keyToFaderIndexDown.end())
    {
        int faderIndex = keyToFaderIndexDown[keyCode];
        nudgeFader(faderIndex, -320);
        return true;
    }

    return false;
}

// MidiInputCallback implementation
void MainComponent::handleIncomingMidiMessage(juce::MidiInput *source, const juce::MidiMessage &message)
{

    // Some versions of Pro Tools use Note Off #0 Vel 64 as a 'ping'
    if (message.getRawDataSize() == 3)
    {
        auto status = message.getRawData()[0];
        auto data1 = message.getRawData()[1];
        auto data2 = message.getRawData()[2];

        // Check if it’s note-off on channel 1, note #0, velocity 64
        // i.e. status = 0x80, data1 = 0, data2 = 64
        if ((status == 0x80) && (data1 == 0x00) && (data2 == 0x40))
        {
            DBG("Got Pro Tools ping (Note Off #0, vel 64), replying with noteOn #0, vel 127");
            if (midiOutput != nullptr)
            {
                // Typical HUI reply
                // channel=1, note=0, velocity=127 => 0x90 0x00 0x7F
                midiOutput->sendMessageNow(juce::MidiMessage::noteOn(1, 0, (uint8_t)127));
            }
            return;
        }
    }

    // handle the “classic” ping version
    // if (message.isNoteOn() && message.getNoteNumber() == 0 && message.getVelocity() == 0)
    // {
    //     if (midiOutput != nullptr)
    //         midiOutput->sendMessageNow(juce::MidiMessage::noteOn(1, 0, (uint8_t)127));
    //     return;
    // }

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
                // update fader value
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
    // Open MIDI output device
    auto midiOutputs = juce::MidiOutput::getAvailableDevices();
    for (auto &output : midiOutputs)
    {
        if (output.name == "IAC Driver Bus 1")
        {
            midiOutput = juce::MidiOutput::openDevice(output.identifier);
            if (midiOutput != nullptr)
            {
                // Send HUI initialization message
                uint8_t sysexInit[] = {0xF0, 0x00, 0x00, 0x66, 0x0F, 0x01, 0xF7};
                midiOutput->sendMessageNow(juce::MidiMessage(sysexInit, sizeof(sysexInit)));

                startTimer(1000); // ping once every 1000 ms

                // requestFaderPositions();
            }
            break;
        }
    }

    // Open MIDI input device
    auto midiInputs = juce::MidiInput::getAvailableDevices();
    for (auto &input : midiInputs)
    {
        if (input.name == "IAC Driver Bus 1")
        {
            midiInput = juce::MidiInput::openDevice(input.identifier, this);
            if (midiInput != nullptr)
                midiInput->start();
            break;
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
    for (int i = 0; i < numFaders; ++i)
    {
        // Initialize fader value in code only, but don't externally notify Pro Tools here
        faderValues[i] = 12256;

        // Set up the slider. Use dontSendNotification to avoid triggering sliderValueChanged().
        faders[i].setSliderStyle(juce::Slider::LinearVertical);
        faders[i].setRange(0, 16383, 1);
        faders[i].setValue(faderValues[i], juce::dontSendNotification);
        faders[i].addListener(this);
        addAndMakeVisible(faders[i]);

        // Setup fader label
        faderLabels[i].setText("Fader " + juce::String(i + 1), juce::dontSendNotification);
        faderLabels[i].setJustificationType(juce::Justification::centred);
        addAndMakeVisible(faderLabels[i]);

        // Prevent faders from stealing keyboard focus
        faders[i].setWantsKeyboardFocus(false);
    }

    // Define key mappings for nudging faders
    keyToFaderIndexUp['q'] = 0;
    keyToFaderIndexUp['w'] = 1;
    keyToFaderIndexUp['e'] = 2;
    keyToFaderIndexUp['r'] = 3;
    keyToFaderIndexUp['t'] = 4;
    keyToFaderIndexUp['y'] = 5;
    keyToFaderIndexUp['u'] = 6;
    keyToFaderIndexUp['i'] = 7;

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
    {
        DBG("Invalid fader move: index=" << faderIndex << " output=" << (midiOutput != nullptr ? "yes" : "no"));
        return;
    }

    // Convert the 14-bit value (0-16383) to LSB and MSB
    int lsb = value & 0x7F;        // Get lower 7 bits
    int msb = (value >> 7) & 0x7F; // Get upper 7 bits

    // Send 'touch fader' message
    uint8_t touchData1[3] = {0xB0, 0x0F, (uint8_t)faderIndex};
    uint8_t touchData2[3] = {0xB0, 0x2F, 0x40};

    // Send fader position
    uint8_t msbData[3] = {0xB0, (uint8_t)faderIndex, (uint8_t)msb};
    uint8_t lsbData[3] = {0xB0, (uint8_t)(0x20 | faderIndex), (uint8_t)lsb};

    // Send 'release fader' message
    uint8_t releaseData1[3] = {0xB0, 0x0F, (uint8_t)faderIndex};
    uint8_t releaseData2[3] = {0xB0, 0x2F, 0x00};

    // Send the complete sequence
    midiOutput->sendMessageNow(juce::MidiMessage(touchData1, 3)); // Touch start 1
    midiOutput->sendMessageNow(juce::MidiMessage(touchData2, 3)); // Touch start 2

    midiOutput->sendMessageNow(juce::MidiMessage(msbData, 3)); // Position MSB
    midiOutput->sendMessageNow(juce::MidiMessage(lsbData, 3)); // Position LSB

    midiOutput->sendMessageNow(juce::MidiMessage(releaseData1, 3)); // Touch end 1
    midiOutput->sendMessageNow(juce::MidiMessage(releaseData2, 3)); // Touch end 2
}

// void MainComponent::requestFaderPositions()
// {
//     if (midiOutput == nullptr)
//         return;

//     // In HUI protocol, we can request fader positions by sending a "touch" message
//     // for each fader and then releasing it
//     for (int i = 0; i < numFaders; ++i)
//     {
//         uint8_t touchData1[3] = {0xB0, 0x0F, (uint8_t)i};
//         uint8_t touchData2[3] = {0xB0, 0x2F, 0x40};
//         uint8_t releaseData1[3] = {0xB0, 0x0F, (uint8_t)i};
//         uint8_t releaseData2[3] = {0xB0, 0x2F, 0x00};

//         midiOutput->sendMessageNow(juce::MidiMessage(touchData1, 3));
//         midiOutput->sendMessageNow(juce::MidiMessage(touchData2, 3));
//         midiOutput->sendMessageNow(juce::MidiMessage(releaseData1, 3));
//         midiOutput->sendMessageNow(juce::MidiMessage(releaseData2, 3));
//     }
// }

void MainComponent::timerCallback()
{
    // Send a "ping" to Pro Tools about once a second (or half a second).
    // For HUI, that is: Note On, channel 1, note #0, velocity 0
    // i.e. 0x90 0x00 0x00 in raw MIDI.
    if (midiOutput != nullptr)
    {
        // This is our outgoing ping
        midiOutput->sendMessageNow(juce::MidiMessage::noteOn(1, 0, (uint8_t)0));
    }
}
