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
#include "draw.h"

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
const int BRIGHTNESS_PIN = 11;
const int SERVO_PIN  = 5;
const int SENSOR_PIN = A0;

const int DEBOUNCE_TIME_MS = 20;
const int SENSOR_REACT_THRESHOLD = 400;

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
  short timeMode;
  bool automaticMode;
  bool speedMode;
  uint8_t crc;
};

#define TIME_MODES_MAX 9
const uint32_t timeModes[] = {20, 50, 100, 250, 500, 750, 1000, 5000, 60000};

#define SPEED_MODES_MAX 7
const uint32_t speedModes[] = {1, 2, 5, 10, 45, 90, 180};

volatile bool buttonOKEvent = false;
volatile bool button1Event = false;
volatile bool timerEvent = false;

int32_t dialValue = 0;

uint32_t time = 0;
uint32_t lastButtonOKPress = 0;
uint32_t lastButton1Press = 0;

short timeMode = 4;
uint32_t tickTimeMs = 500;
short speedMode = 4;
uint32_t speedIncrement = 10;
bool automate = true;

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

uint16_t hours = 12;
uint16_t minutes = 0;
uint32_t miliseconds = 0;
const uint32_t CLOCK_TICK_LOOK_FOR = 1000;

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
  dialValue += digitalRead(BUTTON_2) == LOW ? 1 : -1;
  button1Event = true;
}

ISR(TIMER2_COMPA_vect)
{
  timerEvent = true;
}

void resetSettings()
{
  timeMode = 4;
  speedMode = 3;
  automate = true;

  tickTimeMs = timeModes[timeMode];
  speedIncrement = speedModes[speedMode];
}

void validateEEPROM()
{
  EEPROM.get(USER_DATA_EEPORM_ADDRESS, data);

  if (data.magic != USER_DATA_MAGIC || data.version != USER_DATA_VERSION)
  { 
    Serial.println("Magic or version mismatch! Resetting settings");
    resetSettings();
    updateEEPROM();
    return;
  }

  uint8_t crc = CRC8((uint8_t*)&data, sizeof(data) - 1);
  if (data.crc != crc)
  {
    Serial.println("CRC mismatch! Resetting settings");
    resetSettings();
    updateEEPROM();
    return;
  }

  Serial.println("EEPROM validated");

  timeMode = data.timeMode;
  speedMode = data.speedMode;
  automate = data.automaticMode;

  tickTimeMs = timeModes[timeMode];
  speedIncrement = speedModes[speedMode];
}

void updateEEPROM()
{
  data = {};
  data.magic = USER_DATA_MAGIC;
  data.version = USER_DATA_VERSION;
  data.timeMode = timeMode;
  data.speedMode = speedMode;
  data.automaticMode = automate;

  uint8_t crc = CRC8((uint8_t*)&data, sizeof(data) - 1);
  data.crc = crc;

  Serial.println("Updating EEPROM");
  EEPROM.put(USER_DATA_EEPORM_ADDRESS, data);
}

void updateEEPROMIfChanged()
{
  if (data.timeMode == timeMode
      && data.speedMode == speedMode
      && data.automaticMode == automate)
  {
    Serial.println("EEPROM matches, no need to update");
    return;
  }

  updateEEPROM();
}

void setup()
{ 
  Serial.begin(9600);

  for(int i = 4; i <= 12; i++)
  {
    pinMode(i, OUTPUT);
  }

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
}

void loop()
{
  if (timerEvent)
  {
    timerEvent = false;
    time++;

    int sensor_val = analogRead(SENSOR_PIN);
    if (automate && sensor_val >= SENSOR_REACT_THRESHOLD && operationState == OperationState::Closed)
    {
      operationState = OperationState::Opening;
    }

    if (operationState == OperationState::Opening)
    {
      door_position += speedIncrement;
      if (door_position >= DOOR_POSITION_MAX)
      {
        door_position = DOOR_POSITION_MAX;
        operationState = OperationState::Open;
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
      }
    }

    if (automate)
    {
      can_count_down = analogRead(SENSOR_PIN) < SENSOR_REACT_THRESHOLD;
    }
    else
    {
      can_count_down = digitalRead(BUTTON_2) == LOW;
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
      }
    }
  }

  if (buttonOKEvent) 
  {
    noInterrupts(); buttonOKEvent = false; interrupts();

    lastButtonOKPress = time;
  }

  if (time - lastButtonOKPress == DEBOUNCE_TIME_MS)
  {
    lastButtonOKPress = -1;
    Serial.println("BUTTON OK activated");

    TFTscreen.fillScreen(0);
    screenNeedsRefresh = true;

    switch (state)
    {
      case State::Displaying:
        state = State::Settings;
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
            state = State::Displaying;
            break;
          
          case Settings::Time:
            state = State::ChangingValue;
            dialValue = timeMode;
            dial->max = TIME_MODES_MAX-1;
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
        state = State::Settings;
        updateEEPROMIfChanged();
        break;
    }
  }

  if (button1Event)
  {
    noInterrupts(); button1Event = false; interrupts();

    lastButton1Press = time;
  }

  if (time - lastButton1Press == DEBOUNCE_TIME_MS)
  {
    lastButton1Press = -1;
    Serial.println("BUTTON 1 activated");

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
          case Settings::Time:
            if (dialValue >= TIME_MODES_MAX) {
              dialValue = TIME_MODES_MAX-1;
            } else if (dialValue < 0) {
              dialValue = 0;
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
            break;
        }
        break;
    }

  //   switch (state)
  //   {
  //     case State::Displaying:
  //       if (!automate)
  //       {
  //         switch (operationState)
  //         {
  //           case OperationState::Open:
  //             operationState = OperationState::Closing;
  //             break;
  //           case OperationState::Closed:
  //             operationState = OperationState::Opening;
  //             break;
  //         }
  //       }
  //       break;

  //     case State::Settings:
  //       switch (settingsState)
  //       {
  //         case Settings::Time:
  //           settingsState = Settings::Speed;
  //           break;
          
  //         case Settings::Speed:
  //           settingsState = Settings::Automation;
  //           break;
          
  //         case Settings::Automation:
  //           settingsState = Settings::Nothing;
  //           break;

  //         case Settings::Nothing:
  //           settingsState = Settings::Reset;
  //           break;
          
  //         case Settings::Reset:
  //           settingsState = Settings::Time;
  //           break;
  //       }
  //       break;
      
  //     case State::ChangingValue:
  //       switch (settingsState)
  //       {
  //         case Settings::Time:
  //           if (++timeMode == TIME_MODES_MAX) timeMode = 0;
  //           tickTimeMs = timeModes[timeMode];
  //           break;
          
  //         case Settings::Speed:
  //           if (++speedMode == SPEED_MODES_MAX) speedMode = 0;
  //           speedIncrement = speedModes[speedMode];
  //           break;
          
  //         case Settings::Automation:
  //           automate = !automate;
  //           break;
  //       }
  //       break;
    // }
  }

  if (time - lastTick >= tickTimeMs)
  {
    lastTick = time;
    if (dotTicked)
    {
      if (can_count_down && --number == 0)
      {
        switch (operationState)
        {
          case OperationState::Closed:
            operationState = OperationState::Opening;
            break;
          case OperationState::Open:
            operationState = OperationState::Closing;
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
        screenNeedsRefresh = false;
        drawClock(TFTscreen, hours, minutes, 0, 0);
      }
      break;

    case State::Settings:
      if (screenNeedsRefresh) {
        screenNeedsRefresh = false;
        Serial.println("REFRESHING");
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
          case Settings::Time:
            drawTitleSubtitle(TFTscreen, "SETTINGS", "TIMING");
            updateDialWheel(dial, dialValue, TFTscreen);
            drawDigit(TFTscreen, dialValue);
            break;
          case Settings::Speed:
            drawTitleSubtitle(TFTscreen, "SETTINGS", "SPEED");
            updateDialWheel(dial, dialValue, TFTscreen);
            drawDigit(TFTscreen, dialValue);
            break;
          case Settings::Automation:
            drawTitleSubtitle(TFTscreen, "SETTINGS", "AUTOMATION");
            updateDialWheel(dial, dialValue, TFTscreen);
            drawChar(TFTscreen, automate ? 'Y' : 'N');
            break;
        }
        redrawDialWheel(dial, TFTscreen);
      }
      break;
  }

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