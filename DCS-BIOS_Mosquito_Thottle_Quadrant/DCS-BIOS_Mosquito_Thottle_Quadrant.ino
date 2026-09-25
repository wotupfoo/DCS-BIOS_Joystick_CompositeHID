#include <Arduino.h>

#if defined(MCU_STM32F103C8)        // Bluepill 64k Flash 20k RAM
    // Needs to be th C6 or C8 size. Smaller won't fit.
    // Bluepill 64k Flash 20k RAM    Bluepill 32k Flash 10k RAM
    #if !defined(MCU_STM32F103C8) && !defined(MCU_STM32F103C6)
    #error "Unsupported-board: You need to use a Bluepill in the STM32F103C6 or C8 size"
    #endif
/*
 * Arduino Bluepill 64k (STM32F103C8) pin usage for this device:
 *
 *                                      +------+
 *                                 +----+ USBC +----+
 *         PIN_SUPERCHARGER PB12 --|    +______+    +-- GND  << USE THIS
 *                          PB13 --|                |-- GND  << USE THIS
 *                          PB14 --|       ..       |-- 3V3  << USE THIS FOR ADC
 *                          PB15 --|       ..       |-- nRST
 *                          PA8  --|       ..       |-- PB11     PIN_RKT_SALVO_SW
 *                          PA9  --|                |-- PB10     PIN_RKT_MASTER_SW
 *                          PA10 --|    BLUEPILL    |-- PB1/ADC9 PIN_RKT_MAN_BTN
 *                    USB-  PA11 --|  STM32F103C8   |-- PB0/ADC8 PIN_RKT_FIRING_SW
 *                    USB+  PA12 --|                |-- PA7/ADC7
 *                    JTDI  PA15 --|                |-- PA6/ADC6
 *                    JTDO  PB3  --|                |-- PA5/ADC5
 *                    JTRST PB4  --|                |-- PA4/ADC4 PIN_MIXTURE
 *                          PB5  --|                |-- PA3/ADC3 PIN_THROTTLE_PROP_CONTROL_R
 *                          PB6  --|                |-- PA2/ADC2 PIN_THROTTLE_PROP_CONTROL_L
 *                          PB7  --|                |-- PA1/ADC1 PIN_THROTTLE_CONTROL_R
 *                          PB8  --|                |-- PA0/ADC0 PIN_THROTTLE_CONTROL_L
 *                          PB9  --|                |-- PC15
 *                   USBIN  5V   --|                |-- PC14
 *                          GND  --|    +------+    |-- PC13
 *     *USE THIS FOR ADC >> 3V3  --|    | ISP  |    |-- VBAT
 *                                 +----+ |||| +----+
 *
 * NOTE -   The STM32 ONLY supports 3v3 for ADC. So choose hall sensors accordingly!
 *          The Authentikit MagHall sensor part is 5v. You need to use the equivalent
 *          higly sensitivity (most are 1/10th the sensitivity) magnetic hall effect device.
 *          Allegro A1319LUA-5-T 3.3v sensor - obsolete
 *          Allegro A1315LUA-5-T 3.3v sensor - replacement
 */

 // Analog inputs
#define PIN_THROTTLE_CONTROL_L      PA0
#define PIN_THROTTLE_CONTROL_R      PA1
#define PIN_THROTTLE_PROP_CONTROL_L PA2
#define PIN_THROTTLE_PROP_CONTROL_R PA3
#define PIN_MIXTURE                 PA4

// Digital inputs
#define PIN_RKT_FIRING_SW           PB0
#define PIN_RKT_MAN_BTN             PB1
#define PIN_RKT_MASTER_SW           PB10
#define PIN_RKT_SALVO_SW            PB11
#define PIN_SUPERCHARGER            PB12

// Set the digital input to check on bootup to go into Calibration Mode
#define CALIBRATION_MODE_BUTTON     PIN_RKT_MASTER_SW // Use the Rocket Fire Button on the Right Throttle

#elif defined(ARDUINO_BLUEPILL_F103C8)
    #error "You are building with the STM32duino Core. You must use the Roger Clark/Maple STM32 core"
#else
    #error "Unsupported board - Please use an Arduino STM32 Bluepill or implement your own"
#endif

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
const int analogPins[] = {  PIN_THROTTLE_CONTROL_L, PIN_THROTTLE_CONTROL_R,
                            PIN_THROTTLE_PROP_CONTROL_L, PIN_THROTTLE_PROP_CONTROL_R,
                            PIN_MIXTURE};
const int analogPinCount = sizeof(analogPins) / sizeof(analogPins[0]);
float filteredValues[analogPinCount];
const float alpha = 0.15;
const int deadband = 4; // Ignore changes smaller than this to suppress noise floors

// Digital Inputs (ACTIVE = LOW)
const int digitalPins[] = {PIN_RKT_FIRING_SW, PIN_RKT_MAN_BTN, PIN_RKT_MASTER_SW, PIN_RKT_SALVO_SW, PIN_SUPERCHARGER};
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

// ================================================================
// YOU SHOULD NOT NEED TO CHANGE ANYTHING BELOW THIS LINE
// ================================================================

EdgeLogicPins elp[digitalPinCount];

void setup() {
    // MIDDLEWARE SETUP
    // If you had a real USB registed company and product, you would
    // set it here:
    //USBComposite.setVendorId(0x1209);              // allocated VID
    //USBComposite.setProductId(0x0001);             // allocated PID
    //USBComposite.setManufacturerString("github wotupfoo");
    USBComposite.setProductString("Throtte Quadrant");  // Easier Identification vs 'maple'

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