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
        DBG("Failed to create the virtual MIDI output device!");

    midiInput = juce::MidiInput::createNewDevice("Fader Keys MIDI Input", this);
    if (midiInput == nullptr)
    {
        DBG("Failed to create the virtual MIDI input device!");
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
    // Pro Tools’s ping
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

    int nudgeAmount = fineTune ? 146 : 291;

    switch (keyCode)
    {
    case 0: // 'a'
        nudgeFader(0, -nudgeAmount);
        break;
    case 1: // 's'
        nudgeFader(1, -nudgeAmount);
        break;
    case 2: // 'd'
        nudgeFader(2, -nudgeAmount);
        break;
    case 3: // 'f'
        nudgeFader(3, -nudgeAmount);
        break;
    case 5: // 'g'
        nudgeFader(4, -nudgeAmount);
        break;
    case 4: // 'h'
        nudgeFader(5, -nudgeAmount);
        break;
    case 38: // 'j'
        nudgeFader(6, -nudgeAmount);
        break;
    case 40: // 'k'
        nudgeFader(7, -nudgeAmount);
        break;

    case 12: // 'q'
        nudgeFader(0, nudgeAmount);
        break;
    case 13: // 'w'
        nudgeFader(1, nudgeAmount);
        break;
    case 14: // 'e'
        nudgeFader(2, nudgeAmount);
        break;
    case 15: // 'r'
        nudgeFader(3, nudgeAmount);
        break;
    case 17: // 't'
        nudgeFader(4, nudgeAmount);
        break;
    case 16: // 'y'
        nudgeFader(5, nudgeAmount);
        break;
    case 32: // 'u'
        nudgeFader(6, nudgeAmount);
        break;
    case 34: // 'i'
        nudgeFader(7, nudgeAmount);
        break;

    case 18: // '1'
        nudgeBankLeft();
        break;
    case 19: // '2'
        nudgeBankRight();
        break;
    default:
        break;
    }
}
