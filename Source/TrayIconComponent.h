#pragma once

#include <JuceHeader.h>
#include "FaderEngine.h"

/**
    A tray icon that appears in the system tray / mac menu bar.
    Left-click -> shows a PopupMenu with one toggle item for "fine tune."
*/
class TrayIconComponent : public juce::SystemTrayIconComponent
{
public:
    TrayIconComponent(FaderEngine &engineToControl);
    ~TrayIconComponent() override = default;

    void mouseDown(const juce::MouseEvent &) override;

private:
    FaderEngine &engineRef;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(TrayIconComponent)
};
