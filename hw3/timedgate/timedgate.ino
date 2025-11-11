/*
  VU MIF
  Informatics
  Robotics homework 3
  @author: Donatas Kirda (sid: 2212166)
*/

#include <Arduino.h>
#include <EEPROM.h>
#include <ServoTimer2.h>

uint8_t CRC8(const uint8_t *data, int length);

#define USER_DATA_EEPORM_ADDRESS 0
#define USER_DATA_MAGIC 83
#define USER_DATA_VERSION 2

const int DATA_PIN   = 4;
const int LATCH_PIN  = 5;
const int CLOCK_PIN  = 6;
const int BUTTON_2   = 3;
const int BUTTON_1   = 2;
const int BRIGHTNESS_PIN = 11;
const int SERVO_PIN  = 10;
const int SENSOR_PIN = A0;

const int DEBOUNCE_TIME_MS = 10;
const int SENSOR_REACT_THRESHOLD = 400;

#define SEGMENT_CLEAR B00000000
#define SYMBOL_1      B01100000
#define SYMBOL_2      B11011010
#define SYMBOL_3      B11110010
#define SYMBOL_4      B01100110
#define SYMBOL_5      B10110110
#define SYMBOL_6      B10111110
#define SYMBOL_7      B11100000
#define SYMBOL_8      B11111110
#define SYMBOL_9      B11110110
#define SYMBOL_0      B11111100
#define SYMBOL_P      B11001110
#define SYMBOL_R      B00001010
#define SYMBOL_T      B00011110
#define SYMBOL_N      B00101010
#define SYMBOL_B      B00111110
#define SYMBOL_S      B10110110
#define SYMBOL_Y      B01110110
#define SYMBOL_A      B11101110
#define SYMBOL_D      B01111010
#define SYMBOL_DASH   B00000010

const byte digits[] = {
  SYMBOL_0,
  SYMBOL_1,
  SYMBOL_2,
  SYMBOL_3,
  SYMBOL_4,
  SYMBOL_5,
  SYMBOL_6,
  SYMBOL_7,
  SYMBOL_8,
  SYMBOL_9
};

const short throbber_frame_count = 6;
const short throbber_frame_time = 50;
const byte throbber_frames[] = {
  B00000100,
  B00001000,
  B00010000,
  B00100000,
  B01000000,
  B10000000
};

enum class State
{
  Idle,
  Settings,
  Displaying,
  ChangingValue
};

enum class OperationState
{
  Closed,
  Opening,
  Open,
  Closing
};

enum class Settings
{
  Power,
  Reset,
  Time,
  Nothing,
  Speed,
  Automation,
  Debug
};

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

volatile bool button1Event = false;
volatile bool button2Event = false;
volatile bool timerEvent = false;

uint32_t time = 0;
uint32_t lastButton1Press = 0;
uint32_t lastButton2Press = 0;

short timeMode = 4;
uint32_t tickTimeMs = 500;
short speedMode = 4;
uint32_t speedIncrement = 10;
bool automate = true;

uint32_t lastTick = 0;
bool dotTicked = false;
short number = 0;
bool can_count_down = false;

short throbber_frame_number = 0;
short throbber_frame_direction = 1;

#define DOOR_POSITION_MAX 1023
volatile int door_position = 0;

State state = State::Idle;
OperationState operationState = OperationState::Closed;
Settings settingsState = Settings::Power;
UserData data;

ServoTimer2 doorServo;

byte segmentDisplayLastState = 0;

void writeSegmentBit(byte data, byte mask)
{
  segmentDisplayLastState &= ~mask;
  segmentDisplayLastState |= data & mask;
  sendDataToShiftRegister(segmentDisplayLastState);
}

void writeSegmentData(byte data, bool with_point = false)
{
  if (with_point)
  {
    segmentDisplayLastState = data;
  }
  else
  {
    segmentDisplayLastState = (data & B11111110) | (segmentDisplayLastState & B00000001);
  }
  sendDataToShiftRegister(segmentDisplayLastState);
}

void sendDataToShiftRegister(byte data)
{
  digitalWrite(LATCH_PIN, LOW);
  shiftOut(DATA_PIN, CLOCK_PIN, LSBFIRST, data);
  digitalWrite(LATCH_PIN, HIGH);
}

void setupTimer()
{
  noInterrupts();
  // just reset to normal values
  TCCR1A = 0;
  TCCR1B = 0;
  // clear timer on compare (CTC)
  TCCR1B |= (1 << WGM12);
  // prescale so instead of 16Mhz we run timer at 250Khz ticks (every 0.004ms or 4us)
  TCCR1B |= (1 << CS11) | (1 << CS10);
  // comparison target
  // 0.004ms x [250 ticks] = 1ms
  // subtract -1 cause timer starts at 0 (must be lower than 65536)
  OCR1A = 250 - 1;
  // enable compare A interrupt so we can utilise the timer event
  TIMSK1 |= (1 << OCIE1A);
  interrupts();
}

void onButton1ISR()
{
  button1Event = true;
}

void onButton2ISR()
{
  button2Event = true;
}

ISR(TIMER1_COMPA_vect)
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

  pinMode(BUTTON_1, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(BUTTON_1), onButton1ISR, RISING);

  pinMode(BUTTON_2, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(BUTTON_2), onButton2ISR, RISING);

  setupTimer();

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
      throbber_frame_direction = -1;
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

    if (time % throbber_frame_time == 0)
    {
      if (throbber_frame_direction == 1)
      {
        if (++throbber_frame_number >= throbber_frame_count)
        {
          throbber_frame_number = 0;
        }
      }
      else
      {
        if (--throbber_frame_number < 0)
        {
          throbber_frame_number = throbber_frame_count - 1;
        }
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
  }

  if (button1Event) 
  {
    noInterrupts(); button1Event = false; interrupts();

    lastButton1Press = time;
  }

  if (time - lastButton1Press == DEBOUNCE_TIME_MS && digitalRead(BUTTON_1) == HIGH)
  {
    lastButton1Press = -1;
    Serial.println("BUTTON 1 activated");

    switch (state)
    {
      case State::Idle:
        state = State::Displaying;
        break;

      case State::Displaying:
        state = State::Settings;
        settingsState = Settings::Power;
        break;

      case State::Settings:
        switch (settingsState)
        {
          case Settings::Nothing:
            state = State::Displaying;
            break;

          case Settings::Power:
            state = State::Idle;
            break;
          
          case Settings::Reset:
            resetSettings();
            updateEEPROM();
            number = 0;
            state = State::Displaying;
            break;
          
          case Settings::Time:
            state = State::ChangingValue;
            break;
          
          case Settings::Speed:
            state = State::ChangingValue;
            break;

          case Settings::Automation:
            state = State::ChangingValue;
            break;
        }
        break;
      
      case State::ChangingValue:
        state = State::Displaying;
        updateEEPROMIfChanged();
        break;
    }
  }

  if (button2Event)
  {
    noInterrupts(); button2Event = false; interrupts();

    lastButton2Press = time;
  }

  if (time - lastButton2Press == DEBOUNCE_TIME_MS && digitalRead(BUTTON_2) == HIGH)
  {
    lastButton2Press = -1;
    Serial.println("BUTTON 2 activated");

    switch (state)
    {
      case State::Idle:
      case State::Displaying:
        if (!automate)
        {
          switch (operationState)
          {
            case OperationState::Open:
              operationState = OperationState::Closing;
              throbber_frame_direction = 1;
              break;
            case OperationState::Closed:
              operationState = OperationState::Opening;
              throbber_frame_direction = -1;
              break;
          }
        }
        break;

      case State::Settings:
        switch (settingsState)
        {
          case Settings::Power:
            settingsState = Settings::Time;
            break;
          
          case Settings::Time:
            settingsState = Settings::Speed;
            break;
          
          case Settings::Speed:
            settingsState = Settings::Automation;
            break;
          
          case Settings::Automation:
            settingsState = Settings::Nothing;
            break;

          case Settings::Nothing:
            settingsState = Settings::Reset;
            break;
          
          case Settings::Reset:
            settingsState = Settings::Power;
            break;
        }
        break;
      
      case State::ChangingValue:
        switch (settingsState)
        {
          case Settings::Time:
            if (++timeMode == TIME_MODES_MAX) timeMode = 0;
            tickTimeMs = timeModes[timeMode];
            break;
          
          case Settings::Speed:
            if (++speedMode == SPEED_MODES_MAX) speedMode = 0;
            speedIncrement = speedModes[speedMode];
            break;
          
          case Settings::Automation:
            automate = !automate;
            break;
        }
        break;
    }
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
              throbber_frame_direction = -1;
            break;
          case OperationState::Open:
            operationState = OperationState::Closing;
              throbber_frame_direction = 1;
            break;
        }
      }
    }

    dotTicked = !dotTicked;
    writeSegmentBit(dotTicked ? B00000001 : B00000000, B00000001);
  }

  switch (state)
  {
    case State::Idle:
      writeSegmentData(SEGMENT_CLEAR);
      break;

    case State::Displaying:
      switch (operationState) {
        case OperationState::Closed:
          writeSegmentData(SYMBOL_DASH);
          break;
        case OperationState::Closing:
        case OperationState::Opening:
          writeSegmentData(throbber_frames[throbber_frame_number]);
          break;
        case OperationState::Open:
          if (can_count_down)
          {
            writeSegmentData(digits[number]);
          }
          else
          {
            writeSegmentData(SYMBOL_DASH);
          }
          break;
      }
      break;

    case State::Settings:
      switch (settingsState)
      {
        case Settings::Power:
          writeSegmentData(SYMBOL_P);
          break;
        case Settings::Nothing:
          writeSegmentData(SYMBOL_N);
          break;
        case Settings::Reset:
          writeSegmentData(SYMBOL_R);
          break;
        case Settings::Time:
          writeSegmentData(SYMBOL_T);
          break;
        case Settings::Speed:
          writeSegmentData(SYMBOL_S);
          break;
        case Settings::Automation:
          writeSegmentData(SYMBOL_A);
          break;
      }
      break;
      
    case State::ChangingValue:
      short digit = -1;
      switch (settingsState)
      {
        case Settings::Time:
          digit = timeMode + 1;
          break;
        case Settings::Speed:
          digit = speedMode + 1;
          break;
        case Settings::Automation:
          writeSegmentData(automate ? SYMBOL_Y : SYMBOL_N, false);
          break;
      }
      if (digit != -1)
        writeSegmentData(digits[digit]);
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