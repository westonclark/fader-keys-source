#include "MainComponent.h"
#include <cctype>

//==============================================================================
MainComponent::MainComponent()
{
    setSize(800, 600);

    setWantsKeyboardFocus(true);
    addKeyListener(this);
    setupMidiDevices();
    initializeFaders();

    fineTuneButton.setButtonText("Fine Tune");
    fineTuneButton.setToggleState(false, juce::dontSendNotification);
    addAndMakeVisible(fineTuneButton);
}

MainComponent::~MainComponent()
{
    closeMidiDevices();
    removeKeyListener(this);
}

//==============================================================================
void MainComponent::paint(juce::Graphics &g)
{
    g.fillAll(getLookAndFeel().findColour(juce::ResizableWindow::backgroundColourId));
}

void MainComponent::resized()
{
    auto area = getLocalBounds();

    fineTuneButton.setBounds(area.removeFromTop(30).reduced(10, 5));

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

    int nudgeAmount = fineTune ? 160 : 320;

    // nudge fader up
    if (keyToFaderIndexUp.find(keyCode) != keyToFaderIndexUp.end())
    {
        int faderIndex = keyToFaderIndexUp[keyCode];
        nudgeFader(faderIndex, nudgeAmount);
        return true;
    }
    // nudge faderdown
    if (keyToFaderIndexDown.find(keyCode) != keyToFaderIndexDown.end())
    {
        int faderIndex = keyToFaderIndexDown[keyCode];
        nudgeFader(faderIndex, -nudgeAmount);
        return true;
    }

    // nudge bank left
    if (key == juce::KeyPress('1'))
    {

        nudgeBankLeft();
        return true;
    }
    // nudge bank right
    if (key == juce::KeyPress('2'))
    {
        nudgeBankRight();
        return true;
    }

    return false;
}

void MainComponent::handleIncomingMidiMessage(juce::MidiInput *source, const juce::MidiMessage &message)
{

    // Protools technically sends a note on with velocity 0 for the ping, but juice interprets it as a note off
    if (message.isNoteOff() && message.getVelocity() == 0 && message.getNoteNumber() == 0)
    {
        if (midiOutput != nullptr)
            midiOutput->sendMessageNow(juce::MidiMessage::noteOn(1, 0, (uint8_t)127));
        return;
    }

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

                // update GUI from the MIDI thread safely
                juce::MessageManager::callAsync([this, i, newValue]()
                                                { faders[i].setValue(newValue, juce::dontSendNotification); });

                break;
            }
        }
    }
}

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

void MainComponent::setupMidiDevices()
{
    midiOutput = juce::MidiOutput::createNewDevice("Fader Keys MIDI Output");
    if (midiOutput == nullptr)
    {
        DBG("Failed to create the virtual MIDI output device!");
    }

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

void MainComponent::closeMidiDevices()
{
    if (midiInput != nullptr)
    {
        midiInput->stop();
        midiInput.reset();
    }
    midiOutput.reset();
}

void MainComponent::initializeFaders()
{
    for (int i = 0; i < numFaders; ++i)
    {
        // initialize fader value in code only, but don't externally notify the daw
        // use dontSendNotification to avoid triggering sliderValueChanged().
        faderValues[i] = 12256;
        faders[i].setSliderStyle(juce::Slider::LinearVertical);
        faders[i].setRange(0, 16383, 1);
        faders[i].setValue(faderValues[i], juce::dontSendNotification);
        faders[i].addListener(this);
        addAndMakeVisible(faders[i]);

        // set fader label
        faderLabels[i].setText("Fader " + juce::String(i + 1), juce::dontSendNotification);
        faderLabels[i].setJustificationType(juce::Justification::centred);
        addAndMakeVisible(faderLabels[i]);
    }

    fineTuneButton.onClick = [this]()
    {
        fineTune = fineTuneButton.getToggleState();
    };

    // key mappings for nudging faders
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
    int newValue = faderValues[faderIndex] + delta;
    newValue = juce::jlimit(0, 16383, newValue);
    faderValues[faderIndex] = newValue;

    // update GUI safely
    juce::MessageManager::callAsync([this, faderIndex, newValue]()
                                    { faders[faderIndex].setValue(newValue, juce::dontSendNotification); });

    sendFaderMove(faderIndex, newValue);
}

void MainComponent::sendFaderMove(int faderIndex, int value)
{
    if (faderIndex < 0 || faderIndex >= numFaders || midiOutput == nullptr)
    {
        DBG("Invalid fader move: index=" << faderIndex << " output=" << (midiOutput != nullptr ? "yes" : "no"));
        return;
    }

    int lsb = value & 0x7F;
    int msb = (value >> 7) & 0x7F;

    // touch message
    uint8_t touchData1[3] = {0xB0, 0x0F, (uint8_t)faderIndex};
    uint8_t touchData2[3] = {0xB0, 0x2F, 0x40};

    // fader position
    uint8_t msbData[3] = {0xB0, (uint8_t)faderIndex, (uint8_t)msb};
    uint8_t lsbData[3] = {0xB0, (uint8_t)(0x20 | faderIndex), (uint8_t)lsb};

    // release message
    uint8_t releaseData1[3] = {0xB0, 0x0F, (uint8_t)faderIndex};
    uint8_t releaseData2[3] = {0xB0, 0x2F, 0x00};

    midiOutput->sendMessageNow(juce::MidiMessage(touchData1, 3));
    midiOutput->sendMessageNow(juce::MidiMessage(touchData2, 3));
    midiOutput->sendMessageNow(juce::MidiMessage(msbData, 3));

    midiOutput->sendMessageNow(juce::MidiMessage(lsbData, 3));
    midiOutput->sendMessageNow(juce::MidiMessage(releaseData1, 3));
    midiOutput->sendMessageNow(juce::MidiMessage(releaseData2, 3));
}

void MainComponent::nudgeBankLeft()
{
    if (midiOutput != nullptr)
    {
        uint8_t zoneSelect[3] = {0xB0, 0x0F, 0x0A};    // Select zone 0x0A
        uint8_t buttonPress[3] = {0xB0, 0x2F, 0x40};   // Port 0 on
        uint8_t buttonRelease[3] = {0xB0, 0x2F, 0x00}; // Port 0 off

        midiOutput->sendMessageNow(juce::MidiMessage(zoneSelect, 3));
        midiOutput->sendMessageNow(juce::MidiMessage(buttonPress, 3));
        midiOutput->sendMessageNow(juce::MidiMessage(buttonRelease, 3));
    }
}

void MainComponent::nudgeBankRight()
{
    if (midiOutput != nullptr)
    {
        uint8_t zoneSelect[3] = {0xB0, 0x0F, 0x0A};    // Select zone 0x0A
        uint8_t buttonPress[3] = {0xB0, 0x2F, 0x42};   // Port 2 on
        uint8_t buttonRelease[3] = {0xB0, 0x2F, 0x02}; // Port 2 off

        midiOutput->sendMessageNow(juce::MidiMessage(zoneSelect, 3));
        midiOutput->sendMessageNow(juce::MidiMessage(buttonPress, 3));
        midiOutput->sendMessageNow(juce::MidiMessage(buttonRelease, 3));
    }
}

void MainComponent::handleGlobalKeycode(int keyCode, bool isKeyDown)
{
    // We only want to act on key down:
    if (!isKeyDown)
        return;

    // If fineTune is true, use smaller increments
    int nudgeAmount = fineTune ? 160 : 320;

    // DBG("Global key pressed, mac keyCode=" << keyCode);

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
