#if JUCE_MAC

#import <Cocoa/Cocoa.h>
#include "TrayIconMac.h"
#include "FaderEngine.h"
#include "Main.h"

// A simple Objective‐C helper "handler" that can forward clicks to C++:
@interface StatusItemHandler : NSObject
{
@private
    FaderEngine* engine;
}
- (instancetype)initWithEngine:(FaderEngine*)eng;
- (void)setLowSensitivity:(id)sender;
- (void)setMediumSensitivity:(id)sender;
- (void)setHighSensitivity:(id)sender;
- (void)quitApp:(id)sender;
@end

@implementation StatusItemHandler

- (instancetype)initWithEngine:(FaderEngine*)eng
{
    self = [super init];
    if (self != nil)
        engine = eng;
    return self;
}

- (void)setLowSensitivity:(id)sender
{
    if (engine != nullptr)
    {
        engine->setNudgeSensitivity(FaderEngine::NudgeSensitivity::Low);
        ::TrayIconMac::updateSensitivityMenu(FaderEngine::NudgeSensitivity::Low);

        // Persist setting in properties file
        if (auto* app = dynamic_cast<FaderKeysApplication*>(juce::JUCEApplication::getInstance()))
        {
            auto& props = app->getAppProperties();
            auto* userSettings = props.getUserSettings();
            userSettings->setValue("nudgeSensitivity", (int) FaderEngine::NudgeSensitivity::Low);
            userSettings->saveIfNeeded();
        }
    }
}

- (void)setMediumSensitivity:(id)sender
{
    if (engine != nullptr)
    {
        engine->setNudgeSensitivity(FaderEngine::NudgeSensitivity::Medium);
        ::TrayIconMac::updateSensitivityMenu(FaderEngine::NudgeSensitivity::Medium);

        // Persist setting in properties file
        if (auto* app = dynamic_cast<FaderKeysApplication*>(juce::JUCEApplication::getInstance()))
        {
            auto& props = app->getAppProperties();
            auto* userSettings = props.getUserSettings();
            userSettings->setValue("nudgeSensitivity", (int) FaderEngine::NudgeSensitivity::Medium);
            userSettings->saveIfNeeded();
        }
    }
}

- (void)setHighSensitivity:(id)sender
{
    if (engine != nullptr)
    {
        engine->setNudgeSensitivity(FaderEngine::NudgeSensitivity::High);
        ::TrayIconMac::updateSensitivityMenu(FaderEngine::NudgeSensitivity::High);

        // Persist setting in properties file
        if (auto* app = dynamic_cast<FaderKeysApplication*>(juce::JUCEApplication::getInstance()))
        {
            auto& props = app->getAppProperties();
            auto* userSettings = props.getUserSettings();
            userSettings->setValue("nudgeSensitivity", (int) FaderEngine::NudgeSensitivity::High);
            userSettings->saveIfNeeded();
        }
    }
}

- (void)quitApp:(id)sender
{
    // Use JUCE's quit mechanism to ensure a clean exit
    juce::JUCEApplication::getInstance()->systemRequestedQuit();
}

@end

namespace TrayIconMac
{
    // Static reference to our NSStatusItem, the handler, and menu items:
    static NSStatusItem* statusItem = nil;
    static StatusItemHandler* itemHandler = nil;
    static NSMenuItem* lowItem = nil;
    static NSMenuItem* mediumItem = nil;
    static NSMenuItem* highItem = nil;

    // Pointers to both the normal and highlighted versions of the icon
    static NSImage* normalIcon = nil;
    static NSImage* highlightedIcon = nil;

    // Add new static property for the button
    static NSStatusBarButton* statusButton = nil;

    // Creates the NSStatusItem, attaches a native macOS menu.
    void createStatusBarIcon (FaderEngine* engine)
    {
        if (statusItem != nil)
            return;

        // Create the status item (with variable length)
        statusItem = [[NSStatusBar systemStatusBar]
                        statusItemWithLength:NSVariableStatusItemLength];

        // Get the button from the status item
        statusButton = [statusItem button];

        if (auto* imageData = BinaryData::sliderssmall_png)
        {
            NSData* data = [NSData dataWithBytes:imageData length:BinaryData::sliderssmall_pngSize];
            normalIcon = [[NSImage alloc] initWithData:data];

            if (normalIcon != nil)
            {
                [normalIcon setTemplate:NO];  // Allows background coloring

                [normalIcon setSize:NSMakeSize(24, 24)];
                [statusButton setImage:normalIcon];
            }
        }

        // Handler object
        itemHandler = [[StatusItemHandler alloc] initWithEngine:engine];

        // Create the menu
        NSMenu* menu = [[NSMenu alloc] initWithTitle:@"FadersMenu"];

        // Title item
        NSMenuItem* titleItem = [[NSMenuItem alloc] initWithTitle:@"Nudge Sensitivity"
                                                           action:nil
                                                    keyEquivalent:@""];
        [titleItem setEnabled:NO];
        [menu addItem:titleItem];

        // Separator
        [menu addItem:[NSMenuItem separatorItem]];

        // Sensitivity options
        lowItem = [[NSMenuItem alloc] initWithTitle:@"Low"
                                             action:@selector(setLowSensitivity:)
                                      keyEquivalent:@""];
        mediumItem = [[NSMenuItem alloc] initWithTitle:@"Medium"
                                                action:@selector(setMediumSensitivity:)
                                         keyEquivalent:@""];
        highItem = [[NSMenuItem alloc] initWithTitle:@"High"
                                              action:@selector(setHighSensitivity:)
                                       keyEquivalent:@""];

        [lowItem setTarget:itemHandler];
        [mediumItem setTarget:itemHandler];
        [highItem setTarget:itemHandler];

        // Initial state - set before attaching menu
        [lowItem setState:NSControlStateValueOff];
        [mediumItem setState:NSControlStateValueOn];  // Set medium as checked by default
        [highItem setState:NSControlStateValueOff];

        [menu addItem:lowItem];
        [menu addItem:mediumItem];
        [menu addItem:highItem];

        // Separator
        [menu addItem:[NSMenuItem separatorItem]];

        // Quit item
        NSMenuItem* quitItem = [[NSMenuItem alloc] initWithTitle:@"Quit"
                                                          action:@selector(quitApp:)
                                                   keyEquivalent:@""];
        [quitItem setTarget:itemHandler];
        [menu addItem:quitItem];

        // Attach menu and set highlight mode
        [statusItem setMenu:menu];
        [[statusItem button] cell].highlighted = (NSChangeBackgroundCellMask | NSContentsCellMask);
    }

    // Destroy the status item
    void removeStatusBarIcon()
    {
        if (statusItem != nil)
        {
            [[NSStatusBar systemStatusBar] removeStatusItem:statusItem];
            statusItem = nil;
        }
        itemHandler = nil;
        lowItem = nil;
        mediumItem = nil;
        highItem = nil;
        normalIcon = nil;
        highlightedIcon = nil;
    }

    // Updates the menu title to reflect the current sensitivity state
    void updateSensitivityMenu (FaderEngine::NudgeSensitivity sensitivity)
    {
        if (lowItem && mediumItem && highItem)
        {
            // Cache the current state to avoid unnecessary updates
            static FaderEngine::NudgeSensitivity lastSensitivity =
                FaderEngine::NudgeSensitivity::Medium;

            if (sensitivity != lastSensitivity)
            {
                [lowItem setState:(sensitivity == FaderEngine::NudgeSensitivity::Low ? NSControlStateValueOn : NSControlStateValueOff)];
                [mediumItem setState:(sensitivity == FaderEngine::NudgeSensitivity::Medium ? NSControlStateValueOn : NSControlStateValueOff)];
                [highItem setState:(sensitivity == FaderEngine::NudgeSensitivity::High ? NSControlStateValueOn : NSControlStateValueOff)];
                lastSensitivity = sensitivity;
            }
        }
    }

    void updateCapsLockState(bool capsLockOn)
    {
        if (statusItem == nil || statusButton == nil)
            return;

        @try {
            if (capsLockOn)
            {
                [statusButton setContentTintColor:nil];
                statusButton.wantsLayer = YES;
                statusButton.layer.backgroundColor = [NSColor systemRedColor].CGColor;
                statusButton.layer.cornerRadius = 4;
            }
            else
            {
                if (statusButton.layer)
                {
                    statusButton.layer.backgroundColor = nil;
                    [statusButton setContentTintColor:nil];
                }
            }
        }
        @catch (NSException* exception) {
            DBG("Exception in updateCapsLockState: " << exception.reason.UTF8String);
        }
    }
}

#endif
