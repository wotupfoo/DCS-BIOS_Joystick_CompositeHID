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

USBHID HID;
HIDCustomJoystick CustomJoystick(HID);

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

struct CustomJoystickReport
{
    uint32_t buttons; // Supports up to 32 buttons stored in bits
    uint16_t axis[analogPinCount];
} report, lastReport;
size_t reportSize = 0;

// ================================================================
// Middleware - DCS-BIOS, MobiFligt, SimTool etc
// ================================================================
// DCS-BIOS
// My implmentation that adds CompositeSerial:: support to DCS-BIOS (this URL until it's upstreamed)
// A ZIP file of this repo is installed into the Arduino IDE instead of the original from DCS-Skunkworks
// https://github.com/wotupfoo/dcs-bios-arduino-library forked from DCS-Skunkworks/dcs-bios-arduino-library
#define DCSBIOS_USBCOMPOSITE_STM32F1_SERIAL // NEW: Functionality added into WotUpFoo fork in src/DcsBios.h
#ifdef DCSBIOS_USBCOMPOSITE_STM32F1_SERIAL
// Helpers to ensure DCS-BIOS strings go to our Composite Serial
void dcsbiosSendChar(char c) { CompositeSerial.write(c); }
void sendDcsBiosMessage(const char *msg) { CompositeSerial.print(msg); }
#endif

#include "DcsBios.h" // DCS World BIOS Class Rx/Tx over Serial (DcsBios::)

void setup()
{
    reportSize = sizeof(report);
    HID.begin(HID_JOYSTICK);
    while (!USBComposite)
        ;

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