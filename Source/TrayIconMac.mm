#if JUCE_MAC

#import <Cocoa/Cocoa.h>
#include "TrayIconMac.h"
#include "FaderEngine.h"

// Forward declaration so we can update the icon after creation
namespace TrayIconMac
{
    void updateCapsLockState (bool capsLockOn);
}

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
    }
}

- (void)setMediumSensitivity:(id)sender
{
    if (engine != nullptr)
    {
        engine->setNudgeSensitivity(FaderEngine::NudgeSensitivity::Medium);
        ::TrayIconMac::updateSensitivityMenu(FaderEngine::NudgeSensitivity::Medium);
    }
}

- (void)setHighSensitivity:(id)sender
{
    if (engine != nullptr)
    {
        engine->setNudgeSensitivity(FaderEngine::NudgeSensitivity::High);
        ::TrayIconMac::updateSensitivityMenu(FaderEngine::NudgeSensitivity::High);
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
    // We keep a static reference to our NSStatusItem, the handler, and menu items:
    static NSStatusItem* statusItem = nil;
    static StatusItemHandler* itemHandler = nil;
    static NSMenuItem* lowItem = nil;
    static NSMenuItem* mediumItem = nil;
    static NSMenuItem* highItem = nil;

    // Store pointers to both the normal and highlighted versions of your icon
    static NSImage* normalIcon = nil;
    static NSImage* highlightedIcon = nil;

    // Creates the NSStatusItem, attaches a native macOS menu.
    void createStatusBarIcon (FaderEngine* engine)
    {
        if (statusItem != nil)
            return;

        // Create the status item (with variable length)
        statusItem = [[NSStatusBar systemStatusBar]
                        statusItemWithLength:NSVariableStatusItemLength];

        // Load the normal icon
        if (auto* imageData = BinaryData::fadersiconsmall_png)
        {
            NSData* data = [NSData dataWithBytes:imageData length:BinaryData::fadersiconsmall_pngSize];
            normalIcon = [[NSImage alloc] initWithData:data];

            if (normalIcon != nil)
            {
                // Don't set as template if you want to preserve colors
                [normalIcon setTemplate:NO];
                [normalIcon setSize:NSMakeSize(24, 24)];
                [statusItem setImage:normalIcon];
            }
        }

        // Load the highlighted icon
        if (auto* highlightData = BinaryData::fadersiconsmallhighlighted_png)
        {
            NSData* data = [NSData dataWithBytes:highlightData length:BinaryData::fadersiconsmallhighlighted_pngSize];
            highlightedIcon = [[NSImage alloc] initWithData:data];

            if (highlightedIcon != nil)
            {
                // Don't set as template for the highlighted version either
                [highlightedIcon setTemplate:NO];
                [highlightedIcon setSize:NSMakeSize(24, 24)];
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
        [lowItem setState:NSOffState];
        [mediumItem setState:NSOnState];  // Set medium as checked by default
        [highItem setState:NSOffState];

        [menu addItem:lowItem];
        [menu addItem:mediumItem];
        [menu addItem:highItem];

        // Separator
        [menu addItem:[NSMenuItem separatorItem]];

        // Quit item
        NSMenuItem* quitItem = [[NSMenuItem alloc] initWithTitle:@"Quit Fader Keys"
                                                          action:@selector(quitApp:)
                                                   keyEquivalent:@""];
        [quitItem setTarget:itemHandler];
        [menu addItem:quitItem];

        // Attach menu and set highlight mode
        [statusItem setMenu:menu];
        [statusItem setHighlightMode:YES];
    }

    // Destroys the status item (call at shutdown).
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
                [lowItem setState:(sensitivity == FaderEngine::NudgeSensitivity::Low ? NSOnState : NSOffState)];
                [mediumItem setState:(sensitivity == FaderEngine::NudgeSensitivity::Medium ? NSOnState : NSOffState)];
                [highItem setState:(sensitivity == FaderEngine::NudgeSensitivity::High ? NSOnState : NSOffState)];
                lastSensitivity = sensitivity;
            }
        }
    }

    // Call this method whenever you detect Caps Lock turning on/off
    void updateCapsLockState (bool capsLockOn)
    {
        if (statusItem == nil)
        {
            return;
        }

        if (capsLockOn)
        {
            // Show the "highlighted" icon
            if (highlightedIcon != nil)
            {
                [statusItem setImage: highlightedIcon];
            }
        }
        else
        {
            // Show the "normal" icon
            if (normalIcon != nil)
            {
                [statusItem setImage: normalIcon];
            }
        }
    }
}

#endif
