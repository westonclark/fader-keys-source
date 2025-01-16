/*
  ==============================================================================

    GlobalKeyListener.cpp
    Created: 9 Jan 2025 3:29:40pm
    Author:  Weston Clark

  ==============================================================================
*/

#include "GlobalKeyListener.h"
#include "FaderEngine.h"

#import <Cocoa/Cocoa.h>
#import <Carbon/Carbon.h>

namespace
{
    CFMachPortRef eventTap = nullptr;
    CFRunLoopSourceRef runLoopSource = nullptr;
    FaderEngine* globalKeyEngine = nullptr;
    std::unique_ptr<juce::Timer> retryTimer;

    CGEventRef eventTapCallback(CGEventTapProxy proxy,
                                CGEventType type,
                                CGEventRef event,
                                void* userInfo)
    {
        if ((type == kCGEventKeyDown || type == kCGEventKeyUp) && globalKeyEngine != nullptr)
        {
            NSEvent* nsEvent = [NSEvent eventWithCGEvent:event];
            if (nsEvent != nil)
            {
                unsigned short keyCode = [nsEvent keyCode];
                bool isKeyDown = (type == kCGEventKeyDown);

                // Get caps lock state
                NSUInteger flags = [NSEvent modifierFlags];
                bool isCapsLockOn = ((flags & NSEventModifierFlagCapsLock) != 0);

                // Only handle these key events if Caps Lock is on
                if (isCapsLockOn)
                {
                    // Forward to our FaderEngine
                    // The engine’s handleGlobalKeycode(int keyCode, bool isKeyDown) is a normal C++ method
                    juce::MessageManager::callAsync([=]()
                    {
                        // Just call the engine function directly:
                        globalKeyEngine->handleGlobalKeycode((int)keyCode, isKeyDown);
                    });

                    // If the keyCode is one we want to “swallow” (nudge keys, etc.), return nullptr
                    // so the OS doesn't see it.
                    switch (keyCode)
                    {
                        case 0:  // 'a'
                        case 1:  // 's'
                        case 2:  // 'd'
                        case 3:  // 'f'
                        case 5:  // 'g'
                        case 4:  // 'h'
                        case 38: // 'j'
                        case 40: // 'k'

                        case 12: // 'q'
                        case 13: // 'w'
                        case 14: // 'e'
                        case 15: // 'r'
                        case 17: // 't'
                        case 16: // 'y'
                        case 32: // 'u'
                        case 34: // 'i'

                        case 18: // '1'
                        case 19: // '2'
                            // Swallow the event
                            return nullptr;

                        default:
                            break;
                    }
                }
            }
        }
        // If not swallowed, return event to let the OS proceed
        return event;
    }

    class EventTapRetryTimer : public juce::Timer
    {
    public:
        EventTapRetryTimer() { startTimer(1000); } // Check every second

        void timerCallback() override
        {
            CGEventMask eventMask = (1 << kCGEventKeyDown) | (1 << kCGEventKeyUp);
            auto newEventTap = CGEventTapCreate(kCGSessionEventTap,
                                                kCGHeadInsertEventTap,
                                                kCGEventTapOptionDefault,
                                                eventMask,
                                                eventTapCallback,
                                                nullptr);

            if (newEventTap)
            {
                // Success! Set up the event tap
                eventTap = newEventTap;
                runLoopSource = CFMachPortCreateRunLoopSource(kCFAllocatorDefault, eventTap, 0);
                CFRunLoopAddSource(CFRunLoopGetCurrent(), runLoopSource, kCFRunLoopCommonModes);
                CGEventTapEnable(eventTap, true);

                DBG("GlobalKeyListener successfully started after permission grant.");
                retryTimer.reset(); // Stop and delete
            }
        }
    };
}

void startGlobalKeyListener(FaderEngine* engine)
{
    if (eventTap != nullptr)
        return;  // already created

    globalKeyEngine = engine;

    CGEventMask eventMask = (1 << kCGEventKeyDown) | (1 << kCGEventKeyUp);
    eventTap = CGEventTapCreate(kCGSessionEventTap,
                                kCGHeadInsertEventTap,
                                kCGEventTapOptionDefault,
                                eventMask,
                                eventTapCallback,
                                nullptr);

    if (!eventTap)
    {
        DBG("Failed to create event tap! Check Accessibility Permissions.");
        // Start retry timer
        retryTimer = std::make_unique<EventTapRetryTimer>();
        return;
    }

    runLoopSource = CFMachPortCreateRunLoopSource(kCFAllocatorDefault, eventTap, 0);
    CFRunLoopAddSource(CFRunLoopGetCurrent(), runLoopSource, kCFRunLoopCommonModes);
    CGEventTapEnable(eventTap, true);

    DBG("GlobalKeyListener started.");
}

void stopGlobalKeyListener()
{
    if (eventTap != nullptr)
    {
        CGEventTapEnable(eventTap, false);
        CFRunLoopRemoveSource(CFRunLoopGetCurrent(), runLoopSource, kCFRunLoopCommonModes);
        CFRelease(runLoopSource);
        runLoopSource = nullptr;
        CFRelease(eventTap);
        eventTap = nullptr;

        DBG("GlobalKeyListener stopped.");
    }

    globalKeyEngine = nullptr;
}
