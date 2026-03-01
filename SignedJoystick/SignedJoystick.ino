#include <USBComposite.h>
#include "HIDCustomJoystick.h"

USBHID HID;
HIDCustomJoystick joy(HID);      // must pass HID here

void setup() {
  HID.registerComponent();
  joy.begin();
  USBComposite.begin();
  while (!USBComposite) { }
}

void loop() {
  joy.buttons(1 | 2);
  joy.axis(0, 0);
  joy.axis(1, 0);
  joy.axis(2, 0);
  joy.axis(3, 0);
  joy.axis(4, 0);
  joy.axis(5, 0);
  joy.send(); 
  delay(500);
  joy.buttons(0);
  joy.axis(0, 1023);
  joy.axis(1, 1023);
  joy.axis(2, 1023);
  joy.axis(3, 1023);
  joy.axis(4, 1023);
  joy.axis(5, 1023);
  joy.send(); 
  delay(500);
}

