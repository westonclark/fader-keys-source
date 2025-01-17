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

                if (isCapsLockOn)
                {
                    // Valid keycodes
                    static const std::unordered_set<unsigned short> validKeyCodes = {
                        0, 1, 2, 3, 5, 4, 38, 40,  // a-k
                        12, 13, 14, 15, 17, 16, 32, 34,  // q-i
                        18, 19  // 1-2
                    };

                    if (validKeyCodes.find(keyCode) != validKeyCodes.end())
                    {
                        juce::MessageManager::callAsync([=]()
                        {
                            if (globalKeyEngine != nullptr)
                                globalKeyEngine->handleGlobalKeycode((int)keyCode, isKeyDown);
                        });
                        return nullptr;  // Swallow event
                    }
                }
            }
        }
        // If not swallowed, return event to let the OS proceed
        return event;
    }

// Keep retrying to create the event tap until user grants permission
    class EventTapRetryTimer : public juce::Timer
    {
    public:
        EventTapRetryTimer() { startTimer(1000); }

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
                eventTap = newEventTap;
                runLoopSource = CFMachPortCreateRunLoopSource(kCFAllocatorDefault, eventTap, 0);
                CFRunLoopAddSource(CFRunLoopGetCurrent(), runLoopSource, kCFRunLoopCommonModes);
                CGEventTapEnable(eventTap, true);

                // Re-hide the dock icon after permission is granted

                // if (juce::Process::isDockIconVisible())
                // {
                //     juce::Process::setDockIconVisible(false);
                // }

                DBG("GlobalKeyListener successfully started after permission grant.");
                retryTimer.reset();
            }
        }
    };
}

void startGlobalKeyListener(FaderEngine* engine)
{
    if (eventTap != nullptr)
        return;

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
        // Start retry timer if we failed to create the event tap on first try
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
    // Cancel any pending retry timer first
    if (retryTimer != nullptr)
    {
        retryTimer->stopTimer();
        retryTimer.reset();
    }

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
