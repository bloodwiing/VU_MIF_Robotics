## VU MIF ROBOTICS HOMEWORK 1
Task: "Make something extraordinary"

### Problem
An automatic light system that increases indoor light brightness based on outside detected brigthness,
with manual preset options that aren't automated.

### Design
Using a photoresistor detect the outside brightness.
It acts like a pullup, where the higher the brightness - the higher the voltage passes.

Support 4 different operation modes:
- **OFF**:
  The light is off.
- **REACTIVE**:
  The light changes lightness based on the external sensor input.
  It goes brigther as the photoresistor detects dimmer light.
- **FULL**:
  The light is at high brightness regardless of any external input.
- **NIGHT**:
  A variant of the **FULL** mode that's not as glaring

These modes are toggled via a pushbutton.

Additionally the active mode is displayed via the LCD display as well as the current set brightness of the LED as a percentage.

### Parts List
| Component | Qty |
| --- | --- |
| Arduino Uno R3 | 1 |
| White LED | 1 |
| Photoresistor | 1 |
| Pushbutton | 1 |
| 2.5 kOhm Resistor | 1 |
| 10 kOhm Resistor | 2 |
| LCD 16 x 2 | 1 |
| Breadboard | (optional unless soldering) |
| Jumper cables | (optional unless soldering) |

### Photo

<img width="761" height="459" alt="2025-09-23-114756_hyprshot" src="https://github.com/user-attachments/assets/7d256627-4611-45e9-94ad-7fbc914eab67" />

### Video Demo

https://github.com/user-attachments/assets/4fc78b31-2f4e-4211-9897-624afdff7422

### Review

**Features**:
* Working sensor->light reactivity.
* Several mode support.
* The button has support for debouncing for any electrical noise.
* LCD display to inform the user of the state.

**Possible improvements**:
* **FULL** and **REACTIVE** mode should not be one of the cycle toggles, when the user is trying to navigate their home under the guide of a **NIGHT** light mode.
* LCD display brightness could be regulated with a potentiometer.
* Photoresistor sensitivity could be regulated with a potentiometer.
