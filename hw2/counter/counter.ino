/*
  VU MIF
  Informatics
  Robotics homework 2
  @author: Donatas Kirda (sid: 2212166)
*/

#include <Arduino.h>
#include <EEPROM.h>

uint8_t CRC8(const uint8_t *data, int length);

#define USER_DATA_EEPORM_ADDRESS 0
#define USER_DATA_MAGIC 37
#define USER_DATA_VERSION 1

const int SEGMENT_A  = 5;
const int SEGMENT_B  = 4;
const int SEGMENT_C  = 10;
const int SEGMENT_D  = 9;
const int SEGMENT_E  = 8;
const int SEGMENT_F  = 6;
const int SEGMENT_G  = 7;
const int SEGMENT_DP = 12;
const int BUTTON_2   = 3;
const int BUTTON_1   = 2;
const int BRIGHTNESS_PIN = 11;

const int DEBOUNCE_TIME_MS = 20;

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

enum class State
{
  Idle,
  Settings,
  Time,
  ChangingValue
};

enum class Settings
{
  Power,
  Reset,
  Time,
  Nothing,
  Brightness
};

struct UserData
{
  byte magic;
  byte version;
  short timeMode;
  short brightnessMode;
  uint8_t crc;
};

#define TIME_MODES_MAX 9
const uint32_t timeModes[] = {20, 50, 100, 250, 500, 750, 1000, 5000, 60000};

#define BRIGTHNESS_MODES_MAX 7
const byte brightnessModes[] = {1, 10, 20, 45, 80, 140, 255};
// const byte brightnessModes[] = {240, 175, 100, 60, 0};

volatile bool button1Event = false;
volatile bool button2Event = false;
volatile bool timerEvent = false;

uint32_t time = 0;
uint32_t lastButton1Press = 0;
uint32_t lastButton2Press = 0;

short timeMode = 4;
uint32_t tickTimeMs = 500;
short brightnessMode = 4;
byte brightness = 80;

uint32_t lastTick = 0;
bool dotTicked = false;
short number = 0;

State state = State::Idle;
Settings settingsState = Settings::Power;
UserData data;

void writeSegmentData(byte data, bool with_point = false)
{
  digitalWrite(SEGMENT_A, data & B10000000 ? HIGH : LOW);
  digitalWrite(SEGMENT_B, data & B01000000 ? HIGH : LOW);
  digitalWrite(SEGMENT_C, data & B00100000 ? HIGH : LOW);
  digitalWrite(SEGMENT_D, data & B00010000 ? HIGH : LOW);
  digitalWrite(SEGMENT_E, data & B00001000 ? HIGH : LOW);
  digitalWrite(SEGMENT_F, data & B00000100 ? HIGH : LOW);
  digitalWrite(SEGMENT_G, data & B00000010 ? HIGH : LOW);
  if (with_point)
  {
    digitalWrite(SEGMENT_DP, data & B00000001 ? HIGH : LOW);
  }
}

void setupTimer()
{
  noInterrupts();
  // just reset to normal values
  TCCR1A = 0;
  TCCR1B = 0;
  // clear timer on compare
  TCCR1B |= (1 << WGM12);
  // prescale so instead of 16Mhz we run timer at 250Khz ticks (every 0.004ms or 4us)
  TCCR1B |= (1 << CS11) | (1 << CS10);
  // comparison target A
  // 0.004ms x [250 ticks] = 1ms
  // subtract -1 cause timer starts at 0
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
  brightnessMode = 3;

  tickTimeMs = timeModes[timeMode];
  brightness = brightnessModes[brightnessMode];
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
  brightnessMode = data.brightnessMode;

  tickTimeMs = timeModes[timeMode];
  brightness = brightnessModes[brightnessMode];
}

void updateEEPROM()
{
  data = {};
  data.magic = USER_DATA_MAGIC;
  data.version = USER_DATA_VERSION;
  data.timeMode = timeMode;
  data.brightnessMode = brightnessMode;

  uint8_t crc = CRC8((uint8_t*)&data, sizeof(data) - 1);
  data.crc = crc;

  Serial.println("Updating EEPROM");
  EEPROM.put(USER_DATA_EEPORM_ADDRESS, data);
}

void updateEEPROMIfChanged()
{
  if (data.timeMode == timeMode && data.brightnessMode == brightnessMode)
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

  validateEEPROM();
}

void loop()
{ 
  if (timerEvent)
  {
    timerEvent = false;
    time++;
  }

  if (button1Event) 
  {
    noInterrupts(); button1Event = false; interrupts();

    lastButton1Press = time;
  }

  if (time - lastButton1Press == DEBOUNCE_TIME_MS && digitalRead(BUTTON_1) == HIGH)
  {
    lastButton1Press = -1;

    switch (state)
    {
      case State::Idle:
        state = State::Time;
        break;

      case State::Time:
        state = State::Settings;
        settingsState = Settings::Power;
        break;

      case State::Settings:
        switch (settingsState)
        {
          case Settings::Nothing:
            state = State::Time;
            break;

          case Settings::Power:
            state = State::Idle;
            break;
          
          case Settings::Reset:
            resetSettings();
            updateEEPROM();
            number = 0;
            state = State::Time;
            break;
          
          case Settings::Time:
            state = State::ChangingValue;
            break;
          
          case Settings::Brightness:
            state = State::ChangingValue;
            break;
        }
        break;
      
      case State::ChangingValue:
        state = State::Time;
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

    switch (state)
    {
      case State::Idle:
        state = State::Time;
        break;

      case State::Settings:
        switch (settingsState)
        {
          case Settings::Power:
            settingsState = Settings::Time;
            break;
          
          case Settings::Time:
            settingsState = Settings::Brightness;
            break;
          
          case Settings::Brightness:
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
          
          case Settings::Brightness:
            if (++brightnessMode == BRIGTHNESS_MODES_MAX) brightnessMode = 0;
            brightness = brightnessModes[brightnessMode];
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
      if (++number == 10) number = 0;
    }

    dotTicked = !dotTicked;
    digitalWrite(SEGMENT_DP, dotTicked ? HIGH : LOW);
  }

  switch (state)
  {
    case State::Idle:
      writeSegmentData(SEGMENT_CLEAR);
      break;

    case State::Time:
      writeSegmentData(digits[number]);
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
        case Settings::Brightness:
          writeSegmentData(SYMBOL_B);
          break;
      }
      break;
      
    case State::ChangingValue:
      short digit;
      switch (settingsState)
      {
        case Settings::Time:
          digit = timeMode + 1;
          break;
        case Settings::Brightness:
          digit = brightnessMode + 1;
          break;
      }
      writeSegmentData(digits[digit]);
      break;
  }

  analogWrite(BRIGHTNESS_PIN, brightness);
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