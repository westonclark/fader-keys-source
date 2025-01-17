/*
  ==============================================================================

    GlobalKeyListener.h
    Created: 9 Jan 2025 3:30:13pm
    Author:  Weston Clark

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>

// Forward declare fader engine class so we don't have circular includes
class FaderEngine;

// Called to start/stop capturing keys globally:
void startGlobalKeyListener(FaderEngine *engine);
void stopGlobalKeyListener();
