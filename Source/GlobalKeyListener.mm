/*
  ==============================================================================

    GlobalKeyListener.cpp
    Created: 9 Jan 2025 3:29:40pm
    Author:  Weston Clark

  ==============================================================================
*/

#include "GlobalKeyListener.h"
#include "FaderEngine.h"
#include "TrayIconMac.h"

#import <Cocoa/Cocoa.h>

namespace
{
    // Add this function to detect if the frontmost application is a DAW you care about.
    bool isSupportedDawFocused()
    {
        auto frontmostApp = [NSWorkspace sharedWorkspace].frontmostApplication;
        if (frontmostApp == nil)
            return false;

        auto bundleID = frontmostApp.bundleIdentifier;
        if (bundleID == nil)
            return false;

        // Replace these with the actual bundle identifiers for each DAW you care about.
        static const std::unordered_set<std::string> supportedDawBundleIDs {
            "com.avid.ProTools",    // Pro Tools
            "com.apple.logic10",    // Logic Pro X
            "com.ableton.live",     // Ableton Live
        };

        return (supportedDawBundleIDs.find([bundleID UTF8String]) != supportedDawBundleIDs.end());
    }

    CFMachPortRef eventTap = nullptr;
    CFRunLoopSourceRef runLoopSource = nullptr;
    FaderEngine* globalKeyEngine = nullptr;
    std::unique_ptr<juce::Timer> retryTimer;

    // validKeyCodes to block
    const std::unordered_set<unsigned short> validKeyCodes =
    {
        0, 1, 2, 3, 5, 4, 38, 40, // a-k
        12, 13, 14, 15, 17, 16, 32, 34, // q-i
        18, 19  // 1-2
    };

    CGEventRef eventTapCallback(CGEventTapProxy proxy,
                                CGEventType type,
                                CGEventRef event,
                                void* userInfo)
    {
        if (type == kCGEventFlagsChanged)
        {
            // Read raw flags from the CGEventRef
            static CGEventFlags lastFlags = 0;
            CGEventFlags currentFlags = CGEventGetFlags(event);

            // Check if Caps Lock was toggled
            if ((currentFlags & kCGEventFlagMaskAlphaShift) != (lastFlags & kCGEventFlagMaskAlphaShift))
            {
                bool isCapsLockOn = ((currentFlags & kCGEventFlagMaskAlphaShift) != 0);
                TrayIconMac::updateCapsLockState(isCapsLockOn);
            }
            lastFlags = currentFlags;
        }
        else if ((type == kCGEventKeyDown || type == kCGEventKeyUp) && globalKeyEngine != nullptr)
        {
            NSEvent* nsEvent = [NSEvent eventWithCGEvent:event];
            if (nsEvent != nil)
            {
                unsigned short keyCode = [nsEvent keyCode];
                bool isKeyDown = (type == kCGEventKeyDown);
                bool isShiftDown = (CGEventGetFlags(event) & kCGEventFlagMaskShift) != 0;

                // Check Caps Lock state
                CGEventFlags flags = CGEventGetFlags(event);
                bool isCapsLockOn = ((flags & kCGEventFlagMaskAlphaShift) != 0);
                TrayIconMac::updateCapsLockState(isCapsLockOn);

                // Only swallow keystroke if:
                //    1) Caps-Lock is ON
                //    2) The frontmost application is one of the DAWs
                //    3) keyCode is in 'validKeyCodes'
                if (isCapsLockOn
                    && isSupportedDawFocused()
                    && (validKeyCodes.find(keyCode) != validKeyCodes.end()))
                {
                    juce::MessageManager::callAsync([=]()
                    {
                        if (globalKeyEngine != nullptr)
                            globalKeyEngine->handleGlobalKeycode((int)keyCode, isKeyDown, isShiftDown);
                    });
                    return nullptr;  // Swallow event
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
            CGEventMask eventMask = (1 << kCGEventKeyDown) | (1 << kCGEventKeyUp) | (1 << kCGEventFlagsChanged);
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

    CGEventMask eventMask = (1 << kCGEventKeyDown) | (1 << kCGEventKeyUp) | (1 << kCGEventFlagsChanged);
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
