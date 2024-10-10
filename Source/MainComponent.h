#pragma once

#include <JuceHeader.h>

//==============================================================================
/*
    This component lives inside our window, and this is where you should put all
    your controls and content.
*/
class MainComponent : public juce::AudioAppComponent,
                      public juce::KeyListener,
                      public juce::MidiInputCallback,
                      public juce::MidiKeyboardStateListener
{
public:
    //==============================================================================
    MainComponent();
    ~MainComponent() override;

    //==============================================================================
    void prepareToPlay(int samplesPerBlockExpected, double sampleRate) override;
    void getNextAudioBlock(const juce::AudioSourceChannelInfo &bufferToFill) override;
    void releaseResources() override;

    //==============================================================================
    void paint(juce::Graphics &g) override;
    void resized() override;

    // KeyListener callbacks
    bool keyPressed(const juce::KeyPress &key, juce::Component *originatingComponent) override;
    bool keyStateChanged(bool isKeyDown, juce::Component *originatingComponent) override;

    // MidiInputCallback
    void handleIncomingMidiMessage(juce::MidiInput *source, const juce::MidiMessage &message) override;

    // MidiKeyboardStateListener
    void handleNoteOn(juce::MidiKeyboardState *source, int midiChannel, int midiNoteNumber, float velocity) override;
    void handleNoteOff(juce::MidiKeyboardState *source, int midiChannel, int midiNoteNumber, float velocity) override;

    // Overridden to handle visibility change
    void visibilityChanged() override;

private:
    //==============================================================================
    // Your private member variables go here...

    juce::MidiKeyboardState keyboardState;
    juce::MidiKeyboardComponent keyboardComponent;

    // MIDI devices
    std::unique_ptr<juce::MidiOutput> midiOutput;
    juce::ComboBox midiOutputSelector;

    std::unique_ptr<juce::MidiInput> midiInput;
    juce::ComboBox midiInputSelector;

    juce::Label midiOutputLabel;
    juce::Label midiInputLabel;

    void setupMidiDevices();
    void closeMidiDevices();

    // Flag to check if focus has been set
    bool hasFocusBeenSet = false;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MainComponent)
};
