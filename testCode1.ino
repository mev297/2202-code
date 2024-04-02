#include <Arduino.h>
#include <Adafruit_NeoPixel.h>
#include <Wire.h>
#include <SPI.h>
#include "Adafruit_TCS34725.h"

#define SERVO_PIN_1 9 // Define the pin connected to the first servo motor
#define SERVO_PIN_2 10 // Define the pin connected to the second servo motor
#define SERVO_MIN_PULSE_WIDTH 544 // Minimum pulse width in microseconds
#define SERVO_MAX_PULSE_WIDTH 2400 // Maximum pulse width in microseconds
#define SERVO_MIN_ANGLE 0 // Minimum angle in degrees
#define SERVO_MAX_ANGLE 180 // Maximum angle in degrees

// Function declarations
void buttonISR(void* arg);
void timerISR();
long degreesToDutyCycle(int deg);

// Button structure
struct Button {
  const int pin;
  unsigned int numberPresses;
  unsigned long lastPressTime;
  bool pressed;
};

// Constants
const int cTCSLED            = 14;
const int cStepPin           = 40;
const int cDirPin            = 39;
const long cDebounceDelay    = 20;

// Variables
unsigned int robotIndex      = 0;
Button button                = {0, 0, 0, false};
boolean stepDir              = true;
volatile int32_t stepCount   = 0;
boolean stepperRunState      = false;
Adafruit_TCS34725 tcs        = Adafruit_TCS34725(TCS34725_INTEGRATIONTIME_2_4MS, TCS34725_GAIN_4X);
bool tcsFlag                 = true;
int pos1 = 0; // Variable to store the position of the first servo motor
int pos2 = 0; // Variable to store the position of the second servo motor

hw_timer_t * pTimer; // Declare pTimer

void setup() {
  pinMode(SERVO_PIN_1, OUTPUT); // Set the first servo pin as an output
  pinMode(SERVO_PIN_2, OUTPUT); // Set the second servo pin as an output
  // Initialize timer
  pTimer = timerBegin(0, 80, true); // Timer 0, prescaler 80, count up
  timerAttachInterrupt(pTimer, &timerISR, true); // Attach timer interrupt
  timerAlarmWrite(pTimer, 1000, true); // 1 ms period
  timerAlarmEnable(pTimer); // Enable the timer
}

void loop() {
  uint16_t r, g, b, c; // RGBC values from TCS34725

  switch(robotIndex) {
    case 0: //arm is up
      //Bot.toPosition
      break;

    case 1: //robot detecting colour
      digitalWrite(cTCSLED, !digitalRead(cLEDSwitch)); // turn on onboard LED if switch state is low (on position)
      if (tcsFlag) { // if colour sensor initialized
        tcs.getRawData(&r, &g, &b, &c); // get raw RGBC values
        // Print RGB values if required
        Serial.printf("R: %d, G: %d, B: %d, C %d\n", r, g, b, c);
      }
      // if statement for detecting the colours
      if (g > r || g == r) {
        break;
      }
      // Additional code if needed
      break;

    case 2: //rotate arm down with servo 1
      for (pos1 = SERVO_MIN_ANGLE; pos1 <= 60; pos1++) {
        int pulseWidth1 = map(pos1, SERVO_MIN_ANGLE, SERVO_MAX_ANGLE, SERVO_MIN_PULSE_WIDTH, SERVO_MAX_PULSE_WIDTH);
        analogWrite(SERVO_PIN_1, pulseWidth1); // Send the pulse width to the first servo
        delay(15); // Delay for smooth movement (adjust as needed)
      }
      break;

    case 3: //stepper rotates 1.5 rev
      stepperRunState = true; // Start the stepper motor
      stepDir = true; // Set the direction
      digitalWrite(cDirPin, stepDir); // Set direction pin
      break;

    case 4: //arm goes back up using servo 1
      for (pos1 = 60; pos1 >= SERVO_MIN_ANGLE; pos1--) {
        int pulseWidth1 = map(pos1, SERVO_MIN_ANGLE, SERVO_MAX_ANGLE, SERVO_MIN_PULSE_WIDTH, SERVO_MAX_PULSE_WIDTH);
        analogWrite(SERVO_PIN_1, pulseWidth1); // Send the pulse width to the first servo
        delay(15); // Delay for smooth movement (adjust as needed)
      }
      break;

    case 5: //servo 2 rotates more (in same direction as before) to bring to mounted container
      for (pos2 = SERVO_MIN_ANGLE; pos2 <= SERVO_MAX_ANGLE; pos2++) {
        int pulseWidth2 = map(pos2, SERVO_MIN_ANGLE, SERVO_MAX_ANGLE, SERVO_MIN_PULSE_WIDTH, SERVO_MAX_PULSE_WIDTH);
        analogWrite(SERVO_PIN_2, pulseWidth2); // Send the pulse width to the second servo
        delay(15); // Delay for smooth movement (adjust as needed)
      }
      break;

    case 6: //stepper rotates in the opposite direction to release the stone
      stepperRunState = true; // Start the stepper motor
      stepDir = false; // Reverse the direction
      digitalWrite(cDirPin, stepDir); // Set direction pin
      break;

    case 7: //servo 2 rotates more (in same direction as before) to bring to mounted container
      for (pos2 = SERVO_MAX_ANGLE; pos2 >= SERVO_MIN_ANGLE; pos2--) {
        int pulseWidth2 = map(pos2, SERVO_MIN_ANGLE, SERVO_MAX_ANGLE, SERVO_MIN_PULSE_WIDTH, SERVO_MAX_PULSE_WIDTH);
        analogWrite(SERVO_PIN_2, pulseWidth2); // Send the pulse width to the second servo
        delay(15); // Delay for smooth movement (adjust as needed)
      }
      break;

    default:
      // Handle invalid robotIndex value
      break;
  }
}

void buttonISR(void* arg) {
  Button* s = static_cast<Button*>(arg);
  uint32_t pressTime = millis();
  if (pressTime - s->lastPressTime > cDebounceDelay) {
    s->numberPresses += 1;
    s->pressed = true;
    s->lastPressTime = pressTime;
  }
}

void timerISR() {
  if (stepperRunState) {
    digitalWrite(cStepPin, !digitalRead(cStepPin));
    if (stepDir) {
      stepCount++;
    } else {
      stepCount--;
    }
  }
}

long degreesToDutyCycle(int deg) {
  const long cMinDutyCycle = 400;
  const long cMaxDutyCycle = 2100;
  long dutyCycle = map(deg, 0, 180, cMinDutyCycle, cMaxDutyCycle);
  return dutyCycle;
}
