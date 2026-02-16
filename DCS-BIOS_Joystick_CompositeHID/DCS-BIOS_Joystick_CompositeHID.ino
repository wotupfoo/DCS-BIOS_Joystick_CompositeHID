// ================================================================
// Arduino Library - USB Device Driver https://github.com/arpruss/USBComposite_stm32f1
// ================================================================
// Load the USB Composite driver that includes the USB Classes including:
// HID - Keyboard, Mouse, Joystick, Gamepad
// CDC - Virtual Communications Port
// Mass Storage Devivce, MIDI, Audio, Xbox360|XboxOne
#include "USBComposite.h"

USBHID HID;                         // Create USB HID Base Class
USBCompositeSerial CompositeSerial; // Create CDC Virtual Com Port
HIDJoystick Joystick(HID);          // Create HID Joystick
/*
// ================================================================
// Uncomment if you want the Joystick values to change from 10 bits to 9..16 bits
//#define JOYSTICK_REPORT_10BIT_ANALOG   // 10 bit = 0..1023 (default)
#define JOYSTICK_REPORT_12BIT_ANALOG     // 12 bit = 0..4095
//#define JOYSTICK_REPORT_16BIT_ANALOG   // 16 bit = 0..65535
#define JOYSTICK_REPORT_BUTTONS 8
#define JOYSTICK_REPORT_AXIS 8
// Uncomment this if you want each digital input to have a complementary pair of buttons
// Useful if you need to have an ON and OFF event presented as two ON events
#define CUSTOMJOYSTICK_USB_REPORT_COMPLEMENTARY_BUTTONS
// Create a custom Joystick Report based on the number of digital inputs and analog inputs (and their bit depth)
#include "CustomJoystick_USB_Report.h"
//HIDReporter CustomJoystick(CompositeSerial, HID, reportDescription, 1);
*/

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
const int digitalPins[] = {PB0, PB1, PB10, PB11, PB12, PB13, PB14, PB15};
const int digitalPinCount = sizeof(digitalPins) / sizeof(digitalPins[0]);

struct JoystickReport
{
    uint16_t axis[analogPinCount];
    uint32_t buttons; // Supports up to 32 buttons stored in bits
} report, lastReport;

// ================================================================
// Application Class(es)
// ================================================================
// BEFORE incliding DcsBios.h, select one of the supported Serial port implementations
#define DCSBIOS_DEFAULT_SERIAL // (default) Arduino Serial Class (most generic, polls)
// #define DCSBIOS_IRQ_SERIAL            // ATmegaU IRQ based (Ardunio UNO etc)

// NEW TO DCS-BIOS - use the CompositeSerial (my implementation) serial driver
// #define DCSBIOS_USBCOMPOSITE_STM32F1_SERIAL
#ifdef DCSBIOS_USBCOMPOSITE_STM32F1_SERIAL
// Helpers to ensure DCS-BIOS strings go to our Composite Serial
void dcsbiosSendChar(char c) { CompositeSerial.write(c); }
void sendDcsBiosMessage(const char *msg) { CompositeSerial.print(msg); }
#endif
#include "DcsBios.h" // DCS World BIOS Class Rx/Tx over Serial (DcsBios::)

// ================================================================
// ARDUINO CODE
// ================================================================
void setup()
{
    /*
        HID.begin(CompositeSerial,
                    Joystick_8x_16bit_analog_and_8_digitalpins_USB_reportDescription,
                    sizeof(Joystick_8x_16bit_analog_and_8_digitalpins_USB_reportDescription));
    */
    HID.begin(CompositeSerial, HID_JOYSTICK);
    while (!USBComposite)
        ;
    Joystick.setManualReportMode(true);

    DcsBios::setup();
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

        // Only mark as changed if it exceeds the noise deadband
        if (abs((int)currentVal - (int)lastReport.axis[i]) > deadband)
        {
            report.axis[i] = currentVal;
            changed = true;
        }
        else
        {
            report.axis[i] = lastReport.axis[i]; // Keep stable
        }
    }

    // 2. Process Buttons with Change Detection
    report.buttons = 0;
    for (int i = 0; i < digitalPinCount; i++)
    {
        if (digitalRead(digitalPins[i]) == LOW)
        {
            report.buttons |= (1 << i);
        }
    }
    if (report.buttons != lastReport.buttons)
        changed = true;

    // 3. Conditional Send
    if (changed)
    {
        Joystick.send(&report, sizeof(report));
        lastReport = report; // Sync
    }

    delay(5); // Fast polling, but 'changed' logic prevents USB flooding
}