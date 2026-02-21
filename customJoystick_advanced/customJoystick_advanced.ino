#include <Arduino.h>

// Input edge and debounce library
// 1 input -> Button[n+0,1,2] = [debounce (level), rise (pulse), fall (pulse)]
// We will map each digital input to 3 buttons, [debounced,rising,falling]
#include <EdgeLogic.h>

// ================================================================
// Arduino Library - USB Device Driver https://github.com/arpruss/USBComposite_stm32f1
// ================================================================
// Load the USB Composite driver that includes the USB Classes including:
// HID - Keyboard, Mouse, Joystick, Gamepad
// CDC - Virtual Communications Port
// Mass Storage Devivce, MIDI, Audio, Xbox360|XboxOne
#include <USBComposite.h>
USBCompositeSerial CompositeSerial;

// Custom Joystick layout - 32 buttons, 8 axis (10bit range, 16bit packing)
#include "HIDCustomJoystick.h"

// The first endpoint will be the CompositeSerial followed by whatever is 
// put in this array. Currently only a single custom configured joystick
const uint8 reportDescription[] = {
  HID_CUSTOM_JOYSTICK_REPORT_DESCRIPTOR()
};

USBHID HID;
HIDCustomJoystick CustomJoystick(HID);
CustomJoystickReport_t report, lastReport;

// ================================================================
// Board Inputs
// ================================================================
// Analog Inputs
const int analogPins[] = {PA0, PA1, PA2, PA3, PA4, PA5, PA6, PA7};
const int analogPinCount = sizeof(analogPins) / sizeof(analogPins[0]);
float filteredValues[analogPinCount];
const float alpha = 0.15;
const int deadband = 4; // Ignore changes smaller than this to suppress noise floors

// Digital Inputs
const int digitalPins[] = {PB0, PB1, PB10, PB11, PB12, PB13, PB14, PB15}; // ACTIVE = LOW
const int digitalPinCount = sizeof(digitalPins) / sizeof(digitalPins[0]);

EdgeLogicPins elp[digitalPinCount];

void setup() {
    // MIDDLEWARE SETUP
    // Create a Serial port and whatever is in the reportDescrition
    HID.begin(CompositeSerial, reportDescription, sizeof(reportDescription));
    USBComposite.begin();  
    while (!USBComposite);

    CustomJoystick.setManualReportMode(true);

    // HARDWARE SETUP
    for (int i = 0; i < analogPinCount; i++)
    {
        pinMode(analogPins[i], INPUT_ANALOG);
        filteredValues[i] = analogRead(analogPins[i]);
    }
    for (int i = 0; i < digitalPinCount; i++)
    {
        elp[i] = EdgeLogicPins(i,INPUT_PULLUP);
    }
}

void loop()
{
    bool changed = false;
    
    // 1. Process Analog with Change Detection
    for (int i = 0; i < analogPinCount; i++)
    {
        int raw = analogRead(analogPins[i]);
        filteredValues[i] = (alpha * raw) + ((1.0 - alpha) * filteredValues[i]);
        uint16_t currentVal = (uint16_t)filteredValues[i];

        // Only change if it exceeds the noise deadband
        if (abs((int)currentVal - (int)lastReport.axis[i]) > deadband)
        {
            CustomJoystick.axis(i, currentVal);
            changed = true;
        }
    }

    // 2. Process Buttons with Change Detection
    for (int i = 0; i < digitalPinCount; i++)
    {
        elp[i].loop();              // Update Logic
        int currentbutton = i*3;    // Debounce + HIGH + LOW = 3
        // Buttons are 1..32 so use "1 +" in front of the current button
        CustomJoystick.button(1 + currentbutton + 0, elp[i].read());   // Debounce
        CustomJoystick.button(1 + currentbutton + 1, elp[i].rose());   // HIGH pulse
        CustomJoystick.button(1 + currentbutton + 2, elp[i].fell());   // LOW pulse
    }
    if (report.buttons != lastReport.buttons)
        changed = true;

    // 3. Conditional Send
    if (changed)
    {
        CustomJoystick.send();
        lastReport.buttons = report.buttons;
        memcpy(lastReport.axis, report.axis, sizeof(report.axis)); // Sync
    }

    delay(5); // Fast polling, but 'changed' logic prevents USB flooding
}