#pragma once

#include <JuceHeader.h>
#include <array>

class FaderEngine : public juce::MidiInputCallback
{
public:
    FaderEngine();
    ~FaderEngine() override = default;

    void handleIncomingMidiMessage(juce::MidiInput *source, const juce::MidiMessage &message) override;

    // Called by the tray icon menu to toggle fineTune
    bool getFineTune() const { return fineTune; }
    void setFineTune(bool shouldBeFine) { fineTune = shouldBeFine; }

    void handleGlobalKeycode(int keyCode, bool isKeyDown);

private:
    // MIDI devices
    void setupMidiDevices();
    void closeMidiDevices();

    std::unique_ptr<juce::MidiOutput> midiOutput;
    std::unique_ptr<juce::MidiInput> midiInput;

    // finetune
    bool fineTune = false;

    // fader values
    static constexpr int numFaders = 8;
    std::array<int, numFaders> faderValues;

    // Fader methods
    void nudgeFader(int faderIndex, int delta);
    void sendFaderMove(int faderIndex, int value);

    // Bank methods
    void nudgeBankLeft();
    void nudgeBankRight();

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(FaderEngine)
};
