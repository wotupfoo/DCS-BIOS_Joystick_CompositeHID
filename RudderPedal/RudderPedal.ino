#include <STM32ADC.h>
#include <USBComposite.h>
#include "HIDCustomJoystick.h"
#include <string.h>

STM32ADC adc(ADC1);
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
// All GPIO pins counter clockwise (not checked if using them is valid)
/*
const uint8 allPins[]={ PB12, PB13, PB14, PB15,
                        PA0,  PA1,  PA2,  PA3,  PA4,  PA5,  PA6, PA7,
                        PB0,  PB1,
                        PB10, PB11, PB12, PB13, PB14, PB15,
                        PA8,  PA9,  PA10, PA11, PA12, PA15,
                        PB3,  PB4,  PB5,  PB6,  PB7,  PA8,  PA9,
                        PC13, PC14, PC15 }; // ACTIVE = LOW
*/
/*  RESERVED
const uint8 ReservedPins[]={PA13, // JTAG_TMS/SWDIO
                            PA14, // JTAG_TCK/SWCLK
                            PB2,  // BOOT1
                            PB3}; // JTAG_TRACE/SWO (IF ENABLED) 
*/

// ================================================================
// Analog Inputs
/*  3.3v max ADC Channels
    PA0/ADC0, PA1/ADC1, PA2/ADC2, PA3/ADC4, 
    PA4/ADC4, PA5/ADC5, PA6/ADC6, PA7/ADC7, 
    PB0/ADC8, PB1,ADC9
*/
//const int analogPins[] = {PA0};
const int analogPins[] = {PA2};
//const int analogPins[] = {PA0, PA1, PA2};
//const int analogPins[] = {  PA0, PA1, PA2, PA3, PA4, PA5, PA6, PA7, PB0, PB1};
const int analogPinCount = sizeof(analogPins) / sizeof(analogPins[0]);
float filteredValues[analogPinCount];
const float alpha = 0.5; // 0.15; Filter attack speed
const int mapped_deadband = 10; // Ignore changes smaller than this to suppress noise floors

/*
// ================================================================
// Digital Inputs
const int digitalPins[] = { PB12, PB13, PB14, PB15,
                    // ADC  PA0,  PA1,  PA2,  PA3,  PA4,  PA5,  PA6, PA7,
                    // ADC  PB0,  PB1,
                            PB10, PB11, PB12, PB13, PB14, PB15,
                            PA8,  PA9,  PA10, PA11, PA12, PA15,
                            PB3,  PB4,  PB5,  PB6,  PB7,  PA8,  PA9,
                            PC13, PC14, PC15 }; // ACTIVE = LOW
const int digitalPins1[] = {PB12, PB13, PB14, PB15, PA8,  PA9,  PA10, PA11}; // ACTIVE = LOW
const int digitalPins2[] = {PA12, PA15, PB10, PB11}; // ACTIVE = LOW
const int digitalPinCount = sizeof(digitalPins) / sizeof(digitalPins[0]);
#if (digitalPinCount > 8)   // The custom Joystick report has 32 buttons. 4 per input are needed -> 8 input max
#error Too many digital input pins. Limit of 8 digitalPins to drive 32 joystick buttons (4 per digital input)
#endif
*/

// Assuming 0..1023 output range
#define AXIS_START_RANGE 10
uint16_t axis_raw_min[analogPinCount];
uint16_t axis_raw_max[analogPinCount];

void setup()
{
    // MIDDLEWARE SETUP
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
        axis_raw_min[i] = raw-1;
        axis_raw_max[i] = raw+1;
    }
    CompositeSerial.println("Starting");
    CompositeSerial.println("Vcc,Vhall,Percent");
}

const unsigned long loop_interval_hz = 100;  // Plenty for joystick, might not be for DCS-BIOS updates.
const unsigned long loop_interval_ms = (long)(1000/loop_interval_hz);
unsigned long previousMillis = 0;

const float Vref = 3.3;
const float Vstep = Vref/4096.0;

void loop()
{
    // 100Hz loop
    unsigned long currentMillis = millis();
    if (currentMillis - previousMillis >= loop_interval_ms)
    {
        previousMillis = currentMillis;

        bool changed = false;

#if 0
        //Not properly implemented in the library. Stub code is adc_read()*1.1
        float Vcc_adc = adc.readVcc();
        float Vcc_ratio = 3300.0/Vcc_adc;
        float Vcc = Vcc_adc / 1000.0;

        CompositeSerial.print("Vcc_adc=");
        CompositeSerial.print(Vcc_adc,0);
//        CompositeSerial.print(",");

        CompositeSerial.print(" Vcc_ratio=");
        CompositeSerial.print(Vcc_ratio);
//        CompositeSerial.print(",");

        CompositeSerial.print(" Vcc=");
        CompositeSerial.print(Vcc);
//        CompositeSerial.print(",");
#endif

// 1. Process Analog with Change Detection
        for (int i = 0; i < analogPinCount; i++)
        {
            // X-AXIS = RUDDER
            int raw = analogRead(analogPins[i]);
            // Center on 2.5v = 2048 so that it's in the middle of the hall sensor range
            // 3113 is measured value with 2.5v adc input, 5v Vcc on sensor
            //raw = raw * 2048.0 / 3113.0;
            filteredValues[i] = (alpha * raw) + ((1.0 - alpha) * filteredValues[i]);
            float Vhall = filteredValues[i];
            //CompositeSerial.print(" Vadc=");
            //CompositeSerial.print(filteredValues[i]);

#if 1
            //CompositeSerial.print(" ");
            //CompositeSerial.print("A");
            //CompositeSerial.print(i);
            uint16_t axis_filtered = (uint16_t)filteredValues[i];

            if(axis_filtered < axis_raw_min[i]) axis_raw_min[i] = axis_filtered;
            if(axis_filtered > axis_raw_max[i]) axis_raw_max[i] = axis_filtered;

            //CompositeSerial.print("[");
            //CompositeSerial.print(axis_raw_min[i]);
            //CompositeSerial.print("<");
            //CompositeSerial.print(filteredValues[i]);
            //CompositeSerial.print("<");
            //CompositeSerial.print(axis_raw_max[i]);

            // Spread range over the whole 10 bits
            uint16_t axis_mapped = map(axis_filtered, axis_raw_min[i], axis_raw_max[i], 0, 1023);
            //uint16_t axis_mapped = axis_filtered;

            //CompositeSerial.print("=");
            //CompositeSerial.print(axis_mapped);
            //CompositeSerial.print("]");

            // Only change if it exceeds the noise deadband
            if (abs((int)axis_mapped - (int)lastReport.axes[i]) > mapped_deadband) {
                //CompositeSerial.print("*");
                joy.axis(i, axis_mapped);
                lastReport.axes[i] = axis_mapped;
                changed = true;
            }
            else {
                //CompositeSerial.print(" ");
            }
            //CompositeSerial.print("\t : ");
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
        //CompositeSerial.println();
    }
}
