#pragma once

#include <JuceHeader.h>
#include <array>

/**
 * FaderEngine handles all MIDI communication and fader control logic.
 * It implements the HUI protocol for communicating with DAWs and manages
 * the state of 8 virtual faders.
 */
class FaderEngine : public juce::MidiInputCallback
{
public:
    enum class NudgeSensitivity
    {
        Low,
        Medium,
        High
    };

    FaderEngine();
    ~FaderEngine() override;

    void handleIncomingMidiMessage(juce::MidiInput *source, const juce::MidiMessage &message) override;

    // Called by the tray icon menu to change sensitivity
    NudgeSensitivity getNudgeSensitivity() const { return sensitivity; }
    void setNudgeSensitivity(NudgeSensitivity newSensitivity) { sensitivity = newSensitivity; }

    void handleGlobalKeycode(int keyCode, bool isKeyDown, bool isShiftDown);

private:
    /** Creates MIDI messages for fader movement following HUI protocol */
    std::vector<juce::MidiMessage> createFaderMoveMessages(int faderIndex, int value) const;

    /** Creates pitch wheel message for Logic Pro compatibility */
    juce::MidiMessage createPitchWheelMessage(int faderIndex, int value) const;

    /** Handles bank switching commands */
    bool handleBankSwitching(int keyCode, bool isShiftDown);

    // MIDI setup devices
    void setupMidiDevices();
    void closeMidiDevices();

    std::unique_ptr<juce::MidiOutput> midiOutput;
    std::unique_ptr<juce::MidiInput> midiInput;

    // fader values
    static constexpr int numFaders = 8;
    std::array<int, numFaders> faderValues{};

    // Fader nudge methods
    void nudgeFader(int faderIndex, int delta);

    // Bank methods
    void nudgeBankLeft();
    void nudgeBankRight();
    void nudgeBankLeft8();
    void nudgeBankRight8();

    NudgeSensitivity sensitivity = NudgeSensitivity::Medium;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(FaderEngine)
};
