#include "MainComponent.h"
#include <map>
#include <cctype> // For std::tolower

//==============================================================================
MainComponent::MainComponent()
    : keyboardComponent(keyboardState, juce::MidiKeyboardComponent::horizontalKeyboard)
{
    // Make sure you set the size of the component after
    // you add any child components.
    setSize(800, 600);

    // Prevent the keyboard component from stealing focus
    keyboardComponent.setWantsKeyboardFocus(false);

    // Add the keyboard component to the main component
    addAndMakeVisible(keyboardComponent);

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

    // Register keyboard state listener
    keyboardState.addListener(this);

    // Some platforms require permissions to open input channels so request that here
    if (juce::RuntimePermissions::isRequired(juce::RuntimePermissions::recordAudio) &&
        !juce::RuntimePermissions::isGranted(juce::RuntimePermissions::recordAudio))
    {
        juce::RuntimePermissions::request(
            juce::RuntimePermissions::recordAudio,
            [&](bool granted)
            { setAudioChannels(granted ? 2 : 0, 2); });
    }
    else
    {
        // Specify the number of input and output channels that we want to open
        setAudioChannels(0, 2);
    }

    // Remove grabKeyboardFocus() from constructor
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

MainComponent::~MainComponent()
{
    // This shuts down the audio device and clears the audio source.
    shutdownAudio();

    // Close MIDI devices and listeners
    closeMidiDevices(); // Resets the unique_ptrs

    keyboardState.removeListener(this);
    removeKeyListener(this);
}

//==============================================================================
void MainComponent::prepareToPlay(int samplesPerBlockExpected,
                                  double sampleRate)
{
    // Not used since we're not processing audio
}

void MainComponent::getNextAudioBlock(
    const juce::AudioSourceChannelInfo &bufferToFill)
{
    // Since we're not producing any audio, clear the buffer
    bufferToFill.clearActiveBufferRegion();
}

void MainComponent::releaseResources()
{
    // Not used since we're not processing audio
}

//==============================================================================
void MainComponent::paint(juce::Graphics &g)
{
    // Fill the background
    g.fillAll(
        getLookAndFeel().findColour(juce::ResizableWindow::backgroundColourId));
}

void MainComponent::resized()
{
    // Position the keyboard component
    keyboardComponent.setBounds(10, getHeight() - 110, getWidth() - 20, 100);

    // Position the MIDI output selector
    midiOutputLabel.setBounds(10, 10, 80, 24);
    midiOutputSelector.setBounds(100, 10, getWidth() - 110, 24);

    // Position the MIDI input selector
    midiInputLabel.setBounds(10, 40, 80, 24);
    midiInputSelector.setBounds(100, 40, getWidth() - 110, 24);
}

// KeyListener callbacks
bool MainComponent::keyPressed(const juce::KeyPress &key, juce::Component *originatingComponent)
{
    DBG("Key Pressed: " << key.getTextDescription());
    // Map keys to MIDI notes starting from middle C
    int baseMidiNote = 60; // Middle C

    // Define a mapping from keys to MIDI note offsets
    std::map<char, int> keyToNoteOffset = {
        {'a', 0},  // C4
        {'s', 2},  // D4
        {'d', 4},  // E4
        {'f', 5},  // F4
        {'g', 7},  // G4
        {'h', 9},  // A4
        {'j', 11}, // B4
        {'k', 12}, // C5
        {'l', 14}, // D5
        {';', 16}, // E5
        {'\'', 17} // F5
    };

    char keyChar = key.getTextCharacter();
    keyChar = std::tolower(keyChar);

    if (keyToNoteOffset.find(keyChar) != keyToNoteOffset.end())
    {
        int midiNote = baseMidiNote + keyToNoteOffset[keyChar];
        float velocity = 1.0f;

        juce::MidiMessage noteOn = juce::MidiMessage::noteOn(1, midiNote, velocity);
        noteOn.setTimeStamp(juce::Time::getMillisecondCounterHiRes() * 0.001);

        // Send MIDI message
        if (midiOutput != nullptr)
            midiOutput->sendMessageNow(noteOn);

        // Update keyboard state
        keyboardState.noteOn(1, midiNote, velocity);

        return true; // Key event was handled
    }

    return false; // Key event was not handled
}

bool MainComponent::keyStateChanged(bool isKeyDown, juce::Component *originatingComponent)
{
    DBG("Key State Changed");

    // Handle key releases to send Note Off messages
    // Since JUCE doesn't call keyPressed on key release, we need to handle key state changes

    // Map keys to MIDI notes starting from middle C
    int baseMidiNote = 60; // Middle C

    // Define a mapping from keys to MIDI note offsets
    std::map<char, int> keyToNoteOffset = {
        {'a', 0},  // C4
        {'s', 2},  // D4
        {'d', 4},  // E4
        {'f', 5},  // F4
        {'g', 7},  // G4
        {'h', 9},  // A4
        {'j', 11}, // B4
        {'k', 12}, // C5
        {'l', 14}, // D5
        {';', 16}, // E5
        {'\'', 17} // F5
    };

    // Iterate over all mapped keys
    for (auto &keyMapping : keyToNoteOffset)
    {
        char keyChar = keyMapping.first;
        int midiNote = baseMidiNote + keyMapping.second;

        if (!juce::KeyPress::isKeyCurrentlyDown(keyChar))
        {
            // Key is released
            // Send Note Off message
            float velocity = 0.0f; // Velocity for Note Off
            juce::MidiMessage noteOff = juce::MidiMessage::noteOff(1, midiNote, velocity);
            noteOff.setTimeStamp(juce::Time::getMillisecondCounterHiRes() * 0.001);

            // Send MIDI message
            if (midiOutput != nullptr)
                midiOutput->sendMessageNow(noteOff);

            // Update keyboard state
            keyboardState.noteOff(1, midiNote, velocity);
        }
    }

    return false;
}

// MidiInputCallback implementation
void MainComponent::handleIncomingMidiMessage(juce::MidiInput *source, const juce::MidiMessage &message)
{
    // Handle incoming MIDI messages
    // For example, update the keyboard state
    keyboardState.processNextMidiEvent(message);
}

// MidiKeyboardStateListener implementation
void MainComponent::handleNoteOn(juce::MidiKeyboardState *source, int midiChannel, int midiNoteNumber, float velocity)
{
    // This is called when a note is pressed on the virtual keyboard
    // You can handle this event if needed
}

void MainComponent::handleNoteOff(juce::MidiKeyboardState *source, int midiChannel, int midiNoteNumber, float velocity)
{
    // This is called when a note is released on the virtual keyboard
    // You can handle this event if needed
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
