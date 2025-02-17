# Fader Keys

Transform your computer keyboard into a powerful DAW controller, letting you control up to 8 faders simultaneously without external hardware.

## What is Fader Keys?

- Control multiple faders at once using just the keys on your keyboard (Q-A, W-S, etc.)
- Adjustable fader nudge sensitivity for precise control
- Supports both HUI and MCU protocols for compatibility with Pro Tools, Logic, Ableton, and many other DAWs

## Controls

<p align="center">
<img width="600" alt="keyboard-layout" src="https://github.com/user-attachments/assets/02287bdf-8e30-40b4-9442-8fb681ef0b3d">
</p>

> [!NOTE]
> Holding down the `shift` key while banking will bank in groups of 8 instead of 1

> [!NOTE]
> Holding down the `shift` key while nuding fader levels will temporarily do a large nudge

> [!NOTE]
> The menu bar icon will highlight red when Fader Keys is active, indicating that keyboard focus is being captured

<p align="center">
<img width='460' alt="Sensitivity Settings" src="https://github.com/user-attachments/assets/e9879612-2c1b-44d5-8a44-102d6e06c681" />
</p>

## Installation & Setup

### 1. Install and Authorize

- Run the installer
- Launch Fader Keys from your Applications folder
- When asked, input your serial number (found in the confirmation email or at www.faderkeys.com under `My Products`)
- Once authorized, the application will quit and ask you to relaunch

<p align="center">
<img width="500" alt="Serial Number Registration" src="https://github.com/user-attachments/assets/6902d5c2-db27-4510-ac9b-e34c60380fc8" />
</p>

### 2. Grant Accessibility Permissions

- On first launch after authorization, Fader Keys will ask for keyboard input access via System Preferences
- If the popup does show up you can give Fader Keys access under `System Settings -> Privacy & Security -> Accessibility`

## Pro Tools Setup

#### Enable MIDI Input

- Navigate to `Setup → MIDI → MIDI Input Devices`
- Enable Fader Keys in the device list

#### Configure HUI Controller

- In the menu bar, go to `Setup → Peripherals`
- Navigate to the `MIDI Controllers` tab
- Add a HUI controller
- Assign Fader Keys ports the the `Receive From` and `Send To` ports

## Logic Pro Setup

- In the menu bar, navigate to `Logic Pro → Controll Surfaces → Setup`
- Click the `New` button and select `Install` to add a new MIDI device
- Search for `Mackie Control` device and select `Add`
- Assign the `Input and Output` ports to Fader Keys

## Ableton Live Setup

- In the menu bar, navigate to `Ableton Live → Preferences` and select the `Link MIDI` tab
- Under Control Surface select `Mackie Control`
- Select the `Fader Keys MIDI` as your Input and Output

## Studio One Setup

- In the menu bar, navigate to `Studio One → Preferences` and select the `External Devices`
- Select the `Mackie/Control` device
- Select the `Fader Keys MIDI` as your `Send To` and `Receive From` ports
