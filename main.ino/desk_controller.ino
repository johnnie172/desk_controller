#include <Wire.h>
#include <EEPROM.h>
#include <hd44780.h>
#include <hd44780ioClass/hd44780_I2Cexp.h> // include i/o class header

hd44780_I2Cexp lcd; // declare lcd object: auto locate & config display for hd44780 chip

const float minHeight = 740.00; // time to reach 17.4
const float maxHeight = 1240.5; // 19.95
float currentHeight;
float nextHeight;
const int upSpeed = 25; // 25mm per second
const int downSpeed = 29;

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

int relay1Pin = 9;
int relay2Pin = 10;

int eeprom_adr = 0;

bool move = false;

long int movingTime;
long int timeUp;
long int timeDown;

void printHeight(){
      float x = currentHeight;
      long val;
      float divider = 10.0;
      val = (long)(x * 10L); // val = 1234
      if (val > 10000) divider = 100.0
      else divider = 10.0
      x = (float)val / 10.0; // x = 1234 / 10.0 = 123.4
      lcd.setCursor(0, 1);
      lcd.print("                 ")
      lcd.print(x / 10);
      // TODO: round to 1 decimal
}

void goToHeight(float height)
{
      if (height > maxHeight || height < minHeight || height == currentHeight) return;
      if (currentHeight > height) {
            Serial.println("down to ");  Serial.print(height);
            goDown(height);
      } else {
            Serial.println("up to ");  Serial.print(height);
            goUp(height);
}
}

void goUp(float height)
{
      move = true;
      long int start = millis();
      int millisToMove = (height - currentHeight) / upSpeed * 1000;
      long int movingTime;
      digitalWrite(relay1Pin, LOW);
      while (millis() - start < millisToMove) {
            Serial.println(millis() - start);
            Serial.println(millisToMove);
            movingTime = millis() - start;
      }
      // TODO: can be less them min / max heights
      currentHeight = currentHeight + movingTime / 100 * upSpeed / 10;
      digitalWrite(relay1Pin, HIGH);
      printHeight();
}

void goDown(float height)
{
      move = true;
      long int start = millis();
      int millisToMove = (currentHeight - height) / downSpeed * 1000;
      long int movingTime;
      digitalWrite(relay2Pin, LOW);
      while (millis() - start < millisToMove) {
            Serial.println(millisToMove);
            movingTime = millis() - start;
      }
      currentHeight = currentHeight - movingTime / 100 * downSpeed / 10;
      digitalWrite(relay2Pin, HIGH);
      printHeight();
}

void up()
{
      move = true;
      digitalWrite(relay1Pin, LOW);
      if (timeUp == 0)
      {
            timeUp = millis();
      }
}

void down()
{
      move = true;
      digitalWrite(relay2Pin, LOW);
      if (timeDown == 0)
      {
            timeDown = millis();
      }
}

void none()
{
      move = false;

      if (timeDown > 0)
      {
            digitalWrite(relay2Pin, HIGH);
            long int t2 = millis();
            movingTime = t2 - timeDown;
            nextHeight = currentHeight - movingTime / 100 * downSpeed / 10;
            currentHeight = max(nextHeight, minHeight);
            timeDown = 0;
            movingTime = 0;
      }
      if (timeUp > 0)
      {
            digitalWrite(relay1Pin, HIGH);
            long int t2 = millis();
            movingTime = t2 - timeUp;
            nextHeight = currentHeight + movingTime / 100 * upSpeed / 10;
            currentHeight = min(nextHeight, maxHeight);
            timeUp = 0;
            movingTime = 0;
      }
      printHeight();
}

void setup()
{
      // get last height
      currentHeight = minHeight;
      //    int eeprom_value = EEPROM.read(eeprom_adr);
      //    EEPROM.update(address, value);

      Serial.begin(9600);

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

      // initialize LCD with number of columns and rows:
      lcd.begin(20, 4);

      // Print a message to the LCD
      lcd.print(" Johnnie  Desk!");
}

void loop()
{
      // read and print btns values
      button1State = !digitalRead(button1Pin); // green
      button2State = !digitalRead(button2Pin);
      button3State = !digitalRead(button3Pin);
      button4State = !digitalRead(button4Pin);
      button5State = !digitalRead(button5Pin); // red

      delay(50);

      // click on up
      if (button1State == 1)
      {
            up();
      }
      // click on down
      if (button5State == 1)
      {
            down();
      }

      if (button2State == 1)
      {
            goToHeight(760.00);
      }

      if (button3State == 1){
      long int start = millis();
      digitalWrite(relay2Pin, LOW);
      while (millis() - start > 1000) {
                  Serial.println(millis() - start);
            }
            digitalWrite(relay2Pin, HIGH);
      }

      if (button4State == 1)
      {
            goToHeight(1070.00);
      }

      // no button selected
      if (button1State == 0 && button5State == 0 && (button2State =! 1 || button4State != 1))
      {
            none();
      }
}
