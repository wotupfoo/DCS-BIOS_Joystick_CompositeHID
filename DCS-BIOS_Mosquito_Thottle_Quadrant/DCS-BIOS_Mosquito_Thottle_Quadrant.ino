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
#define DCSBIOS_DISABLE_SERVO
#include <DcsBios.h> // DCS World BIOS Class Rx/Tx over Serial (DcsBios::)

// DH-89 Mosquito Throttle Quadrant
DcsBios::Potentiometer throttleControlL("THROTTLE_CONTROL_L", analogPins[0], true); // reverse=true sensor is backwards
DcsBios::Potentiometer throttleControlR("THROTTLE_CONTROL_R", analogPins[1]);

DcsBios::Potentiometer propControlL("PROP_CONTROL_L", analogPins[2], true); // reverse=true sensor is backwards
DcsBios::Potentiometer propControlR("PROP_CONTROL_R", analogPins[3]);

DcsBios::AnalogMultiPos mixture("MIXTURE", analogPins[4], 1);   // Hall Sensor (Analog) -> Digital

DcsBios::Switch2Pos rktFiringSw("RKT_FIRING_SW", digitalPins[0]);   // On the right throttle lever
DcsBios::Switch2Pos rktManBtn("RKT_MAN_BTN", digitalPins[1]);       // ???
DcsBios::Switch2Pos rktMasterSw("RKT_MASTER_SW", digitalPins[2]);   // Above Quadrant
DcsBios::Switch2Pos rktSalvoSw("RKT_SALVO_SW", digitalPins[3]);     // On dashboard

DcsBios::Switch2Pos supercharger("SUPERCHARGER", digitalPins[4]);

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