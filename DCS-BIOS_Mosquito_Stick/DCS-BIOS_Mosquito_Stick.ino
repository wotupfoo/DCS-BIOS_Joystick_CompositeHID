//#define DEBUG_SKETCH    // Uncomment to print out debug info on the Serial

#include <Arduino.h>

// ================================================================
// Arduino Library - USB Device Driver 
// https://github.com/arpruss/USBComposite_stm32f1
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
const HIDReportDescriptor jRD = {
  joystickReportDescriptor,         // report descriptor buffer
  sizeof(joystickReportDescriptor)  // report descriptor size
};
USBCompositeSerial CompositeSerial;
// There is no access to the Joystick report so keep a local copy
JoyReport_t report, lastReport;


// ================================================================
// Board Inputs
// ================================================================
// Analog Inputs
const int analogPins[] =     {PA1,  PA0,  PA2};   // Roll (x), Pitch (y), Brake (slider)
const bool analogInvert[] = {true, true, false};   // Reverse the axis direction?
const int analogPinCount = sizeof(analogPins) / sizeof(analogPins[0]);
float filteredValues[analogPinCount];
const float alpha = 0.15;
const int deadband = 2; // Ignore changes smaller than this to suppress noise floors
uint16_t analogValues[analogPinCount];   // Array of ADC values

// Digital Inputs (ACTIVE = LOW)
const int digitalPins[] = {PB9, PB8, PB7}; // Machine-Gun, 50mm Canon, Pickle(Bomb)
const int digitalPinCount = sizeof(digitalPins) / sizeof(digitalPins[0]);
#if (digitalPinCount > 8)   // The custom Joystick report has 32 buttons. 4 per input are needed -> 8 input max
#error Too many digital input pins. Limit of 8 digitalPins to drive 32 joystick buttons (4 per digital input)
#endif
bool digitalValues[digitalPinCount];      // Array of Digital Input values

// ================================================================
// Middleware - DCS-BIOS, MobiFligt, SimTool etc
// ================================================================
// DCS-BIOS
// My implmentation that adds CompositeSerial:: support to DCS-BIOS (this URL until it's upstreamed)
// A ZIP file of this repo is installed into the Arduino IDE instead of the original from DCS-Skunkworks
// https://github.com/wotupfoo/dcs-bios-arduino-library forked from DCS-Skunkworks/dcs-bios-arduino-library
#define DCSBIOS_USBCOMPOSITE_STM32F1_SERIAL // NEW: Functionality added into WotUpFoo fork in src/DcsBios.h
#include <DcsBios.h> // DCS World BIOS Class Rx/Tx over Serial (DcsBios::)

// ================================================================
// ADD YOUR DCS-BIOS DEVICES HERE
// ================================================================
// Flight controls are only sent/received over the Joystick HID device
// analogPins[0] Stick Roll (X)
// analogPins[1] Stick Pitch (Y)
// analogPins[2] Rudder (Z) (not implemented here)

// DH-89 Mosquito Stick
DcsBios::Potentiometer stickWheelBrk("STICK_WH_BRK", analogPins[2]); // Wheel brake lever

DcsBios::Switch2Pos stickBtnA("STICK_BTN_A", digitalPins[0]);       // Machine Gun Trigger
DcsBios::Switch2Pos stickBtnB1("STICK_BTN_B1", digitalPins[1]);     // Canon Trigger
DcsBios::Switch2Pos stickBtnB2("STICK_BTN_B2", digitalPins[2]);     // Pickle Trigger (Bombs, Drop tanks)
//DcsBios::Switch2Pos stickWhBrkLock("STICK_WH_BRK_LOCK", digitalPins[3]);    // Wheel brake lock (not implemented on physical stick)

// ================================================================
// YOU SHOULD NOT NEED TO CHANGE ANYTHING BELOW THIS LINE
// ================================================================

void setup() {
    // MIDDLEWARE SETUP
    // If you had a real USB registed company and product, you would
    // set it here:
    //USBComposite.setVendorId(0x1209);              // allocated VID
    //USBComposite.setProductId(0x0001);             // allocated PID
    //USBComposite.setManufacturerString("github wotupfoo");
    USBComposite.setProductString("Flight Stick");

    // Create a Serial port and whatever is in the reportDescription
    HID.begin(CompositeSerial, &jRD);
    USBComposite.begin();  
    while (!USBComposite);

    CustomJoystick.setManualReportMode(true);
    // HARDWARE SETUP
    for (int i = 0; i < analogPinCount; i++)
    {
        pinMode(analogPins[i], INPUT_ANALOG);
        CustomJoystick.invertAxis(i, analogInvert[i]); // Invert if needed
        // Init the filter and lastReport or it'll never stop changing
        int raw = analogRead(analogPins[i]);
        filteredValues[i] = raw;
        uint16_t currentVal = (uint16_t)filteredValues[i];
        // Windows joy.cpl seems to prefer 10bit (0..1023) vs 12bit (0..4095)
        analogValues[i] = currentVal >> 2; // 12bit to 10bit
        report.axis[i] = CustomJoystick.axis(i, analogValues[i]);
        lastReport.axis[i] = 0;     // This should trigger updates
    }

    report.buttons = 0;
    lastReport.buttons = 0;
    for (int i = 0; i < digitalPinCount; i++)
    {
        pinMode(digitalPins[i],INPUT_PULLUP);
    }
}

bool ANALOGchanged;
bool DIGITALchanged;
void loop()
{
    int analogDiff[analogPinCount];

    // Reset triggers
    ANALOGchanged = false;
    DIGITALchanged = false;

    // 0. Process DCS-BIOS
    DcsBios::loop();

    // 1. Process Analog with Change Detection
    for (int i = 0; i < analogPinCount; i++)
    {
        uint16_t currentVal;
        uint16_t joystickVal;

        int raw = analogRead(analogPins[i]);
        filteredValues[i] = (alpha * raw) + ((1.0 - alpha) * filteredValues[i]);
        currentVal = (uint16_t)filteredValues[i];
        // Windows joy.cpl seems to prefer 10bit (0..1023) vs 12bit (0..4095)
        analogValues[i] = currentVal >> 2; // 12bit to 10bit

        // Only change if it exceeds the noise deadband
        analogDiff[i] = abs((int)analogValues[i] - (int)lastReport.axis[i]);
        if (analogDiff[i] > deadband)
        {
            // .axis will return the value as-is or inverted if it is a reversed axis
            report.axis[i] = CustomJoystick.axis(i, analogValues[i]);
            ANALOGchanged = true;
        }
    }

    // 2. Process Buttons with Debounce and Change Detection
    CustomJoystick.buttons(0);  // Clear all 32 buttons to have clean slate
    report.buttons = 0;
    for (int i = 0; i < digitalPinCount; i++)
    {
        // Normal mode
        digitalValues[i] = !digitalRead(digitalPins[i]);  // Active LOW
        CustomJoystick.button(1 + i, digitalValues[i]);    // Buttons start at 1
        report.buttons |= digitalValues[i] << i;    // 0..31 bits
    }
    if (report.buttons != lastReport.buttons)
        DIGITALchanged = true;

    // 3. Conditional Send
    if (ANALOGchanged || DIGITALchanged)
    {
        // Send Joystick update to the PC
        CustomJoystick.send();
        // Save for comparision next time around
        lastReport.buttons = report.buttons;
        for (int i = 0; i < analogPinCount; i++)
        { 
            lastReport.axis[i] = report.axis[i]; 
        }
    }

    delay(5); // Fast polling, but 'changed' logic prevents USB flooding

#ifdef DEBUG_SKETCH
    static char buf[150];
    snprintf(buf,sizeof(buf), "Analog %u: Digital %u: Diff %04u:%04u:%04u Pitch %04u, Roll %04u, Brake %04u, Machine Gun %u, 50mm Canon %u, Pickle %u",
                        ANALOGchanged, DIGITALchanged, 
                        analogDiff[0], analogDiff[1], analogDiff[2],
                        analogValues[0], analogValues[1], analogValues[2],
                        digitalValues[0], digitalValues[1], digitalValues[2]
                        );
    CompositeSerial.println(buf);
#endif
}