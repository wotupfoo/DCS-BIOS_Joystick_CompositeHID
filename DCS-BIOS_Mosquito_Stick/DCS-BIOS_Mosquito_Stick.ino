#include <Arduino.h>
// ================================================================
// Arduino Library - USB Device Driver https://github.com/arpruss/USBComposite_stm32f1
// ================================================================
// Load the USB Composite driver that includes the USB Classes including:
// HID - Keyboard, Mouse, Joystick, Gamepad
// CDC - Virtual Communications Port
// Mass Storage Devivce, MIDI, Audio, Xbox360|XboxOne
#include <USBComposite.h>

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
USBCompositeSerial CompositeSerial;

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

// ================================================================
// Middleware - DCS-BIOS, MobiFligt, SimTool etc
// ================================================================
// DCS-BIOS
// My implmentation that adds CompositeSerial:: support to DCS-BIOS (this URL until it's upstreamed)
// A ZIP file of this repo is installed into the Arduino IDE instead of the original from DCS-Skunkworks
// https://github.com/wotupfoo/dcs-bios-arduino-library forked from DCS-Skunkworks/dcs-bios-arduino-library
#define DCSBIOS_USBCOMPOSITE_STM32F1_SERIAL // NEW: Functionality added into WotUpFoo fork in src/DcsBios.h
//#define DCSBIOS_DISABLE_SERVO
#include <DcsBios.h> // DCS World BIOS Class Rx/Tx over Serial (DcsBios::)

// Flight controls are only sent/received over the Joystick HID device
// analogPins[0] Stick Pitch
// analogPins[1] Stick Roll
// analogPins[2] Rudder Yaw

// DH-89 Mosquito Stick
DcsBios::Switch2Pos stickBtnA("STICK_BTN_A", digitalPins[0]);       // Machine Gun Trigger
DcsBios::Switch2Pos stickBtnB1("STICK_BTN_B1", digitalPins[1]);     // Cannon Trigger
DcsBios::Switch2Pos stickBtnB2("STICK_BTN_B2", digitalPins[2]);     // Ordinance Trigger (Bombs, Drop tanks)
DcsBios::Switch2Pos stickWhBrkLock("STICK_WH_BRK_LOCK", digitalPins[3]);    // Wheel brake lock
DcsBios::Potentiometer stickWhBrk("STICK_WH_BRK", analogPins[3]);           // Wheel brake lever

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
        pinMode(digitalPins[i], INPUT_PULLUP);
    }
}

void loop()
{
    DcsBios::loop();
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
        if (digitalRead(digitalPins[i]) == LOW)
        {
            CustomJoystick.button(i + 1, 1); // Buttons are 1..32 so use i+1
        }
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