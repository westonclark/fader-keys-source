#include "FaderEngine.h"

FaderEngine::FaderEngine()
{
    setupMidiDevices();
}

FaderEngine::~FaderEngine()
{
    closeMidiDevices();
}

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

    // HUI Messages
    if (message.isController())
    {
        int controllerNumber = message.getControllerNumber();
        int value = message.getControllerValue();

        // DBG("Received MIDI CC - Controller: " << controllerNumber
        //                                       << " Value: " << value);

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

                // Log the complete fader value after update
                // DBG("Fader " << i << " updated - MSB: " << msb
                //              << " LSB: " << lsb
                //              << " Complete Value: " << newValue
                //              << " (0x" << juce::String::toHexString(newValue) << ")");

                break;
            }
        }
    }
}

void FaderEngine::nudgeFader(int faderIndex, int delta)
{
    int newValue = faderValues[faderIndex] + delta;
    newValue = juce::jlimit(0, 16383, newValue);

    faderValues[faderIndex] = newValue;

    sendFaderMove(faderIndex, newValue);
}

void FaderEngine::sendFaderMove(int faderIndex, int value)
{
    if (faderIndex < 0 || faderIndex >= numFaders || midiOutput == nullptr)
    {
        DBG("Invalid fader move: index=" << faderIndex
                                         << " output=" << (midiOutput != nullptr ? "yes" : "no"));
        return;
    }

    int lsb = value & 0x7F;
    int msb = (value >> 7) & 0x7F;

    uint8_t touchData1[3] = {0xB0, 0x0F, (uint8_t)faderIndex};
    uint8_t touchData2[3] = {0xB0, 0x2F, 0x40};
    uint8_t msbData[3] = {0xB0, (uint8_t)faderIndex, (uint8_t)msb};
    uint8_t lsbData[3] = {0xB0, (uint8_t)(0x20 | faderIndex), (uint8_t)lsb};
    uint8_t releaseData1[3] = {0xB0, 0x0F, (uint8_t)faderIndex};
    uint8_t releaseData2[3] = {0xB0, 0x2F, 0x00};
    // Touch message
    midiOutput->sendMessageNow(juce::MidiMessage(touchData1, 3));
    midiOutput->sendMessageNow(juce::MidiMessage(touchData2, 3));

    // Move message
    midiOutput->sendMessageNow(juce::MidiMessage(msbData, 3));
    midiOutput->sendMessageNow(juce::MidiMessage(lsbData, 3));

    // Release message
    midiOutput->sendMessageNow(juce::MidiMessage(releaseData1, 3));
    midiOutput->sendMessageNow(juce::MidiMessage(releaseData2, 3));
}

//==============================================================================
void FaderEngine::nudgeBankLeft()
{
    if (midiOutput != nullptr)
    {
        uint8_t zoneSelect[3] = {0xB0, 0x0F, 0x0A};
        uint8_t buttonPress[3] = {0xB0, 0x2F, 0x40};
        uint8_t buttonRelease[3] = {0xB0, 0x2F, 0x00};

        midiOutput->sendMessageNow(juce::MidiMessage(zoneSelect, 3));
        midiOutput->sendMessageNow(juce::MidiMessage(buttonPress, 3));
        midiOutput->sendMessageNow(juce::MidiMessage(buttonRelease, 3));
    }
}

void FaderEngine::nudgeBankRight()
{
    if (midiOutput != nullptr)
    {
        uint8_t zoneSelect[3] = {0xB0, 0x0F, 0x0A};
        uint8_t buttonPress[3] = {0xB0, 0x2F, 0x42};
        uint8_t buttonRelease[3] = {0xB0, 0x2F, 0x02};

        midiOutput->sendMessageNow(juce::MidiMessage(zoneSelect, 3));
        midiOutput->sendMessageNow(juce::MidiMessage(buttonPress, 3));
        midiOutput->sendMessageNow(juce::MidiMessage(buttonRelease, 3));
    }
}

void FaderEngine::handleGlobalKeycode(int keyCode, bool isKeyDown)
{
    if (!isKeyDown)
        return;

    // Values calibrated for ~0.5dB, ~1dB, and ~2dB movements
    // With slight boost to up movements to compensate
    static const struct
    {
        int up;
        int down;
    } NUDGE_VALUES[] = {
        {240, 160}, // Low - about 0.5dB (half of medium) + 64 boost up
        {384, 320}, // Medium - about 1dB + 32 boost up
        {704, 640}  // High - about 2dB (unchanged)
    };

    int nudgeUp, nudgeDown;
    switch (sensitivity)
    {
    case NudgeSensitivity::Low:
        nudgeUp = NUDGE_VALUES[0].up;
        nudgeDown = NUDGE_VALUES[0].down;
        break;
    case NudgeSensitivity::High:
        nudgeUp = NUDGE_VALUES[2].up;
        nudgeDown = NUDGE_VALUES[2].down;
        break;
    default:
        nudgeUp = NUDGE_VALUES[1].up;
        nudgeDown = NUDGE_VALUES[1].down;
        break;
    }

    // Use a static lookup table for fast key mapping
    static const struct
    {
        int keyCode;
        int faderIndex;
        bool isPositive;
    } keyMap[] = {
        {12, 0, true},  // 'q' - fader 0 up
        {0, 0, false},  // 'a' - fader 0 down
        {13, 1, true},  // 'w' - fader 1 up
        {1, 1, false},  // 's' - fader 1 down
        {14, 2, true},  // 'e' - fader 2 up
        {2, 2, false},  // 'd' - fader 2 down
        {15, 3, true},  // 'r' - fader 3 up
        {3, 3, false},  // 'f' - fader 3 down
        {17, 4, true},  // 't' - fader 4 up
        {5, 4, false},  // 'g' - fader 4 down
        {16, 5, true},  // 'y' - fader 5 up
        {4, 5, false},  // 'h' - fader 5 down
        {32, 6, true},  // 'u' - fader 6 up
        {38, 6, false}, // 'j' - fader 6 down
        {34, 7, true},  // 'i' - fader 7 up
        {40, 7, false}, // 'k' - fader 7 down
    };

    // Look up the action
    for (const auto &mapping : keyMap)
    {
        if (mapping.keyCode == keyCode)
        {
            // Use different values for up vs down
            int nudgeAmount = mapping.isPositive ? nudgeUp : -nudgeDown;
            nudgeFader(mapping.faderIndex, nudgeAmount);
            return;
        }
    }

    // Bank switching
    if (keyCode == 18) // '1' key
        nudgeBankLeft();
    else if (keyCode == 19) // '2' key
        nudgeBankRight();
}
