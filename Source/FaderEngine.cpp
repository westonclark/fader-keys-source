#include "FaderEngine.h"

namespace
{
    // Nudge amounts for sensitivity levels
    static const std::array<std::pair<int, int>, 3> NUDGE_VALUES{{
        // Up, Down
        {240, 160}, // Low - Approx 0.5dB movement
        {384, 320}, // Medium - Approx 1.0dB movement
        {704, 640}  // High - Approx 2.0dB movement
    }};

    struct KeyMapping
    {
        int keyCode;
        int faderIndex;
        bool isUpward;
    };

    // Global keycode to fader mapping
    static const std::array<KeyMapping, 16> KEY_FADER_MAP{{
        // KeyCode, FaderIndex, IsUpward
        // Fader 1 controls
        {12, 0, true}, // Q - Fader 1 Up
        {0, 0, false}, // A - Fader 1 Down
        // Fader 2 controls
        {13, 1, true}, // W - Fader 2 Up
        {1, 1, false}, // S - Fader 2 Down
        // Fader 3 controls
        {14, 2, true}, // E - Fader 3 Up
        {2, 2, false}, // D - Fader 3 Down
        // Fader 4 controls
        {15, 3, true}, // R - Fader 4 Up
        {3, 3, false}, // F - Fader 4 Down
        // Fader 5 controls
        {17, 4, true}, // T - Fader 5 Up
        {5, 4, false}, // G - Fader 5 Down
        // Fader 6 controls
        {16, 5, true}, // Y - Fader 6 Up
        {4, 5, false}, // H - Fader 6 Down
        // Fader 7 controls
        {32, 6, true},  // U - Fader 7 Up
        {38, 6, false}, // J - Fader 7 Down
        // Fader 8 controls
        {34, 7, true},  // I - Fader 8 Up
        {40, 7, false}, // K - Fader 8 Down
    }};
}

// CONSTRUCTOR / DESTRUCTOR
//==============================================================================
FaderEngine::FaderEngine()
{
    setupMidiDevices();
}

FaderEngine::~FaderEngine()
{
    closeMidiDevices();
}

// MIDI DEVICE SETUP / TEARDOWN
//==============================================================================
void FaderEngine::setupMidiDevices()
{
    midiOutput = juce::MidiOutput::createNewDevice("Fader Keys MIDI Output");
    if (midiOutput == nullptr)
        DBG("Failed to create virtual MIDI output device");

    midiInput = juce::MidiInput::createNewDevice("Fader Keys MIDI Input", this);
    if (midiInput == nullptr)
    {
        DBG("Failed to create virtual MIDI input device");
    }
    else
    {
        midiInput->start();
    }
}

void FaderEngine::closeMidiDevices()
{
    if (midiInput != nullptr)
    {
        midiInput->stop();
        midiInput.reset();
    }
    midiOutput.reset();
}

// INCOMING MIDI MESSAGE HANDLING
//==============================================================================
void FaderEngine::handleIncomingMidiMessage(juce::MidiInput *,
                                            const juce::MidiMessage &message)
{
    // Pro Tools's ping
    if (message.isNoteOff() && message.getVelocity() == 0 && message.getNoteNumber() == 0)
    {
        if (midiOutput != nullptr)
            midiOutput->sendMessageNow(juce::MidiMessage::noteOn(1, 0, (uint8_t)127));
        return;
    }

    // Pro Tools HUI messages
    if (message.isController())
    {
        int controllerNumber = message.getControllerNumber();
        int value = message.getControllerValue();

        // HUI uses CC 0-7 for MSB and 32-39 for LSB
        for (int i = 0; i < numFaders; ++i)
        {
            int msbCC = i;
            int lsbCC = i + 32;

            if (controllerNumber == msbCC || controllerNumber == lsbCC)
            {
                int msb = faderValues[i] >> 7;
                int lsb = faderValues[i] & 0x7F;

                if (controllerNumber == msbCC)
                    msb = value & 0x7F;
                else
                    lsb = value & 0x7F;

                int newValue = (msb << 7) | lsb;
                faderValues[i] = newValue;

                break;
            }
        }
    }

    // Logic pitch wheel messages
    else if (message.isPitchWheel())
    {
        int channel = message.getChannel();
        if (channel >= 1 && channel <= numFaders)
        {
            faderValues[channel - 1] = message.getPitchWheelValue();
        }
    }
}

// GLOBAL KEYCODE HANDLING
//==============================================================================

void FaderEngine::handleGlobalKeycode(int keyCode, bool isKeyDown, bool isShiftDown)
{
    if (!isKeyDown)
        return;

    if (handleBankSwitching(keyCode, isShiftDown))
        return;

    // Get movement amounts based on current sensitivity or shift override
    const auto &nudgeAmounts = isShiftDown ? NUDGE_VALUES[static_cast<int>(NudgeSensitivity::High)] : // Use High sensitivity if shift is pressed
                                   NUDGE_VALUES[static_cast<int>(sensitivity)];                       // Otherwise use current sensitivity

    // Look for matching fader control
    for (const auto &mapping : KEY_FADER_MAP)
    {
        if (mapping.keyCode == keyCode)
        {
            const int delta = mapping.isUpward ? nudgeAmounts.first : -nudgeAmounts.second;
            nudgeFader(mapping.faderIndex, delta);
            return;
        }
    }
}

// FADER MOVEMENT
//==============================================================================
void FaderEngine::nudgeFader(int faderIndex, int delta)
{
    // Validate fader index and MIDI output
    if (faderIndex < 0 || faderIndex >= numFaders || midiOutput == nullptr)
    {
        DBG("Invalid fader move: index=" << faderIndex
                                         << " output=" << (midiOutput != nullptr ? "yes" : "no"));
        return;
    }

    // Calculate new value and limit to valid range (0-16383)
    int newValue = faderValues[faderIndex] + delta;
    newValue = juce::jlimit(0, 16383, newValue);

    // Store the new value
    faderValues[faderIndex] = newValue;

    const auto huiMessages = createFaderMoveMessages(faderIndex, newValue);
    const auto pitchWheelMessage = createPitchWheelMessage(faderIndex, newValue);

    // Send both HUI and pitch wheel messages
    for (const auto &msg : huiMessages)
    {
        midiOutput->sendMessageNow(msg);
    }
    midiOutput->sendMessageNow(pitchWheelMessage);
}

std::vector<juce::MidiMessage> FaderEngine::createFaderMoveMessages(int faderIndex, int value) const
{
    // Split 14-bit value into MSB and LSB (7 bits each)
    const int msb = (value >> 7) & 0x7F; // Most significant 7 bits
    const int lsb = value & 0x7F;        // Least significant 7 bits

    const std::array<std::pair<uint8_t, uint8_t>, 6> controls{{
        // Touch sequence
        {0x0F, static_cast<uint8_t>(faderIndex)}, // Select fader
        {0x2F, 0x40},                             // Apply touch pressure

        // Fader position
        {static_cast<uint8_t>(faderIndex), static_cast<uint8_t>(msb)},        // Set coarse position (MSB)
        {static_cast<uint8_t>(0x20 | faderIndex), static_cast<uint8_t>(lsb)}, // Set fine position (LSB)

        // Release sequence
        {0x0F, static_cast<uint8_t>(faderIndex)}, // Select fader again
        {0x2F, 0x00}                              // Remove pressure
    }};

    // Create MIDI messages from our control sequence
    std::vector<juce::MidiMessage> messages;
    messages.reserve(controls.size());

    for (const auto &[control, value] : controls)
    {
        messages.emplace_back(
            juce::MidiMessage::controllerEvent(
                1, // HUI protocol requires channel 1
                control,
                value));
    }

    return messages;
}

juce::MidiMessage FaderEngine::createPitchWheelMessage(int faderIndex, int value) const
{
    // Pitch wheel messages use channels 1-8 for faders 1-8
    const int midiChannel = faderIndex + 1;
    return juce::MidiMessage::pitchWheel(midiChannel, value);
}

// BANK SWITCHING
//==============================================================================
bool FaderEngine::handleBankSwitching(int keyCode, bool isShiftDown)
{
    switch (keyCode)
    {
    case 18: // '1' key
        if (isShiftDown)
            nudgeBankLeft8();
        else
            nudgeBankLeft();
        return true;
    case 19: // '2' key
        if (isShiftDown)
            nudgeBankRight8();
        else
            nudgeBankRight();
        return true;
    default:
        return false;
    }
}

void FaderEngine::nudgeBankLeft()
{
    if (midiOutput == nullptr)
        return;

    // Logic: Send C2 (58) note on/off for single track left
    midiOutput->sendMessageNow(juce::MidiMessage::noteOn(1, 48, (uint8_t)127));
    midiOutput->sendMessageNow(juce::MidiMessage::noteOff(1, 48));

    // Pro Tools: Zone select, button press, button release
    uint8_t zoneSelect[3] = {0xB0, 0x0F, 0x0A};
    uint8_t buttonPress[3] = {0xB0, 0x2F, 0x40};
    uint8_t buttonRelease[3] = {0xB0, 0x2F, 0x00};

    midiOutput->sendMessageNow(juce::MidiMessage(zoneSelect, 3));
    midiOutput->sendMessageNow(juce::MidiMessage(buttonPress, 3));
    midiOutput->sendMessageNow(juce::MidiMessage(buttonRelease, 3));
}

void FaderEngine::nudgeBankRight()
{
    if (midiOutput == nullptr)
        return;

    // Logic: Send C#2 (49) note on/off for single track right
    midiOutput->sendMessageNow(juce::MidiMessage::noteOn(1, 49, (uint8_t)127));
    midiOutput->sendMessageNow(juce::MidiMessage::noteOff(1, 49));

    // Pro Tools: Zone select, button press, button release
    uint8_t zoneSelect[3] = {0xB0, 0x0F, 0x0A};
    uint8_t buttonPress[3] = {0xB0, 0x2F, 0x42};
    uint8_t buttonRelease[3] = {0xB0, 0x2F, 0x02};

    midiOutput->sendMessageNow(juce::MidiMessage(zoneSelect, 3));
    midiOutput->sendMessageNow(juce::MidiMessage(buttonPress, 3));
    midiOutput->sendMessageNow(juce::MidiMessage(buttonRelease, 3));
}

void FaderEngine::nudgeBankLeft8()
{
    if (midiOutput == nullptr)
        return;

    // Logic: Send note 46 for bank left
    midiOutput->sendMessageNow(juce::MidiMessage::noteOn(1, 46, (uint8_t)127));
    midiOutput->sendMessageNow(juce::MidiMessage::noteOff(1, 46));

    // Pro Tools: Zone select (0A), button press, button release
    uint8_t zoneSelect[3] = {0xB0, 0x0F, 0x0A};
    uint8_t buttonPress[3] = {0xB0, 0x2F, 0x41}; // Using port 1 for bank left 8
    uint8_t buttonRelease[3] = {0xB0, 0x2F, 0x01};

    midiOutput->sendMessageNow(juce::MidiMessage(zoneSelect, 3));
    midiOutput->sendMessageNow(juce::MidiMessage(buttonPress, 3));
    midiOutput->sendMessageNow(juce::MidiMessage(buttonRelease, 3));
}

void FaderEngine::nudgeBankRight8()
{
    if (midiOutput == nullptr)
        return;

    // Logic: Send note 47 for bank right
    midiOutput->sendMessageNow(juce::MidiMessage::noteOn(1, 47, (uint8_t)127));
    midiOutput->sendMessageNow(juce::MidiMessage::noteOff(1, 47));

    // Pro Tools: Zone select (0A), button press, button release
    uint8_t zoneSelect[3] = {0xB0, 0x0F, 0x0A};
    uint8_t buttonPress[3] = {0xB0, 0x2F, 0x43}; // Using port 3 for bank right 8
    uint8_t buttonRelease[3] = {0xB0, 0x2F, 0x03};

    midiOutput->sendMessageNow(juce::MidiMessage(zoneSelect, 3));
    midiOutput->sendMessageNow(juce::MidiMessage(buttonPress, 3));
    midiOutput->sendMessageNow(juce::MidiMessage(buttonRelease, 3));
}
