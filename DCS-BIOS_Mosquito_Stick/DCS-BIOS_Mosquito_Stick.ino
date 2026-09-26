//#define DEBUG_SKETCH    // Uncomment to print out debug info on the Serial
#include <Arduino.h>

#if defined(ARDUINO_GENERIC_STM32F103C) 
    //  Bluepill 128k Flash 20k RAM  Bluepill 64k Flash 20k RAM   Bluepill 32k Flash 10k RAM
    #if !defined(MCU_STM32F103CB) && !defined(MCU_STM32F103C8) && !defined(MCU_STM32F103C6)
    #error "Unsupported-board: You need to use a Bluepill in the STM32F103C6, C8 or CB size"
    #endif
/*
 * Arduino Bluepill (STM32F103C6/STM32F103C8) pin usage for this device:
 *
 *                                      +------+
 *                                 +----+ USBC +----+
 *                          PB12 --|    +______+    +-- GND  << USE THIS
 *                          PB13 --|                |-- GND  << USE THIS
 *                          PB14 --|       ..       |-- 3V3  << USE THIS FOR ADC
 *                          PB15 --|       ..       |-- nRST
 *                          PA8  --|       ..       |-- PB11
 *                          PA9  --|                |-- PB10
 *                          PA10 --|    BLUEPILL    |-- PB1/ADC9
 *                    USB-  PA11 --|  STM32F103C8   |-- PB0/ADC8
 *                    USB+  PA12 --|                |-- PA7/ADC7
 *                    JTDI  PA15 --|                |-- PA6/ADC6
 *                    JTDO  PB3  --|                |-- PA5/ADC5
 *                    JTRST PB4  --|                |-- PA4/ADC4
 *                          PB5  --|                |-- PA3/ADC3 [OPTIONAL] PIN_RUDDER
 *                          PB6  --|                |-- PA2/ADC2 PIN_WHEEL_BRAKE
 *               PIN_PICKLE PB7  --|                |-- PA1/ADC1 PIN_ROLL
 *               PIN_GUN    PB8  --|                |-- PA0/ADC0 PIN_PITCH
 *               PIN_CANON  PB9  --|                |-- PC15
 *                    USBIN 5V   --|                |-- PC14
 *                          GND  --|    +------+    |-- PC13/ONBOARD LED
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
#define PIN_PITCH       PA0
#define PIN_ROLL        PA1
#define PIN_WHEEL_BRAKE PA2
//#define PIN_RUDDER      PA3 // This is not in the current build but could be added

// Digital inputs
#define PIN_GUN         PB8
#define PIN_CANON       PB9
#define PIN_PICKLE      PB7

// Set the digital input to check on bootup to go into Calibration Mode
#define CALIBRATION_MODE_BUTTON         PIN_GUN // Use the Machine Gun Button

#elif defined(ARDUINO_BLUEPILL_F103C8)
    #error "You are building with the STM32duino Core. You must use the Roger Clark/Maple STM32 core. The board type is Arduino_STM32:STM32F1:genericSTM32F103C:upload_method=STLinkMethod"
#else
    #error "Unsupported board - Please use an Arduino STM32 Bluepill or implement your own"
#endif

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
const int analogPins[] =    {PIN_ROLL,  PIN_PITCH,  PIN_WHEEL_BRAKE};
const int WheelBrakeAnalogIndex = 2;    // Needed to manipulate the value due to the lock
const bool analogInvert[] = {true,      true,       false};   // Reverse the axis direction?
const int analogPinCount = sizeof(analogPins) / sizeof(analogPins[0]);
float filteredValues[analogPinCount];
const float alpha = 0.15;
const int deadband = 2; // Ignore changes smaller than this to suppress noise floors
uint16_t analogValues[analogPinCount];   // Array of ADC values

// Digital Inputs (ACTIVE = LOW)
const int digitalPins[] = {PIN_GUN, PIN_CANON, PIN_PICKLE}; // Machine-Gun, 50mm Canon, Pickle(Bomb)
const int digitalPinCount = sizeof(digitalPins) / sizeof(digitalPins[0]);
#if (digitalPinCount > 8)   // The custom Joystick report has 32 buttons. 4 per input are needed -> 8 input max
#error Too many digital input pins. Limit of 8 digitalPins to drive 32 joystick buttons (4 per digital input)
#endif
bool digitalValues[digitalPinCount];      // Array of Digital Input values

// Digital Outputs
// just the onboard LED on PC13

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
// analogPins[3] Rudder (Z) (not implemented here)

// DH-89 Mosquito Stick
//DcsBios::Potentiometer stickWheelBrk("STICK_WH_BRK", PIN_WHEEL_BRAKE); // Wheel brake lever (JOY_SLIDER)
DcsBios::Potentiometer stickWheelBrk("STICK_WH_BRK",        // Wheel brake lever (JOY_SLIDER)
                                        PIN_WHEEL_BRAKE,    // PIN
                                        false,    // Inverted
                                        500,      // Minimum value
                                        1023);    // Maximum value

DcsBios::Switch2Pos stickBtnA ("STICK_BTN_A",  PIN_GUN);        // Machine Gun Trigger (JOY_BTN1)
DcsBios::Switch2Pos stickBtnB1("STICK_BTN_B1", PIN_CANON);      // Canon Trigger (JOY_BTN2)
DcsBios::Switch2Pos stickBtnB2("STICK_BTN_B2", PIN_PICKLE);     // Pickle Trigger (Bombs, Drop tanks) (JOY_BTN3)
//DcsBios::Switch2Pos stickWhBrkLock("STICK_WH_BRK_LOCK", digitalPins[3]);    // Wheel brake lock (not implemented on physical stick)

// ================================================================
// YOU SHOULD NOT NEED TO CHANGE ANYTHING BELOW THIS LINE
// ================================================================

#if 0   // DISABLED : WE CAN'T GET THE LOCK STATE FROM THE GAME
volatile bool BrakeLockActive = false;
volatile uint16_t  BrakeLeverOutput;
// If the Wheel Brake Lock has been pressed and if the brake lever is >75% (49,152), 
// DCS will let the lock hold the lever. Below 75% brake lever, pressing the lock will 
/// have no effect and it will not be animated as depressed. The physical brake lever 
// value should not be allowed to go below 75% if the lock is active to match the animation.
// Above 80% will automatically release the lock in game btw.
// ================================================================
// Wheel Brake Lever information from game
void onStickWhBrkChange(unsigned int newValue) 
{
    unsigned int newValue1k = newValue >> 6; // 0..64k -> 0..1k
    BrakeLeverOutput = (uint16_t)newValue1k;
}
DcsBios::IntegerBuffer stickWhBrkBuffer(
    Mosquito_STICK_WH_BRK,   // DCS-BIOS Channel
    onStickWhBrkChange       // Called when the value changes
);

// ================================================================
// Wheel Brake Lock information from game
// This is NOT the game state of the pin and spring pressed
// It is the current value of any input
// eg For keyboard input it is if AltL+'t' is pressed.
void onStickWhBrkLockChange(unsigned int newValue) 
{
    BrakeLockActive = (bool)(newValue);   // 0 or 1 from DCS-BIOS
}
DcsBios::IntegerBuffer stickWhBrkLockBuffer(
    Mosquito_STICK_WH_BRK_LOCK, // DCS-BIOS Channel
    onStickWhBrkLockChange      // Called when the value changes
);
#endif

// ======================================================================
// HELPER ROUTINES
// ======================================================================

// ======================================================================
// Debounced button helper
bool buttonPressedDebounced(byte pin, bool level) {
    bool i,j;
    i = digitalRead(pin);
    delay(25);
    j = digitalRead(pin);
    return (i == level && j == level);
}

// A flag to run in Calibration Mode or Normal Mode
bool calibrationMode;

void led(void) {
    digitalWrite(PC13,false);
    delay(100);
    digitalWrite(PC13,true);
}

// ======================================================================
// SETUP
// ======================================================================
void setup() {
    // MIDDLEWARE SETUP
    // If you had a real USB registed company and product, you would
    // set it here:
    //USBComposite.setVendorId(0x1209);              // allocated VID
    //USBComposite.setProductId(0x0001);             // allocated PID
    //USBComposite.setManufacturerString("github wotupfoo");
    //USBComposite.setProductString("-Mosquito Stick-");  // Easier Identification vs 'maple'

    // Create/Register a Serial port and custom reportDescription device (joystick)
    HID.begin(CompositeSerial, &jRD);
    //USBComposite.begin();   // Done during HID.begin()
    while (!USBComposite);  // Make sure it is ready

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
    CompositeSerial.println("Starting: Custom Joystick + DCS-BIOS Serial Port");

    DcsBios::setup();   // For STM32F103Cx this does nothing

    pinMode(PC13, OUTPUT);  // LED_BUILTIN (Active LOW) - Green
    // Blink a few times to indicate it's starting up
    for(int i=0; i<10; i++) 
    {
        led();
        delay(500);
    }
}

// ======================================================================
// LOOP
// ======================================================================
bool ANALOGchanged;
bool DIGITALchanged;
void loop()
{
    int analogDiff[analogPinCount];

    // Reset triggers
    ANALOGchanged = false;
    DIGITALchanged = false;

    // 0. Process DCS-BIOS items
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

    // 2b. Process Virtual Buttons that are derived from other inputs
    // Update when the analog changes signficantly to reduce thrash.
    if (ANALOGchanged)
    {
#if 0        
        // !!!! DISABLED. THERE IS NO WAY TO KNOW FROM THE GAME IF THE LOCKING PIN
        //      IS IN OR OUT. SO WE CAN'T KNOW WHEN TO USE THE GAME VALUE OR THE
        //      REAL VALUE. FOR NOW WE'LL IGNORE THE GAME AND ALWAYS SEND THE VALUE

        // If the Wheel Brake Lock is active, in-game the Wheel Break must be at 
        // 75% or higher. Let DCS output values determine what we send back for 
        // the brake lever as a closed loop. If the lever on the physical stick has
        // the lock engaged we currently can't detect that, but we can hold the
        // brake lever in the game at that value, which is effecively the same as
        // having it implemented.
        // To release the brake automatically, values higher than 80% will automatically 
        // release the lock. It's also enough to release the real lock on the real
        // stick (hopefully).
        uint16_t brakelever = max(BrakeLeverOutput, report.axis[WheelBrakeAnalogIndex]);
        // In this case it's ok to put it through the CustomJoystick.axis because it is 
        // not an inverted input. If it were, we would have a problem.
        report.axis[WheelBrakeAnalogIndex] = CustomJoystick.axis(WheelBrakeAnalogIndex, brakelever);
#endif
    }

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