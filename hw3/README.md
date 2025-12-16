## VU MIF ROBOTICS HOMEWORK 3
Task: "Use all components that you did not use in previous projects and available in the Arduino set"

### Problem
A door management and opening system that can work based on button input or proximity(light sensor) with changeable parameters that persist through shutdowns.

### Design
The system is managed via 2 buttons.

Pressing the Left("Main") button in the encoder will cause the system to "submit" or "enter" into a new menu and optionally save a value.
The button press changes the **Main State** below.

Pressing the Right("Side") button navigates menus or changes values.
A press changes either the **Setting State** or active value.

There are 3 state machines:

Main state:
- **Idle**: Only a dot is visible to indicate the speed of **Timing**
- **Display**: The counter is visible and shown
- **Settings**: The 7 item settings menu to operate the device
- **ChangingValue**: Used in Brightness, Timing, Speed, Clock and Automate settings pages when the value is actively being edited

Setting state:
- **Timing**: If pressed "OK", it will allow the user to select one of 9 different timer frequencies.
  Press "OK" again to apply
- **Speed**: If pressed "OK", it will allow the user to select one of 7 different brightness options.
  Press "OK" again to apply
- **Automate**: If pressed "OK", it will allow the user to choose between proximity-based detection ("YES") or manual button control ("NO").
  Press "OK" again to apply
- **Nothing**: If pressed "OK", it will cause Brightness, Timing, Speed, Clock and Automate settings to reset to defaults.
- **Reset**: If pressed "OK", it will go back to viewing the clock

Door state:
- **Closed**: The door is currently closed
- **Opening**: The door is switching from Closed to Open
- **Open**: The door is currently closed
- **Closing**: The door is switching from Open to Closed

The settings are saved in EEPROM, so they persist over shutdowns.

Both buttons can exit out of Idle mode.

If not in **Settings** or **Changing Value** states AND **Automate** is set to "NO", then the Right("Side") button is used to control the door.
Pushing it down opens the door, it stays open until the user lets go and enough time (based on **Timing**) has passed since the last push to open the door.

### Parts List
| Component | Qty |
| --- | --- |
| Arduino Uno R3 | 1 |
| Pushbutton | 2 |
| Cathode 7 Segment Display | 1 |
| 1 kΩ Resistor | 11 |
| Photoresistor | 1 |
| 250 kΩ Potentiometer | 1 |
| 8-Bit Shift Register | 1 |
| Positional Micro Servo | 1 |

### Photo



### Video Demo



### Review

**Features**:
* Working asynchonous event-based code
* Persistent settings even after a shutdown of the device
* CRC, Magic and version number checks on the user settings to ensure they are not corrupted
* Detection of button debouncing

**Possible improvements**:
* Navigation or value adjustment could be done with a more natural directional input than a one-way pushbutton
