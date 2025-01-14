/*
  ==============================================================================

    GlobalKeyListener.cpp
    Created: 9 Jan 2025 3:29:40pm
    Author:  Weston Clark

  ==============================================================================
*/

#include "GlobalKeyListener.h"
#include "MainComponent.h"

#import <Cocoa/Cocoa.h>
#import <Carbon/Carbon.h>

namespace
{
    CFMachPortRef eventTap = nullptr;
    CFRunLoopSourceRef runLoopSource = nullptr;
    juce::WeakReference<juce::Component> globalKeyTarget;

    CGEventRef eventTapCallback(CGEventTapProxy proxy,
                                CGEventType type,
                                CGEventRef event,
                                void* userInfo)
    {
        if (type == kCGEventKeyDown || type == kCGEventKeyUp)
        {
            NSEvent* nsEvent = [NSEvent eventWithCGEvent:event];
            if (nsEvent != nil)
            {
                unsigned short keyCode = [nsEvent keyCode];
                bool isKeyDown = (type == kCGEventKeyDown);

                // Get caps lock state
                NSUInteger flags = [NSEvent modifierFlags];
                bool isCapsLockOn = (flags & NSEventModifierFlagCapsLock) != 0;

                if (isCapsLockOn)
                {
                    if (auto* targetComp = globalKeyTarget.get())
                    {
                        // Post to the JUCE message thread only when caps lock is on
                        juce::MessageManager::callAsync([=]()
                        {
                            if (auto mc = dynamic_cast<MainComponent*>(targetComp))
                                mc->handleGlobalKeycode((int)keyCode, isKeyDown);
                        });
                    }

                    if (keyCode == 0  || // 'a'
                        keyCode == 1  || // 's'
                        keyCode == 2  || // 'd'
                        keyCode == 3  || // 'f'
                        keyCode == 5  || // 'g'
                        keyCode == 4  || // 'h'
                        keyCode == 38  || // 'j'
                        keyCode == 40  || // 'k'

                        keyCode == 12 || // 'q'
                        keyCode == 13 || // 'w'
                        keyCode == 14 || // 'e'
                        keyCode == 15 || // 'r'
                        keyCode == 17 || // 't'
                        keyCode == 16 || // 'y'
                        keyCode == 32 || // 'u'
                        keyCode == 34 || // 'i'

                        keyCode == 18 || // '1'
                        keyCode == 19)   // '2'
                    {
                        // Return nullptr to swallow the event only when caps lock is on
                        return nullptr;
                    }
                }
            }
        }

        // Returning 'event' lets the OS proceed normally:
        return event;
    }
}

void startGlobalKeyListener(juce::Component* target)
{
    // Don’t start if we already have a tap
    if (eventTap != nullptr)
        return;

    globalKeyTarget = target;

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
}
