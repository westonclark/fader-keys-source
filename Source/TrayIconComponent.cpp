#include "TrayIconComponent.h"
#include "BinaryData.h"

TrayIconComponent::TrayIconComponent(FaderEngine &engineToControl)
    : engineRef(engineToControl)
{
    auto icon = juce::ImageCache::getFromMemory(BinaryData::fadersicon_png, BinaryData::fadersicon_pngSize);
    if (icon.isValid())
    {
        setIconImage(icon, icon);
    }
    else
    {
        DBG("TrayIconComponent: Could not load SPACECAT from BinaryData!");
    }
}

void TrayIconComponent::mouseDown(const juce::MouseEvent &e)
{
    if (e.mods.isLeftButtonDown())
    {
        juce::PopupMenu m;
        const bool isCurrentlyFine = engineRef.getFineTune();

        m.addItem("Toggle Fine Tune (currently " + juce::String(isCurrentlyFine ? "ON" : "OFF") + ")",
                  [this, isCurrentlyFine]
                  {
                      engineRef.setFineTune(!isCurrentlyFine);
                  });

        m.showMenuAsync(juce::PopupMenu::Options());
    }
}
