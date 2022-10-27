#include <Wire.h>
#include <EEPROM.h>
#include <hd44780.h>
#include <hd44780ioClass/hd44780_I2Cexp.h> // include i/o class header

hd44780_I2Cexp lcd; // declare lcd object: auto locate & config display for hd44780 chip

// init blinking variables
unsigned long lastTimeBlink = 0;
long int timeToBlink = 3000;
String strings[2] = {"", "                "};
int blink = 0;
int stringsIndex = 0;

// table consts
const float minHeight = 741.00; // 74.1cm
const float maxHeight = 1247.00;
const float upSpeed = 25.4; // 25.4mm per second
const float downSpeed = 28.8;

// init buttons pin
const int btnUpPin = 2; // green
const int btnM1Pin = 3;
const int btnSavePin = 4;
const int btnM2Pin = 5;
const int btnDownPin = 6; // red
const int buttons[5] = {btnUpPin, btnM1Pin, btnSavePin, btnM2Pin, btnDownPin};

// init buttons state
int button1State, button2State, button3State, button4State, button5State;

// relay
int relay1Pin = 9;
int relay2Pin = 10;

// EEPROM memory addresses
int eep_height_adr = 0;
int eep_m1_adr = 4;
int eep_m2_adr = 7;

// movement variables
bool manualMove = false;
long int movingTime, timeUp, timeDown;
float currentHeight, m1Height, m2Height;

float float_one_point_round(float value)
{
      return ((float)((int)(value * 10))) / 10;
}

String heightToString(float height)
{
      String newHeight = String(float_one_point_round(height / 10));
      newHeight.remove(newHeight.length() - 1, 1);
      return newHeight;
}

int checkButtonPress()
{
      for (int pin : buttons)
      {
            if (!digitalRead(pin))
                  return pin;
      }
      return 0;
}

void onLoopBtnClick()
{
      switch (checkButtonPress())
      {
      case btnUpPin:
            manualUp();
            break;
      case btnM1Pin:
            goToHeight(m1Height);
            break;
      case btnSavePin:
            saveToMem();
            break;
      case btnM2Pin:
            goToHeight(m2Height);
            break;
      case btnDownPin:
            manualDown();
            break;

      default:
            break;
      }

      // no button selected and no auto move
      if (!digitalRead(btnUpPin) == 0 && timeUp || !digitalRead(btnDownPin) == 0 && timeDown)
            manualMoveStopped();
}

void updateEepromHeight(int adr, float height)
{
      EEPROM.put(adr, height);
}

void clearSecRow()
{
      lcd.setCursor(0, 1);
      lcd.print(strings[1]);
}

void printSecRow(String customString, int index = 0, float height = currentHeight)
{
      if (customString == "")
            customString = String("Current:   " + heightToString(height));
      if (index == 0 && strings[index] != customString)
      {
            strings[index] = customString;
            clearSecRow();
            lcd.setCursor(0, 1);
            lcd.print(strings[index]);
      }
      if (index == 1)
            clearSecRow();
}

void printMem()
{
      lcd.setCursor(0, 0);
      lcd.print(strings[1]);
      lcd.setCursor(0, 0);
      lcd.print("M1-" + heightToString(m1Height) + " " + "M2-" + heightToString(m2Height));
}

void blinkIfNeeded(long int timeToBlink = timeToBlink, int blinkRate = 5)
{
      // check if blinking screen needed
      unsigned long time = millis() - lastTimeBlink;

      if (lastTimeBlink != 0 && time <= timeToBlink)
      {
            if (time % blinkRate == 0)
            {
                  blink += 1;
                  stringsIndex = blink % 2;
                  delay(1);
                  printSecRow(strings[0] + " ", stringsIndex);
            }
      }
      if (time >= lastTimeBlink)
            lastTimeBlink = 0;
      stringsIndex = 0;
}

void saveToMem()
{
      long int start = millis();
      lastTimeBlink = start;
      delay(200);
      while (millis() - start < 5000)
      {
            blinkIfNeeded(4500, 400);
            int pressedBtnPin = checkButtonPress();
            if (pressedBtnPin == btnM1Pin)
            {
                  m1Height = currentHeight;
                  updateEepromHeight(eep_m1_adr, m1Height);
                  Serial.println(m1Height);
            }
            if (pressedBtnPin == btnM2Pin)
            {
                  m2Height = currentHeight;
                  updateEepromHeight(eep_m2_adr, m2Height);
                  Serial.println(m2Height);
            }
            if (pressedBtnPin)
                  break;
      }
      printSecRow(strings[0] + " ", stringsIndex);
      printMem();
      delay(50);
}

void goToHeight(float height)
{
      // do nothing if we close to the destination or not in the mx/min values
      if (height > maxHeight || height < minHeight || (currentHeight - 2 <= height && height <= currentHeight + 2))
            return;

      // check direction and start auto move
      printSecRow("Going:  " + heightToString(height));
      if (currentHeight > height)
      {
            Serial.println("down to ");
            Serial.println(height);
            startAutoMove(height, -1, relay2Pin);
      }
      else
      {
            Serial.println("up to ");
            Serial.println(height);
            startAutoMove(height, 1, relay1Pin);
      }

      // set current height to eeprom and screen
      updateEepromHeight(eep_height_adr, currentHeight);
}

void startAutoMove(float height, int dir, int relayPin)
{
      // calculate how much time we need to move
      int millisToMove = (dir == 1) ? (height - currentHeight) / upSpeed * 1000 : (currentHeight - height) / downSpeed * 1000;
      delay(250);

      // start timer and open relay
      digitalWrite(relayPin, LOW);
      long int start = millis();
      long int movingTime = millis() - start;

      // sleep until we reached time or interrupted
      while (movingTime < millisToMove)
      {
            movingTime = millis() - start;
            if (checkButtonPress() > 0)
                  break;
      }

      // close relay and update current height
      digitalWrite(relayPin, HIGH);
      currentHeight = (dir == 1) ? min(currentHeight + movingTime / 100.00 * upSpeed / 10.00, maxHeight) : max(currentHeight - movingTime / 100.00 * downSpeed / 10.00, minHeight);
      delay(250);
}

void manualUp()
{
      manualMove = true;
      digitalWrite(relay1Pin, LOW);
      if (timeUp == 0)
            timeUp = millis();
}

void manualDown()
{
      manualMove = true;
      digitalWrite(relay2Pin, LOW);
      if (timeDown == 0)
            timeDown = millis();
}

void manualMoveStopped()
{
      // on down movement stop
      if (timeDown > 0)
            handleManualStop(relay2Pin, timeDown, -1);

      // on up movement stop
      if (timeUp > 0)
            handleManualStop(relay1Pin, timeUp, 1);
}

void handleManualStop(int relayPin, long int &time, int dir)
{
      // init variables
      manualMove = false;
      movingTime = 0;
      float movingSpeed = (dir == 1) ? upSpeed : downSpeed;

      // stop relay and calculate time of movement
      long int endTime = millis();
      digitalWrite(relayPin, HIGH);
      movingTime = endTime - time;
      time = 0;

      // calculate height by time and save
      currentHeight = (dir == 1) ? min(currentHeight + movingTime / 100.00 * movingSpeed / 10.00, maxHeight) : max(currentHeight - movingTime / 100.00 * movingSpeed / 10.00, minHeight);
      updateEepromHeight(eep_height_adr, currentHeight);
}

void setup()
{
      Serial.begin(9600);

      // read currentHeight, m1, m2 from memory
      EEPROM.get(eep_height_adr, currentHeight);
      Serial.println("EEPROM current height = " + String(currentHeight));
      EEPROM.get(eep_m1_adr, m1Height);
      Serial.println("EEPROM m1 height = " + String(m1Height));
      EEPROM.get(eep_m2_adr, m2Height);
      Serial.println("EEPROM m2 height = " + String(m2Height));

      // init buttons pullup mode
      pinMode(btnUpPin, INPUT_PULLUP);
      pinMode(btnM1Pin, INPUT_PULLUP);
      pinMode(btnSavePin, INPUT_PULLUP);
      pinMode(btnM2Pin, INPUT_PULLUP);
      pinMode(btnDownPin, INPUT_PULLUP);

      // init relay to open
      pinMode(relay1Pin, OUTPUT);
      pinMode(relay2Pin, OUTPUT);
      digitalWrite(relay1Pin, HIGH);
      digitalWrite(relay2Pin, HIGH);

      // initialize LCD and Print memory:
      lcd.begin(20, 4);
      printMem();
}

void loop()
{
      printSecRow("", 0, currentHeight);
      blinkIfNeeded();
      onLoopBtnClick();
      delay(50);
}