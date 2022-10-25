#include <Wire.h>
#include <EEPROM.h>
#include <hd44780.h>
#include <hd44780ioClass/hd44780_I2Cexp.h> // include i/o class header

hd44780_I2Cexp lcd; // declare lcd object: auto locate & config display for hd44780 chip

unsigned long lastTimeBlink = 0;
long int timeToBlink = 3000;
String strings[2] = {"", "                "};
int blink = 0;
int stringsIndex = 0;

// table consts
const float minHeight = 741.00; // 74.1cm
const float maxHeight = 1247.00;
const float upSpeed = 25.5; // 25.5mm per second
const float downSpeed = 28.8;

float m1Height; // = 1198.00;
float m2Height; // = 770.00;

// init buttons pin
const int button1Pin = 2;
const int button2Pin = 3;
const int button3Pin = 4;
const int button4Pin = 5;
const int button5Pin = 6;

// init buttons state
int button1State = 0;
int button2State = 0;
int button3State = 0;
int button4State = 0;
int button5State = 0;

// relay pins
int relay1Pin = 9;
int relay2Pin = 10;

// EEPROM memory addresses
int eep_height_adr = 0;
int eep_m1_adr = 4;
int eep_m2_adr = 7;

// movement variables
bool move = false;
long int movingTime;
long int timeUp;
long int timeDown;
float currentHeight;
float nextHeight;

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

bool checkButtonPress()
{
      if (
          !digitalRead(button1Pin) ||
          !digitalRead(button2Pin) ||
          !digitalRead(button3Pin) ||
          !digitalRead(button4Pin) ||
          !digitalRead(button5Pin))
            return true;
      return false;
}

void onLoopBtnClick()
{
      // read and print btns values
      button1State = !digitalRead(button1Pin); // green
      button2State = !digitalRead(button2Pin);
      button3State = !digitalRead(button3Pin);
      button4State = !digitalRead(button4Pin);
      button5State = !digitalRead(button5Pin); // red

      // click on up
      if (button1State == 1)
      {
            manualUp();
      }
      // click on down
      if (button5State == 1)
      {
            manualDown();
      }

      // click on m1
      if (button2State == 1)
      {
            goToHeight(m1Height);
      }

      // click on save
      if (button3State == 1)
      {
            saveToMem();
      }

      // click on m2
      if (button4State == 1)
      {
            goToHeight(m2Height);
      }

      // no button selected and no auto move
      if (button1State == 0 && button5State == 0 && (button2State = !1 || button4State != 1))
      {
            manualMoveStopped();
      }
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
      lcd.print("M1 " + heightToString(m1Height) + " " + "M2 " + heightToString(m2Height));
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
      while (millis() - start < 5000)
      {
            blinkIfNeeded(4500, 400);
            if (digitalRead(button2Pin) == 0)
            {
                  m1Height = currentHeight;
                  updateEepromHeight(eep_m1_adr, m1Height);
                  Serial.println(m1Height);
                  break;
            }
            if (digitalRead(button4Pin) == 0)
            {
                  m2Height = currentHeight;
                  updateEepromHeight(eep_m2_adr, m2Height);
                  Serial.println(m2Height);
                  break;
            }
            if (digitalRead(button3Pin) == 0)
                  break;
      }
      printMem();
}

void goToHeight(float height)
{
      // do nothing if we close to the destination or not in the mx/min values
      if (height > maxHeight || height < minHeight || (currentHeight - 3 <= height && height <= currentHeight + 3))
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
      long int movingTime;

      // start timer and open relay
      move = true;
      digitalWrite(relayPin, LOW);
      long int start = millis();
      delay(250);

      // sleep until we reached time or interrupted
      while (millis() - start < millisToMove)
      {
            movingTime = millis() - start;
            if (checkButtonPress())
                  break;
      }

      // close relay and update current height
      move = false;
      digitalWrite(relayPin, HIGH);
      currentHeight = (dir == 1) ? currentHeight + movingTime / 100 * upSpeed / 10 : currentHeight - movingTime / 100 * downSpeed / 10;
      delay(250);
}

void manualUp()
{
      move = true;
      digitalWrite(relay1Pin, LOW);
      if (timeUp == 0)
      {
            timeUp = millis();
      }
}

void manualDown()
{
      move = true;
      digitalWrite(relay2Pin, LOW);
      if (timeDown == 0)
      {
            timeDown = millis();
      }
}

void manualMoveStopped()
{
      long int endTime;
      move = false;
      if (timeDown > 0 || timeUp > 0)
      {
            endTime = millis();
            movingTime = 0;
            updateEepromHeight(eep_height_adr, currentHeight);
      }
      if (timeDown > 0)
      {
            digitalWrite(relay2Pin, HIGH);
            movingTime = endTime - timeDown;
            nextHeight = currentHeight - movingTime / 100 * downSpeed / 10;
            currentHeight = max(nextHeight, minHeight);
            timeDown = 0;
      }
      if (timeUp > 0)
      {
            digitalWrite(relay1Pin, HIGH);
            movingTime = endTime - timeUp;
            nextHeight = currentHeight + movingTime / 100 * upSpeed / 10;
            currentHeight = min(nextHeight, maxHeight);
            timeUp = 0;
      }
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
      pinMode(button1Pin, INPUT_PULLUP);
      pinMode(button2Pin, INPUT_PULLUP);
      pinMode(button3Pin, INPUT_PULLUP);
      pinMode(button4Pin, INPUT_PULLUP);
      pinMode(button5Pin, INPUT_PULLUP);

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
