#include <USBComposite.h>
#include "HIDCustomJoystick.h"

USBHID HID;
HIDCustomJoystick joy(HID);      // must pass HID here
const HIDReportDescriptor jRD = {
  joystickReportDescriptor,         // report descriptor buffer
  sizeof(joystickReportDescriptor)  // report descriptor size
};
USBCompositeSerial CompositeSerial;

void setup() {
  HID.begin(CompositeSerial, &jRD);
  USBComposite.begin();
  while (!USBComposite) { }
}

void loop() {
  CompositeSerial.print("Updating button");
  for( uint8_t i = 1; i < joy.getNumButtons()+1; i++ ) {
    CompositeSerial.print(i);
    CompositeSerial.print(" ");
    joy.button(i, false);
  }
  CompositeSerial.println();
  for( uint8_t i = 0; i < joy.getNumAxis(); i++ ) {
    CompositeSerial.print("Updating axis");
    CompositeSerial.println(i);
    joy.axis(i, 0);
  }
  joy.send(); 
  delay(500);

  joy.buttons(0xFFFFFFFF);
  for( uint8_t i = 0; i < joy.getNumAxis(); i++ ) {
    joy.axis(i, 1023);
  }
  joy.send();
  CompositeSerial.println("I am a customJoystick and Composite Serial device using OpenComposite_STM32F1 library and Arudino!");
  CompositeSerial.println("I show up at a Joystick device (see joy.cpl Windows Control Panel app) and open a serial terminal on the COM port to see!");
  delay(500);
}

