#include <Arduino.h>
#include <USBComposite.h>

USBHID HID;
HIDJoystick Joystick(HID);

// ================================================================
// Board Inputs
// ================================================================
// Analog Inputs
const int analogPins[] = {PA0, PA1, PA2, PA3, PA4, PA5};
const int analogPinCount = sizeof(analogPins)/sizeof(analogPins[0]);

// Digital Inputs
const int digitalPins[] = {PB0, PB1, PB10, PB11, PB12, PB13, PB14, PB15};
const int digitalPinCount = sizeof(digitalPins)/sizeof(digitalPins[0]);

struct JoystickReport {
    uint32_t buttons;   // Supports up to 32 buttons stored in bits
    uint16_t axis[analogPinCount];
} report;


// ================================================================
// Arduino Setup
// ================================================================
void setup() {
  HID.begin(HID_JOYSTICK);
  while (!USBComposite);
  Joystick.setManualReportMode(true);

  for (int i = 0; i < analogPinCount; i++) {
      pinMode(analogPins[i], INPUT_ANALOG);
  }
  for (int i = 0; i < digitalPinCount; i++) {
      pinMode(digitalPins[i], INPUT_PULLUP);
  }
}

// ================================================================
// Arduino Loop
// ================================================================
void loop() {
    Joystick.X(analogRead(analogPins[0]));
    Joystick.Y(analogRead(analogPins[1]));
    Joystick.Xrotate(analogRead(analogPins[2]));
    Joystick.Yrotate(analogRead(analogPins[3]));
    Joystick.sliderLeft(analogRead(analogPins[4]));
    Joystick.sliderRight(analogRead(analogPins[5]));

   // 2. Process Buttons with Change Detection
    report.buttons = 0;
    for (int i = 0; i < digitalPinCount; i++) {
        if (digitalRead(digitalPins[i]) == LOW) {
            report.buttons |= (1 << i);
        }
    }
    Joystick.buttons(report.buttons);
    Joystick.send();

    delay(20); // Basic 50Hz update
}
