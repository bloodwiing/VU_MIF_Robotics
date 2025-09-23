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

Additionally this mode is displayed via the LCD display as well as the current set brightness of the LED as a percentage.

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
