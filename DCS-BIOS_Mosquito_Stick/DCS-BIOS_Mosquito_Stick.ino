#include <Arduino.h>

// Input edge and debounce library
// https://github.com/WotUpFoo/EdgeLogic
// 1 input -> Button[n+0,1,2] = [debounce (level), inverted debounce (level), rise (pulse), fall (pulse)]
// We will map each digital input to 4 buttons, [debounced,inverteddebounced,rising,falling]
#include <EdgeLogic.h>

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
JoyReport_t report, lastReport;


// ================================================================
// Board Inputs
// ================================================================
// Analog Inputs
const int analogPins[] = {PA0, PA1, /*PA2,*/ PA3};   // Roll (x), Pitch (y), [Rudder (z)], Brake (slider)
const int analogPinCount = sizeof(analogPins) / sizeof(analogPins[0]);
float filteredValues[analogPinCount];
const float alpha = 0.15;
const int deadband = 4; // Ignore changes smaller than this to suppress noise floors

// Digital Inputs (ACTIVE = LOW)
const int digitalPins[] = {PB13, PB14, PB15}; // Machine-Gun, 50mm Cannon, Pickle(Bomb)
const int digitalPinCount = sizeof(digitalPins) / sizeof(digitalPins[0]);
#if (digitalPinCount > 8)   // The custom Joystick report has 32 buttons. 4 per input are needed -> 8 input max
#error Too many digital input pins. Limit of 8 digitalPins to drive 32 joystick buttons (4 per digital input)
#endif

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
DcsBios::Potentiometer stickWheelBrk("STICK_WH_BRK", analogPins[3]); // Wheel brake lever

DcsBios::Switch2Pos stickBtnA("STICK_BTN_A", digitalPins[0]);       // Machine Gun Trigger
DcsBios::Switch2Pos stickBtnB1("STICK_BTN_B1", digitalPins[1]);     // Cannon Trigger
DcsBios::Switch2Pos stickBtnB2("STICK_BTN_B2", digitalPins[2]);     // Pickle Trigger (Bombs, Drop tanks)
//DcsBios::Switch2Pos stickWhBrkLock("STICK_WH_BRK_LOCK", digitalPins[3]);    // Wheel brake lock (not implemented on physical stick)

// ================================================================
// YOU SHOULD NOT NEED TO CHANGE ANYTHING BELOW THIS LINE
// ================================================================

EdgeLogicPins elp[digitalPinCount];

void setup() {
    // MIDDLEWARE SETUP
    // Create a Serial port and whatever is in the reportDescrition
    HID.begin(CompositeSerial, &jRD);
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

        // 2. Process Buttons with Debounce and Change Detection
    // Each input pin drives 4 joystick buttons:
    //  Debounced
    //  Inverted Debounced (handy if the switch is electrically backwards)
    //  Debounced Rising Edge pulse ("ON" pulse)
    //  Debounced Falling Edge pulse ("OFF" pulse)
    for (int i = 0; i < digitalPinCount; i++)
    {
        elp[i].loop(); // Update Logic
        EdgeLogicPins::outputstates_t outputstates = elp->getOutputState();
        int currentbuttongroup = i*4;     // Debounced + InvertedDebounced + HighPulse + LowPulse = 4
        // Buttons are 1..32 so use "1 +" in front of the current button
        CustomJoystick.button(1 + currentbuttongroup + 0, outputstates.Debounced);
        CustomJoystick.button(1 + currentbuttongroup + 1, outputstates.InvertedDebounced);
        CustomJoystick.button(1 + currentbuttongroup + 2, outputstates.HighPulse);
        CustomJoystick.button(1 + currentbuttongroup + 3, outputstates.LowPulse);
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