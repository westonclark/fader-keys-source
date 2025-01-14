#pragma once

#include <JuceHeader.h>
#include <array>
#include <map>

//==============================================================================
/*
    This component lives inside our window, and this is where you should put all
    your controls and content.
*/
class MainComponent : public juce::Component,
                      public juce::MidiInputCallback,
                      public juce::Slider::Listener
{
public:
    //==============================================================================
    MainComponent();
    ~MainComponent() override;

    //==============================================================================
    void paint(juce::Graphics &g) override;
    void resized() override;

    // MidiInputCallback
    void handleIncomingMidiMessage(juce::MidiInput *source, const juce::MidiMessage &message) override;

    // Slider Listener
    void sliderValueChanged(juce::Slider *slider) override;

    // Global key listener
    void handleGlobalKeycode(int keyCode, bool isKeyDown);

private:
    //==============================================================================
    // MIDI devices
    std::unique_ptr<juce::MidiOutput> midiOutput;
    std::unique_ptr<juce::MidiInput> midiInput;

    void setupMidiDevices();
    void closeMidiDevices();

    // Virtual faders
    static constexpr int numFaders = 8;
    std::array<juce::Slider, numFaders> faders;
    std::array<juce::Label, numFaders> faderLabels;
    std::array<int, numFaders> faderValues; // Stores 14-bit values (0 - 16383)

    // Key mappings
    std::map<int, int> keyToFaderIndexUp;   // Key code to fader index for nudging up
    std::map<int, int> keyToFaderIndexDown; // Key code to fader index for nudging down

    // Fader methods
    void initializeFaders();
    void sendFaderMove(int faderIndex, int value);
    void nudgeFader(int faderIndex, int delta);

    // Fine tune methods
    bool fineTune = false;
    juce::ToggleButton fineTuneButton;

    // Bank methods
    void nudgeBankLeft();
    void nudgeBankRight();

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MainComponent)
};
