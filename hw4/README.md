## VU MIF ROBOTICS HOMEWORK 4
Task: "The most complex project"

### Problem
A door management and opening system that can work based on normal work time or proximity with changeable parameters that persist through shutdowns.

### Design
The system is managed via the Rotary Encoder.

Pressing the button in the encoder will cause the system to "submit" or "enter" into a new menu and optionally save a value.
The button press changes the **Main State** below.

Turning the dial navigates menus or changes values.
A turn left or right changes either the **Setting State** or active value.

There are 3 state machines:

Main state:
- **Display**: The counter is visible and shown
- **Settings**: The 7 item settings menu to operate the device
- **ChangingValue**: Used in Brightness, Timing, Speed, Clock and Automate settings pages when the value is actively being edited

Setting state:
- **Brightness**: If pressed "OK", it will allow the user to select one of 7 different brightness options.
  Press "OK" again to apply
- **Timing**: If pressed "OK", it will allow the user to select one of 9 different timer frequencies.
  Press "OK" again to apply
- **Speed**: If pressed "OK", it will allow the user to select one of 7 different brightness options.
  Press "OK" again to apply
- **Clock**: If pressed "OK", it will allow the user to set a new hour. Pressing "OK" again will allow the user to set a new minute.
  Press "OK" again to apply
- **Automate**: If pressed "OK", it will allow the user to choose between clock-based automation ("YES") or manual proximity-based detection ("NO").
  Press "OK" again to apply
- **Back**: If pressed "OK", it will cause Brightness, Timing, Speed, Clock and Automate settings to reset to defaults.
- **Reset**: If pressed "OK", it will go back to viewing the clock

Door state:
- **Closed**: The door is currently closed
- **Opening**: The door is switching from Closed to Open
- **Open**: The door is currently closed
- **Closing**: The door is switching from Open to Closed

The settings are saved in EEPROM, so they persist over shutdowns.
__Note__: Clock is not reset as it is not kept in EEPROM to prevent constant writes to it

### Parts List
| Component | Qty |
| --- | --- |
| Arduino Uno R3 | 1 |
| EC11 Rotary Encoder | 1 |
| 1.8 TFT 128 x 160 Color Display | 1 |
| APDS9960 Proximity, Light, RGB, and Gesture Sensor | 1 |
| Positional Micro Servo | 1 |

### Photo



### Video Demo



### Review

**Features**:
* Working asynchonous event-based code
* Persistent settings even after a shutdown of the device
* CRC, Magic and version number checks on the user settings to ensure they are not corrupted
* Detection of button debouncing
* Optimised screen refreshing on dials and certain text elements
* Clean menu design

**Possible improvements**:
* Change the microcontroller to a more powerful one, as UNO is too weak for features like animations on navigating menus or changing values
* Change the microcontroller to one that has more programmable memory as door status updates of it opening and closing exceeds the 32256 bytes UNO comes with
* Find a way to store a multicolored buffer that wouldn't exceed memory limitations to reduce the flicker caused by the options menu being navigated
