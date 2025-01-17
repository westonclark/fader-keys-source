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
- (void)toggleFineTune:(id)sender;
@end

@implementation StatusItemHandler

- (instancetype)initWithEngine:(FaderEngine*)eng
{
    self = [super init];
    if (self != nil)
        engine = eng;
    return self;
}

- (void)toggleFineTune:(id)sender
{
    if (engine != nullptr)
    {
        bool current = engine->getFineTune();
        engine->setFineTune(!current);
        TrayIconMac::updateMenuTitle(!current);
    }
}

@end

namespace TrayIconMac
{
    // We keep a static reference to our NSStatusItem and the handler:
    static NSStatusItem* statusItem = nil;
    static StatusItemHandler* itemHandler = nil;
    static NSMenuItem* toggleItem = nil;

    void createStatusBarIcon (FaderEngine* engine)
    {
        if (statusItem != nil) return;

        // Create the status item (with variable length)
        statusItem = [[NSStatusBar systemStatusBar]
                        statusItemWithLength:NSVariableStatusItemLength];

        NSImage* icon = nil;
        if (auto* imageData = BinaryData::fadersicon_png)
        {
            NSData* data = [NSData dataWithBytes:imageData length:BinaryData::fadersicon_pngSize];
            icon = [[NSImage alloc] initWithData:data];
            if (icon != nil)
            {
                [icon setTemplate:YES];
                [icon setSize:NSMakeSize(18, 18)];
                [statusItem setImage:icon];
            }
        }

        // Create the handler object
        itemHandler = [[StatusItemHandler alloc] initWithEngine:engine];

        // Build an NSMenu
        NSMenu* menu = [[NSMenu alloc] initWithTitle:@"FadersMenu"];

        // The single toggle item:
        bool isFine = (engine != nullptr) ? engine->getFineTune() : false;
        toggleItem = [[NSMenuItem alloc] initWithTitle:@"Fine Tune"
                                              action:@selector(toggleFineTune:)
                                       keyEquivalent:@""];
        [toggleItem setTarget:itemHandler];
        [toggleItem setState:(isFine ? NSOnState : NSOffState)];
        [menu addItem:toggleItem];

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
        toggleItem = nil;
    }

    void updateMenuTitle (bool fineTuneEnabled)
    {
        if (toggleItem != nil)
        {
            [toggleItem setState:(fineTuneEnabled ? NSOnState : NSOffState)];
        }
    }
}

#endif
