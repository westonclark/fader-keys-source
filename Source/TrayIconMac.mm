#if JUCE_MAC

#import <Cocoa/Cocoa.h>
#include "TrayIconMac.h"
#include "FaderEngine.h"    // so we can call engine->setFineTune(...)

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

        // Update the menu item’s title to reflect the new state:
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
        if (statusItem != nil) return; // Already created?

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
                [icon setTemplate:YES];   // so the OS can adapt it to dark mode
                [icon setSize:NSMakeSize(18, 18)]; // Set to standard menu bar size
                [statusItem setImage:icon];
            }
        }

        // Create the handler object
        itemHandler = [[StatusItemHandler alloc] initWithEngine:engine];

        // Build an NSMenu
        NSMenu* menu = [[NSMenu alloc] initWithTitle:@"FadersMenu"];

        // Our single toggle item:
        bool isFine = (engine != nullptr) ? engine->getFineTune() : false;
        toggleItem = [[NSMenuItem alloc] initWithTitle:@"Fine Tune"
                                              action:@selector(toggleFineTune:)
                                       keyEquivalent:@""];
        [toggleItem setTarget:itemHandler];
        [toggleItem setState:(isFine ? NSOnState : NSOffState)];

        [menu addItem:toggleItem];

        // Attach the menu to the status item
        [statusItem setMenu:menu];

        // We can highlight automatically while the menu is open
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

#endif // JUCE_MAC
