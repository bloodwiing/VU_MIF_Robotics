#include <math.h>
#include <LiquidCrystal.h>

#define PHOTO_IN A0
#define LIGHT_OUT 11
#define BUTTON_IN 8
#define LCD_RS 6
#define LCD_ENABLE 7
#define LCD_DB4 2
#define LCD_DB5 3
#define LCD_DB6 4
#define LCD_DB7 5

#define LIGHT_MAX 800
#define LIGHT_MIN 150

#define TICK_TIME 20
#define DEBOUNCE_WAIT 200

enum LightState {
  OFF = 0,
  REACTIVE = 1,
  NIGHT_LIGHT = 2,
  BRIGHT = 3,
};

LightState state = REACTIVE;
int debouncing = 0;

LiquidCrystal lcd(LCD_RS, LCD_ENABLE, LCD_DB4, LCD_DB5, LCD_DB6, LCD_DB7);

void setup()
{
  pinMode(LIGHT_OUT, OUTPUT);
  pinMode(BUTTON_IN, INPUT_PULLUP);
  lcd.begin(16, 2);
  Serial.begin(9600);
  
  state = OFF;
  processButtonPress();
}

float invLerpF(int a, int b, int x)
{
  if (x < a) {
    return 0.0;
  }
  if (x > b) {
    return 1.0;
  }
  return ((float)x - (float)a) / ((float)b - (float)a);;
}

int lerpI(int a, int b, float p)
{
  if (p <= 0.0) {
    return a;
  }
  if (p >= 1.0) {
    return b;
  }
  float r = p * (1.0 - (float)a) + p * (float)b;
  return (int)round(r);
}

void processButtonPress()
{
  lcd.clear();
  Serial.print("Switching to ");
  switch (state) {
    case OFF:
      Serial.print("REACTIVE");
      lcd.print("MODE: REACTIVE");
      state = REACTIVE;
      break;
    case REACTIVE:
      Serial.print("NIGHT LIGHT");
      lcd.print("MODE: NIGHT");
      state = NIGHT_LIGHT;
      break;
    case NIGHT_LIGHT:
      Serial.print("BRIGHT");
      lcd.print("MODE: FULL");
      state = BRIGHT;
      break;
    case BRIGHT:
      Serial.print("OFF");
      lcd.print("MODE: OFF");
      state = OFF;
      break;
  }
  Serial.println(" mode");
  
  lcd.setCursor(0, 1);
  lcd.print("LIGHT: ");
}

int getLightBrightness()
{
  int light_value;
  float percent;
  int brightness;
  
  lcd.setCursor(7, 1);
  
  switch (state) {
    case OFF:
      lcd.print("  0%");
      return 0;
      break;
    case REACTIVE:
      light_value = analogRead(PHOTO_IN);
      percent = 1.0 - invLerpF(LIGHT_MIN, LIGHT_MAX, light_value);
      brightness = lerpI(0, 255, percent);
      
      Serial.print(light_value);
      Serial.print(" => ");
      Serial.println(brightness);
    
      if (percent < 1) {
        lcd.print(" ");
      }
      if (percent < 0.1) {
        lcd.print(" ");
      }
      lcd.print(round(percent * 100.0), 10);
      lcd.print("%  ");
    
      return brightness;
      break;
    case NIGHT_LIGHT:
      lcd.print(" 10%");
      return 25;
      break;
    case BRIGHT:
      lcd.print("100%");
      return 255;
      break;
  }
}

void loop()
{
  if (debouncing > 0) {
    debouncing -= TICK_TIME;
  }
  
  int button_value = digitalRead(BUTTON_IN);
  if (button_value == LOW) {
    if (debouncing <= 0) {
      processButtonPress();
    }
    debouncing = DEBOUNCE_WAIT;
  }
  
  analogWrite(LIGHT_OUT, getLightBrightness());
  
  delay(TICK_TIME);
}

