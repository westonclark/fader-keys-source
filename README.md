# Fader Keys

Transform your computer keyboard into a powerful DAW controller, letting you control up to 8 faders simultaneously without external hardware.

## What is Fader Keys?

- Control multiple faders at once using just the keys on your keyboard (Q-A, W-S, etc.)
- Adjustable fader nudge sensitivity for precise control
- Easily accessible On/Off toggle and bank selection keys
- Supports both HUI and MCU protocols for compatibility with Pro Tools, Logic, Ableton, and many other DAWs



## Controls

![keyboard-layout](https://github.com/user-attachments/assets/02287bdf-8e30-40b4-9442-8fb681ef0b3d)

> [!NOTE]
> Holding down the `shift` key while banking will bank in groups of 8 instead of 1

> [!NOTE]
> Holding down the `shift` key while nuding fader levels will temporarily do a large nudge

> [!NOTE]
> The menu bar icon will highlight red when `Caps Lock` is active, indicating that keyboard focus is being captured

## Installation & Setup

### 1. Install the Application
- Run the installer

### 2. Grant Accessibility Permissions
- Launch Fader Keys from your Applications folder
- When prompted, grant keyboard input access via System Preferences

<img width="827" alt="Accessibility Permissions" src="https://github.com/user-attachments/assets/15fc156d-0092-4b31-8757-6151aae2061c" />

### 3. Pro Tools Configuration

#### Enable MIDI Input
1. Navigate to `Setup → MIDI → MIDI Input Devices`
2. Enable Fader Keys in the device list

<img src="https://github.com/user-attachments/assets/8de6b837-3589-4f5b-97af-dbb4095a79be" alt="MIDI Input Setup">

#### Configure HUI Controller
1. Go to `Setup → Peripherals`
2. Add a HUI controller
3. Assign Fader Keys ports for input and output

<img src="https://github.com/user-attachments/assets/8f3c6e2e-6c1f-4249-8544-246c7885916b" alt="Peripherals Setup">

#### Fader Sensitivity
Configure fader sensitivity in the menu bar:
- Low: ~0.5dB moves
- Medium: ~1dB moves
- High: ~2dB moves

<img width="484" alt="Sensitivity Settings" src="https://github.com/user-attachments/assets/e9879612-2c1b-44d5-8a44-102d6e06c681" />
