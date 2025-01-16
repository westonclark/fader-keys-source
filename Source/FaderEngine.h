#pragma once

#include <JuceHeader.h>
#include <array>
#include <map>

/**
    A non-visual class that encapsulates all of the old MainComponent’s logic:
    - MIDI input/output
    - Fader data values
    - Fine tune toggle
    - Global key handling
*/
class FaderEngine : public juce::MidiInputCallback
{
public:
    FaderEngine();
    ~FaderEngine() override;

    //==============================================================================
    // MidiInputCallback
    void handleIncomingMidiMessage(juce::MidiInput *source, const juce::MidiMessage &message) override;

    // Called by the tray icon menu in our example to toggle "fineTune".
    bool getFineTune() const { return fineTune; }
    void setFineTune(bool shouldBeFine) { fineTune = shouldBeFine; }

    // The old "handleGlobalKeycode" from MainComponent:
    void handleGlobalKeycode(int keyCode, bool isKeyDown);

private:
    //==============================================================================
    // MIDI devices
    void setupMidiDevices();
    void closeMidiDevices();

    std::unique_ptr<juce::MidiOutput> midiOutput;
    std::unique_ptr<juce::MidiInput> midiInput;

    // The old fader logic
    static constexpr int numFaders = 8;
    std::array<int, numFaders> faderValues; // Stores 14-bit values (0 - 16383)

    // Our “fineTune” mode
    bool fineTune = false;

    // Key mappings
    std::map<int, int> keyToFaderIndexUp;
    std::map<int, int> keyToFaderIndexDown;

    // Fader methods
    void initializeFaders();
    void nudgeFader(int faderIndex, int delta);
    void sendFaderMove(int faderIndex, int value);

    // Bank methods
    void nudgeBankLeft();
    void nudgeBankRight();

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(FaderEngine)
};
