#if JUCE_MAC

#import <Cocoa/Cocoa.h>
#include "TrayIconMac.h"
#include "FaderEngine.h"

// A simple Objective‐C helper “handler” that can forward clicks to C++:
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
    // We keep a static reference to our NSStatusItem and the handler:
    static NSStatusItem* statusItem = nil;
    static StatusItemHandler* itemHandler = nil;
    static NSMenuItem* lowItem = nil;
    static NSMenuItem* mediumItem = nil;
    static NSMenuItem* highItem = nil;

    void createStatusBarIcon (FaderEngine* engine)
    {
        if (statusItem != nil) return;

        // Create the status item (with variable length)
        statusItem = [[NSStatusBar systemStatusBar]
                        statusItemWithLength:NSVariableStatusItemLength];

        NSImage* icon = nil;
        if (auto* imageData = BinaryData::fadersiconsmall_png)
        {
            NSData* data = [NSData dataWithBytes:imageData length:BinaryData::fadersiconsmall_pngSize];
            icon = [[NSImage alloc] initWithData:data];
            if (icon != nil)
            {
                [icon setTemplate:YES];
                [icon setSize:NSMakeSize(18, 18)];
                [statusItem setImage:icon];
            }
        }

        // Handler object
        itemHandler = [[StatusItemHandler alloc] initWithEngine:engine];

        // Menu
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

        // Initial state
        updateSensitivityMenu(engine->getNudgeSensitivity());

        [statusItem setMenu:menu];
        [statusItem setHighlightMode:YES];
    }

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
    }

    void updateSensitivityMenu(FaderEngine::NudgeSensitivity sensitivity)
    {
        if (lowItem && mediumItem && highItem)
        {
            [lowItem setState:(sensitivity == FaderEngine::NudgeSensitivity::Low ? NSOnState : NSOffState)];
            [mediumItem setState:(sensitivity == FaderEngine::NudgeSensitivity::Medium ? NSOnState : NSOffState)];
            [highItem setState:(sensitivity == FaderEngine::NudgeSensitivity::High ? NSOnState : NSOffState)];
        }
    }
}

#endif
