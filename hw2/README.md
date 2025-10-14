## VU MIF ROBOTICS HOMEWORK 2
Task: "Themed Task (2018 Style)"

### Problem
A counter that has an adjustable counting speed, brightness and can even go into dot only mode.

### Design
The system has 2 buttons.

One button will either enter a menu or save a setting.
The other button is used to toggle between options.

There are 2 state machines:

Main state:
- **Idle**: The number display is off, only the dot flickers
- **Time**: The counter is visible and shown
- **Settings**: The 5 item settings menu to operate the device
- **ChangingValue**: Used in Brightness and Time settings pages when the value is actively being edited

Setting state:
- **Power**: If pressed "OK", it will put device in dot only mode
- **Time**: If pressed "OK", it will allow the user to select one of 9 different timer frequencies.
  Press "OK" again to apply
- **Brightness**: If pressed "OK", it will allow the user to select one of 5 different brightness options.
  Press "OK" again to apply
- **Reset**: If pressed "OK", it will cause Time and Brightness settings to reset to defaults
- **Nothing**: If pressed "OK", it will go back to viewing the counter

The settings are saved in EEPROM, so they persist over shutdowns.

Both buttons show the counter out of Idle mode.

### Parts List
| Component | Qty |
| --- | --- |
| Arduino Uno R3 | 1 |
| 7 Segment display | 1 |
| Pushbutton | 2 |
| 1 kOhm Resistor | 3 |

### Photo

<img width="1164" height="450" alt="2025-10-14-135910_hyprshot" src="https://github.com/user-attachments/assets/691dd7da-7c28-4f50-a93e-6d010666745d" />

### Video Demo

https://github.com/user-attachments/assets/cdfb30e9-ecf3-4c1e-87d4-a34cc3c0045d

### Review

**Features**:
* Working asynchonous event-based code
* Persistent settings even after a shutdown of the device
* CRC, Magic and version number checks on the user settings to ensure they are not corrupted
* Detection of debouncing

**Possible improvements**:
* Navigation or value adjustment could be done with a more natural directional input than a one-way pushbutton

