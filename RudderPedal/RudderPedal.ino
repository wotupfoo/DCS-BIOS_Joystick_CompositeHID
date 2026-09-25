//#define DEBUG_SKETCH    // Uncomment to print out debug info on the Serial

#include <Arduino.h>
#include <STM32ADC.h>
#include <USBComposite.h>
#include "HIDCustomJoystick.h"
#include <string.h>

#if defined(ARDUINO_GENERIC_STM32F103C) 
    // Bluepill 64k Flash 20k RAM    Bluepill 32k Flash 10k RAM
    #if !defined(MCU_STM32F103C8) && !defined(MCU_STM32F103C6)
    #warning "Unsupported-board: You may need to use a Bluepill in the STM32F103C6 or C8 size"
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
 *                    USB+  PA12 --|                |-- PA7/ADC7    PIN_RUDDER
 *                    JTDI  PA15 --|                |-- PA6/ADC6
 *                    JTDO  PB3  --|                |-- PA5/ADC5
 *                    JTRST PB4  --|                |-- PA4/ADC4
 *                          PB5  --|                |-- PA3/ADC3
 *                          PB6  --|                |-- PA2/ADC2
 *                          PB7  --|                |-- PA1/ADC1
 *                          PB8  --|                |-- PA0/ADC0
 *                          PB9  --|                |-- PC15
 *                    USBIN 5V   --|                |-- PC14
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
#define PIN_RUDDER      PA7
#elif defined(ARDUINO_BLUEPILL_F103C8)
    #error "You are building with the STM32duino Core. You must use the Roger Clark/Maple STM32 core. The board type is Arduino_STM32:STM32F1:genericSTM32F103C:upload_method=STLinkMethod"
#else
    #error "Unsupported board - Please use an Arduino STM32 Bluepill or implement your own"
#endif


STM32ADC adc(ADC1); // direct hardware control of the ADC vs using the Arduino library
USBHID HID;
HIDCustomJoystick joy(HID); // must pass HID here
const HIDReportDescriptor jRD = {
    joystickReportDescriptor,        // report descriptor buffer
    sizeof(joystickReportDescriptor) // report descriptor size
};
USBCompositeSerial CompositeSerial;
JoyReport_t report, lastReport;

// ================================================================
// Board Inputs
// ================================================================
const int analogPins[] = {PIN_RUDDER};
const int analogPinCount = sizeof(analogPins) / sizeof(analogPins[0]);
float filteredValues[analogPinCount];
const float alpha = 0.25; // 0.15; Filter attack speed
const int mapped_deadband = 3; // Ignore changes smaller than this to suppress noise floors

void setup()
{
    // MIDDLEWARE SETUP
    //USBComposite.setVendorId(0x1209);              // allocated VID
    //USBComposite.setProductId(0x0001);             // allocated PID
    //USBComposite.setManufacturerString("github wotupfoo");
    USBComposite.setProductString("Rudder");  // Easier Identification vs 'maple'

   HID.begin(CompositeSerial, &jRD);
    USBComposite.begin();
    while (!USBComposite)
    {
    }

    joy.setManualReportMode(true);

    delay(3000);    // Let the ADC warm up to not get bogus numbers
    // HARDWARE SETUP
    // Attempting to smooth out noise on the ADC
    adc.calibrate();
    adc.setSampleRate(ADC_SMPR_239_5);  // Slow the ADC sampler charge ciruit to 239.5 cycles

    for (int i = 0; i < analogPinCount; i++)
    {
        pinMode(analogPins[i], INPUT_ANALOG);
        uint16_t raw = analogRead(analogPins[i]);
        filteredValues[i] = raw;
    }
    CompositeSerial.println("Starting");
}

const unsigned long loop_interval_hz = 100;  // Plenty for joystick, might not be for DCS-BIOS updates.
const unsigned long loop_interval_ms = (long)(1000/loop_interval_hz);
unsigned long previousMillis = 0;

void loop()
{
    // 100Hz loop
    unsigned long currentMillis = millis();
    if (currentMillis - previousMillis >= loop_interval_ms)
    {
        previousMillis = currentMillis;

        bool changed = false;

// 1. Process Analog with Change Detection
        for (int i = 0; i < analogPinCount; i++)
        {
            // X-AXIS = RUDDER
            const int raw_zero = 2048 + 30;
            int raw = analogRead(analogPins[i]) - raw_zero;
            filteredValues[i] = (alpha * raw) + ((1.0 - alpha) * filteredValues[i]);
            int16_t intFiltered = (int16_t)filteredValues[i];

            const int16_t axis_gain = 11;
            int16_t axis = axis_gain*intFiltered;   // Amplify around the center
            axis = max(-2048, axis);
            axis = min(axis, 2047);
            uint16_t axis_mapped = (uint16_t)(axis + 2048);   // Move to center (2048)
            // Windows joy.cpl seems to prefer 10bit (0..1023) vs 12bit (0..4095)
            axis_mapped = axis_mapped >> 2; // 12bit to 10bit
            int delta = abs((int)axis_mapped - (int)lastReport.axis[i]);

#if defined(DEBUG_SKETCH)
            // Putting prints in the deadband test so it's no flooding the serial
            CompositeSerial.print("raw=");
            CompositeSerial.print(raw);
            CompositeSerial.print(" filtered=");
            CompositeSerial.print(intFiltered);
            CompositeSerial.print(" axis=");
            CompositeSerial.print(axis);
            CompositeSerial.print(" axis_mapped=");
            CompositeSerial.print(axis_mapped);
            CompositeSerial.print(" last");
            CompositeSerial.print((int)lastReport.axis[i]);
            CompositeSerial.print(" delta=");
            CompositeSerial.print(delta);
#endif
            // Only change if it exceeds the noise deadband
            if (delta > mapped_deadband) {
#if defined(DEBUG_SKETCH)
                CompositeSerial.print("*");
#endif
                joy.axis(i, axis_mapped);
                lastReport.axis[i] = axis_mapped;
                changed = true;
            }
#if defined(DEBUG_SKETCH)
            if(i < analogPinCount-1) {
                CompositeSerial.print("\t : ");
            }
#endif
        }
        // 2. Process Digital

        // 3. Conditional Send
        if (changed == true)
        {
            joy.send();
            lastReport.buttons = report.buttons;
        }
//        CompositeSerial.print("\r");
        CompositeSerial.println();
    }
}
