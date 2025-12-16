/*
  VU MIF
  Informatics
  Robotics homework 3
  @author: Donatas Kirda (sid: 2212166)
*/

#include <Arduino.h>
#include <EEPROM.h>
#include <Servo.h>
#include <Adafruit_GFX.h>
#include <Adafruit_ST7735.h>
#include "dial.h"
#include "door.h"
#include "draw.h"
#include "constants.h"
#include <Arduino_APDS9960.h>

uint8_t CRC8(const uint8_t *data, int length);

#define USER_DATA_EEPORM_ADDRESS 0
#define USER_DATA_MAGIC 21
#define USER_DATA_VERSION 3

const int LCD_CS     = 10;
const int LCD_DC     = 9;
const int LCD_RST    = 8;
const int BUTTON_OK  = 2;
const int BUTTON_2   = 4;
const int BUTTON_1   = 3;
const int BRIGHTNESS_PIN = 6;
const int SERVO_PIN  = 5;

const int DEBOUNCE_TIME_MS = 20;
const int DEBOUNCE_SWITCH_TIME_MS = 60;
const int SENSOR_REACT_THRESHOLD = 50;

enum class State : uint8_t
{
  Settings,
  Displaying,
  ChangingValue
};

enum class OperationState : uint8_t
{
  Closed,
  Opening,
  Open,
  Closing
};

enum class Settings : uint8_t
{
  Reset,
  Time,
  Nothing,
  Speed,
  Automation,
  Brightness,
  Clock,
  Debug
};

const uint8_t settingTabsCount = 7;
const char* settingTabNames[] = {
  "BRIGHT", "TIMING", "SPEED", "CLOCK", "AUTO", "BACK", "RESET"
};
const Settings settingTabEnums[] = {
  Settings::Brightness, Settings::Time, Settings::Speed, Settings::Clock, Settings::Automation, Settings::Nothing, Settings::Reset
};
char settingTabValues[settingTabsCount][12];

struct UserData
{
  byte magic;
  byte version;
  short brightnessMode;
  short timeMode;
  bool automaticMode;
  short speedMode;
  uint8_t crc;
};

#define BRIGTHNESS_MODES_MAX 7
const byte brightnessModes[] = {1, 10, 20, 45, 80, 140, 255};

#define TIME_MODES_MAX 9
const uint32_t timeModes[] = {20, 50, 100, 250, 500, 750, 1000, 5000, 60000};

#define SPEED_MODES_MAX 7
const uint32_t speedModes[] = {1, 2, 5, 10, 45, 90, 180};

volatile bool buttonOKEvent = false;
volatile bool button1Event = false;
volatile bool button2State = false;
volatile bool timerEvent = false;

int32_t dialValue = 0;

uint32_t time = 0;
uint32_t lastButtonOKPress = 0;
uint32_t lastButton1Press = 0;

short brightnessMode = 4;
uint32_t brightnessValue = 80;
short timeMode = 4;
uint32_t tickTimeMs = 500;
short speedMode = 4;
uint32_t speedIncrement = 10;
bool automate = false;

uint32_t lastTick = 0;
bool dotTicked = false;
short number = 0;
bool can_count_down = false;

#define DOOR_POSITION_MAX 1023
volatile int door_position = 0;

State state = State::Displaying;
OperationState operationState = OperationState::Closed;
size_t settingsTab = 0;
UserData data;

Adafruit_ST7735 TFTscreen = Adafruit_ST7735(LCD_CS, LCD_DC, LCD_RST);

DialConfig* dial = createDialWheel(43, 47, 0x5ACB, 0x37E6, 0, 100);
DoorConfig* door = createDoor(30, 40, 0x5ACB, 0x37E6, 0, DOOR_POSITION_MAX);

uint16_t hours = 12;
uint16_t minutes = 0;
uint32_t miliseconds = 0;
const uint32_t CLOCK_TICK_LOOK_FOR = 1000;

uint16_t newHours = 0;
uint16_t newMinutes = 0;
bool changingMinutes = false;

bool screenNeedsRefresh = true;

Servo doorServo;

void setupTimer()
{
  noInterrupts();
  // just reset to normal values
  TCCR2A = 0;
  TCCR2B = 0;
  TCNT2  = 0;
  // clear timer on compare (CTC)
  TCCR2A |= (1 << WGM21);
  // prescale so instead of 16Mhz we run timer at 250Khz ticks (every 0.004ms or 4us)
  TCCR2B |= (1 << CS22);
  // comparison target
  // 0.004ms x [250 ticks] = 1ms
  // subtract -1 cause timer starts at 0 (must be lower than 65536)
  OCR2A = 250 - 1;
  // enable compare A interrupt so we can utilise the timer event
  TIMSK2 |= (1 << OCIE2A);
  interrupts();
}

void onButtonOKISR()
{
  buttonOKEvent = true;
}

void onButton1ISR()
{
  button1Event = true;
  button2State = digitalRead(BUTTON_2) == LOW;
}

ISR(TIMER2_COMPA_vect)
{
  timerEvent = true;
}

void resetSettings()
{
  brightnessMode = 4;
  timeMode = 4;
  speedMode = 3;
  automate = false;

  brightnessValue = brightnessModes[brightnessMode];
  tickTimeMs = timeModes[timeMode];
  speedIncrement = speedModes[speedMode];
}

void validateEEPROM()
{
  EEPROM.get(USER_DATA_EEPORM_ADDRESS, data);

  if (data.magic != USER_DATA_MAGIC || data.version != USER_DATA_VERSION)
  { 
    Serial.println(F("Magic or version mismatch! Resetting settings"));
    resetSettings();
    updateEEPROM();
    return;
  }

  uint8_t crc = CRC8((uint8_t*)&data, sizeof(data) - 1);
  if (data.crc != crc)
  {
    Serial.println(F("CRC mismatch! Resetting settings"));
    resetSettings();
    updateEEPROM();
    return;
  }

  Serial.println("EEPROM validated");

  brightnessMode = data.brightnessMode;
  timeMode = data.timeMode;
  speedMode = data.speedMode;
  automate = data.automaticMode;

  brightnessValue = brightnessModes[brightnessMode];
  tickTimeMs = timeModes[timeMode];
  speedIncrement = speedModes[speedMode];
}

void updateEEPROM()
{
  data = {};
  data.magic = USER_DATA_MAGIC;
  data.version = USER_DATA_VERSION;
  data.brightnessMode = brightnessMode;
  data.timeMode = timeMode;
  data.speedMode = speedMode;
  data.automaticMode = automate;

  uint8_t crc = CRC8((uint8_t*)&data, sizeof(data) - 1);
  data.crc = crc;

  Serial.println(F("Updating EEPROM"));
  EEPROM.put(USER_DATA_EEPORM_ADDRESS, data);
}

void updateEEPROMIfChanged()
{
  if (data.timeMode == timeMode
      && data.speedMode == speedMode
      && data.automaticMode == automate)
  {
    Serial.println(F("EEPROM matches, no need to update"));
    return;
  }

  updateEEPROM();
}

void setup()
{ 
  Serial.begin(9600);

  pinMode(LCD_RST, OUTPUT);
  pinMode(LCD_DC, OUTPUT);
  pinMode(LCD_CS, OUTPUT);
  pinMode(BRIGHTNESS_PIN, OUTPUT);

  pinMode(BUTTON_1, INPUT);
  pinMode(BUTTON_2, INPUT);
  pinMode(BUTTON_OK, INPUT);

  attachInterrupt(digitalPinToInterrupt(BUTTON_OK), onButtonOKISR, FALLING);
  attachInterrupt(digitalPinToInterrupt(BUTTON_1), onButton1ISR, FALLING);

  setupTimer();

  TFTscreen.initR(INITR_BLACKTAB);  // Init ST7735S chip, black tab
  TFTscreen.setRotation(1);

  TFTscreen.setFont();
  TFTscreen.fillScreen(0);
  TFTscreen.setTextSize(1);

  doorServo.attach(SERVO_PIN);

  validateEEPROM();

  if (!APDS.begin()) {
    Serial.println("Error initializing APDS-9960 sensor!");
  }

  if (automate) {
    if (hours >= 8 && hours <= 18 && operationState == OperationState::Closed) {
      operationState = OperationState::Opening;
      screenNeedsRefresh = true;
    }
    else if (!(hours >= 8 && hours <= 18) && operationState == OperationState::Open) {
      operationState = OperationState::Closing;
      screenNeedsRefresh = true;
    }
  }
}

void updateOptionsMenuValues()
{
  // "BRIGHT", "TIMING", "SPEED", "CLOCK", "AUTO", "BACK", "RESET"
  itoa(brightnessMode, settingTabValues[0], 10);
  itoa(timeMode, settingTabValues[1], 10);
  itoa(speedMode, settingTabValues[2], 10);

  if (hours < 10) {
    settingTabValues[3][0] = '0';
    itoa(hours, settingTabValues[3]+1, 10);
  } else {
    itoa(hours, settingTabValues[3], 10);
  }
  settingTabValues[3][2] = ':';
  if (minutes < 10) {
    settingTabValues[3][3] = '0';
    itoa(minutes, settingTabValues[3]+4, 10);
  } else {
    itoa(minutes, settingTabValues[3]+3, 10);
  }

  strncpy(settingTabValues[4], automate ? "YES" : "NO", sizeof(settingTabValues[4]));
}

void loop()
{
  analogWrite(BRIGHTNESS_PIN, 125);
  if (timerEvent)
  {
    timerEvent = false;
    time++;

    if (APDS.proximityAvailable()) {
      int sensor_val = APDS.readProximity();
      if (!automate && sensor_val <= SENSOR_REACT_THRESHOLD && operationState == OperationState::Closed)
      {
        operationState = OperationState::Opening;
        screenNeedsRefresh = true;
      }
    }

    if (operationState == OperationState::Opening)
    {
      door_position += speedIncrement;
      if (door_position >= DOOR_POSITION_MAX)
      {
        door_position = DOOR_POSITION_MAX;
        operationState = OperationState::Open;
        screenNeedsRefresh = true;
        lastTick = time;
        number = 9;
      }
    }
    else if (operationState == OperationState::Closing)
    {
      door_position -= speedIncrement;
      if (door_position <= 0)
      {
        door_position = 0;
        operationState = OperationState::Closed;
        screenNeedsRefresh = true;
        TFTscreen.fillScreen(0);
      }
    }

    if (automate)
    {
      can_count_down = !(hours >= 8 && hours <= 18);
    }

    if (!can_count_down)
    {
      number = 9;
    }

    if (++miliseconds >= CLOCK_TICK_LOOK_FOR) {
      miliseconds = 0;
      if (state == State::Displaying)
        screenNeedsRefresh = true;
      if (++minutes >= 60) {
        minutes = 0;
        if (++hours >= 24) {
          hours = 0;
        }
        if (hours == 8 && operationState == OperationState::Closed) {
          operationState = OperationState::Opening;
          screenNeedsRefresh = true;
        }
        else if (hours == 18 && operationState == OperationState::Open) {
          operationState = OperationState::Closing;
          screenNeedsRefresh = true;
        }
      }
    }
  }

  if (buttonOKEvent) 
  {
    noInterrupts(); buttonOKEvent = false; interrupts();

    lastButtonOKPress = time;
  }

  if (time - lastButtonOKPress == DEBOUNCE_TIME_MS && digitalRead(BUTTON_OK) == LOW)
  {
    lastButtonOKPress = -1;
    Serial.println(F("BUTTON OK activated"));

    TFTscreen.fillScreen(0);
    screenNeedsRefresh = true;

    switch (state)
    {
      case State::Displaying:
        state = State::Settings;
        updateOptionsMenuValues();
        settingsTab = 0;
        dialValue = 0;
        break;

      case State::Settings:
        switch (settingTabEnums[settingsTab])
        {
          case Settings::Nothing:
            state = State::Displaying;
            break;
          
          case Settings::Reset:
            resetSettings();
            updateEEPROM();
            number = 0;
            hours = 12;
            minutes = 0;
            state = State::Displaying;
            break;
          
          case Settings::Brightness:
            state = State::ChangingValue;
            dialValue = brightnessMode;
            dial->max = BRIGTHNESS_MODES_MAX-1;
            break;
          
          case Settings::Time:
            state = State::ChangingValue;
            dialValue = timeMode;
            dial->max = TIME_MODES_MAX-1;
            break;
          
          case Settings::Clock:
            state = State::ChangingValue;
            newHours = hours;
            newMinutes = minutes;
            changingMinutes = false;
            dialValue = hours;
            break;
          
          case Settings::Speed:
            state = State::ChangingValue;
            dialValue = speedMode;
            dial->max = SPEED_MODES_MAX-1;
            break;

          case Settings::Automation:
            state = State::ChangingValue;
            dialValue = automate ? 1 : 0;
            dial->max = 1;
            break;
        }
        break;
      
      case State::ChangingValue:
        if (settingTabEnums[settingsTab] == Settings::Clock) {
          if (changingMinutes == false)
          {
            changingMinutes = true;
            newHours = dialValue;
            dialValue = newMinutes;
            screenNeedsRefresh = true;
            break;
          } else {
            hours = newHours;
            minutes = dialValue;
          }
        }
        state = State::Settings;
        updateOptionsMenuValues();
        dialValue = settingsTab;
        updateEEPROMIfChanged();
        break;
    }
  }

  if (button1Event)
  {
    noInterrupts(); button1Event = false; interrupts();

    lastButton1Press = time;
  }

  if (time - lastButton1Press == DEBOUNCE_SWITCH_TIME_MS)
  {
    dialValue += button2State ? 1 : -1;
    
    lastButton1Press = -1;
    Serial.println(F("BUTTON 1 activated"));

    switch (state)
    {
      case State::Settings:
        if (dialValue >= settingTabsCount) {
          dialValue = 0;
        } else if (dialValue < 0) {
          dialValue = settingTabsCount - 1;
        }

        if (settingsTab != dialValue)
        {
          settingsTab = dialValue;
          screenNeedsRefresh = true;
        }
        break;

      case State::ChangingValue:
        switch (settingTabEnums[settingsTab])
        {
          case Settings::Brightness:
            if (dialValue >= BRIGTHNESS_MODES_MAX) {
              dialValue = BRIGTHNESS_MODES_MAX-1;
            } else if (dialValue < 0) {
              dialValue = 0;
            }
            if (brightnessMode != dialValue) {
              brightnessMode = dialValue;
            }
            updateDialWheel(dial, dialValue, TFTscreen);
            drawDigit(TFTscreen, dialValue);
            brightnessValue = brightnessModes[brightnessMode];
            break;

          case Settings::Time:
            if (dialValue >= TIME_MODES_MAX) {
              dialValue = TIME_MODES_MAX-1;
            } else if (dialValue < 0) {
              dialValue = 0;
            }
            if (timeMode != dialValue) {
              timeMode = dialValue;
            }
            updateDialWheel(dial, dialValue, TFTscreen);
            drawDigit(TFTscreen, dialValue);
            tickTimeMs = timeModes[timeMode];
            break;
          
          case Settings::Speed:
            if (dialValue >= SPEED_MODES_MAX) {
              dialValue = SPEED_MODES_MAX-1;
            } else if (dialValue < 0) {
              dialValue = 0;
            }
            if (speedMode != dialValue) {
              speedMode = dialValue;
            }
            updateDialWheel(dial, dialValue, TFTscreen);
            drawDigit(TFTscreen, dialValue);
            speedIncrement = speedModes[speedMode];
            break;
          
          case Settings::Automation:
            if (dialValue > 1) {
              dialValue = 1;
            } else if (dialValue < 0) {
              dialValue = 0;
            }
            updateDialWheel(dial, dialValue, TFTscreen);
            automate = dialValue == 1;
            drawChar(TFTscreen, automate ? 'Y' : 'N');
            if (automate) {
              if (hours >= 8 && hours <= 18 && operationState == OperationState::Closed) {
                operationState = OperationState::Opening;
                screenNeedsRefresh = true;
              }
              else if (!(hours >= 8 && hours <= 18) && operationState == OperationState::Open) {
                operationState = OperationState::Closing;
                screenNeedsRefresh = true;
              }
            }
            break;
          
          case Settings::Clock:
            if (dialValue > (changingMinutes ? 59 : 23)) {
              dialValue = 0;
            } else if (dialValue < 0) {
              dialValue = changingMinutes ? 59 : 23;
            }
            if (changingMinutes) {
              drawMinutes(TFTscreen, dialValue, 0, 20, 0x37E6);
            } else {
              drawHours(TFTscreen, dialValue, 0, 20, 0x37E6);
            }
            break;
        }
        break;
    }
  }

  if (time - lastTick >= tickTimeMs)
  {
    lastTick = time;

    if (!automate && APDS.proximityAvailable()) {
      int sensor_val = APDS.readProximity();
      can_count_down = sensor_val > SENSOR_REACT_THRESHOLD;
    }

    if (dotTicked)
    {
      if (can_count_down && --number == 0)
      {
        switch (operationState)
        {
          case OperationState::Closed:
            operationState = OperationState::Opening;
            screenNeedsRefresh = true;
            break;
          case OperationState::Open:
            operationState = OperationState::Closing;
            screenNeedsRefresh = true;
            break;
        }
      }
    }

    dotTicked = !dotTicked;
  }

  switch (state)
  {

    case State::Displaying:
      if (screenNeedsRefresh) {
        Serial.println(F("REFRESHING CLOCK"));
        screenNeedsRefresh = false;
        switch (operationState) {
          case OperationState::Opening:
          case OperationState::Closing:
            redrawDoor(door, TFTscreen);
            updateDoor(door, door_position, TFTscreen);
            break;
          case OperationState::Closed:
            drawClock(TFTscreen, hours, minutes, 0, 0);
        }
      }
      break;

    case State::Settings:
      if (screenNeedsRefresh) {
        screenNeedsRefresh = false;
        Serial.println(F("REFRESHING"));
        drawTitle(TFTscreen, "SETTINGS");
        int minToDraw = settingsTab > 0 ? settingsTab - 1 : 0;
        if (minToDraw + 2 > settingTabsCount) {
          minToDraw = settingTabsCount - 2;
        }
        drawOptionsMenu(TFTscreen, settingTabNames, settingTabValues, settingTabsCount, minToDraw, settingsTab);
      }
      break;
      
    case State::ChangingValue:
      if (screenNeedsRefresh) {
        screenNeedsRefresh = false;
        switch (settingTabEnums[settingsTab])
        {
          case Settings::Brightness:
            drawTitleSubtitle(TFTscreen, "SETTINGS", "BRIGHTNESS");
            redrawDialWheel(dial, TFTscreen);
            updateDialWheel(dial, dialValue, TFTscreen);
            drawDigit(TFTscreen, dialValue);
            break;
          case Settings::Time:
            drawTitleSubtitle(TFTscreen, "SETTINGS", "TIMING");
            redrawDialWheel(dial, TFTscreen);
            updateDialWheel(dial, dialValue, TFTscreen);
            drawDigit(TFTscreen, dialValue);
            break;
          case Settings::Speed:
            drawTitleSubtitle(TFTscreen, "SETTINGS", "SPEED");
            redrawDialWheel(dial, TFTscreen);
            updateDialWheel(dial, dialValue, TFTscreen);
            drawDigit(TFTscreen, dialValue);
            break;
          case Settings::Automation:
            drawTitleSubtitle(TFTscreen, "SETTINGS", "AUTOMATION");
            redrawDialWheel(dial, TFTscreen);
            updateDialWheel(dial, dialValue, TFTscreen);
            drawChar(TFTscreen, automate ? 'Y' : 'N');
            break;
          case Settings::Clock:
            drawTitleSubtitle(TFTscreen, "SETTINGS", "CLOCK");
            drawHours(TFTscreen, newHours, 0, 20, !changingMinutes ? 0x37E6 : 0xFFFF);
            drawClockSeparator(TFTscreen, 0, 20, 0xFFFF);
            drawMinutes(TFTscreen, newMinutes, 0, 20, changingMinutes ? 0x37E6 : 0xFFFF);
            break;
        }
      }
      break;
  }

  analogWrite(BRIGHTNESS_PIN, brightnessValue);

  int servo_pos = map(door_position, 0, DOOR_POSITION_MAX, 750, 2250);
  doorServo.write(servo_pos);
}

// from https://devcoons.com/crc8/
uint8_t CRC8(const uint8_t *data,int length) 
{
  uint8_t crc = 0x00;
  uint8_t extract;
  uint8_t sum;
  for(int i=0;i<length;i++)
  {
    extract = *data;
    for (uint8_t tempI = 8; tempI; tempI--) 
    {
        sum = (crc ^ extract) & 0x01;
        crc >>= 1;
        if (sum)
          crc ^= 0x8C;
        extract >>= 1;
    }
    data++;
  }
  return crc;
}