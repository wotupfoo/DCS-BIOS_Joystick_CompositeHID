#pragma once
#define CUSTOMJOYSTICK_USB_REPORT_H

/*
EXAMPLE CODE [WORK IN PROGRESS]
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



// AWESOME Video by Ben Eater on YouTube on how USB works
// How does USB device discovery work?
// https://www.youtube.com/watch?v=N0O5Uwc3C0o

// Uncomment this if you want each digital input to have a complementary pair of buttons
// Useful if you need to have an ON and OFF event presented as two ON events
// #define CUSTOMJOYSTICK_USB_REPORT_COMPLEMENTARY_BUTTONS

// Choose one of these for the analog range
// #define JOYSTICK_REPORT_10BIT_ANALOG   // 10 bit = 0..1023 (default)
// #define JOYSTICK_REPORT_12BIT_ANALOG   // 12 bit = 0..4095
// #define JOYSTICK_REPORT_16BIT_ANALOG   // 16 bit = 0..65535

#if !defined(JOYSTICK_REPORT_BUTTONS)
#error Define JOYSTICK_REPORT_BUTTONS to set the number of buttons (0..32). Even if unused it must be set to 0.
#endif
#if (JOYSTICK_REPORT_BUTTONS > 32)
#error JOYSTICK_REPORT_BUTTONS must be between 0 and 32
#endif

#if !defined(JOYSTICK_REPORT_AXIS)
#error Define JOYSTICK_REPORT_AXIS to set the number of axis
#endif
#if !(JOYSTICK_REPORT_AXIS == 8)
#error JOYSTICK_REPORT_AXIS: Only 8 Analog Inputs are supported at the moment
#endif

const uint8_t reportDescription[] = {
    0x05, 0x01, // USAGE_PAGE (Generic Desktop)
    0x09, 0x04, // USAGE (Joystick)
                // Start of Application Collection
    0xa1, 0x01, // COLLECTION (Application)
    0x85, 0x01, //   REPORT_ID (1)
                // Analog outputs - Collection of JOYSTICK_REPORT_AXIS inputs
    0x05, 0x01, //   USAGE_PAGE (Generic Desktop)
    0x09, 0x01, //   USAGE (Pointer)
    0xa1, 0x00, //   COLLECTION (Physical)
                // HARD CODED START - THIS IS HARD CODED TO 8 ANALOG INPUTS FOR NOW
    0x09, 0x30, //     USAGE (X)
    0x09, 0x31, //     USAGE (Y)
    0x09, 0x32, //     USAGE (Z)
    0x09, 0x33, //     USAGE (Rx)
    0x09, 0x34, //     USAGE (Ry)
    0x09, 0x35, //     USAGE (Rz)
    0x09, 0x36, //     USAGE (Slider)
    0x09, 0x37, //     USAGE (Dial)
                // HARD CODED END
    0x15, 0x00, //     LOGICAL_MINIMUM (0)
#if defined(JOYSTICK_10BIT_ANALOG_REPORTING)
    0x26, 0xff, 0x03, //     LOGICAL_MAXIMUM (1023) - 10bit range (LSB, MSB)
#elif defined(JOYSTICK_12BIT_ANALOG_REPORTING)
    0x26,
    0xff,
    0x0f, //     LOGICAL_MAXIMUM (4095) - 12bit range (LSB, MSB)
#elif defined(JOYSTICK_16BIT_ANALOG_REPORTING)
    0x26,
    0xff,
    0xff, //     LOGICAL_MAXIMUM (65535) - 16bit range (LSB, MSB)
#endif
    0x75, 0x10,                 //     REPORT_SIZE (16 bits per value)
    0x95, JOYSTICK_REPORT_AXIS, //     REPORT_COUNT (JOYSTICK_REPORT_AXIS)
    0x81, 0x02,                 //     INPUT (Data,Var,Abs)
    0xc0,                       //   END_COLLECTION

#if (defined(JOYSTICK_REPORT_BUTTONS) & JOYSTICK_REPORT_BUTTONS > 0) // ONLY ADD THE BUTTONS IN THE REPORT IF THERE ARE ANY
                                                                     // Digital outputs - single value in report that is bit stuffed
    0x05, 0x09,                                                      //   USAGE_PAGE (Button)
    0x19, 0x01,                                                      //   USAGE_MINIMUM (Button 1)
#ifdef CUSTOMJOYSTICK_USB_REPORT_COMPLEMENTARY_BUTTONS
    0x29, 2 * JOYSTICK_REPORT_BUTTONS, //   USAGE_MAXIMUM
#else
    0x29, JOYSTICK_REPORT_BUTTONS, //   0x08 USAGE_MAXIMUM (Button8)
#endif
    0x15, 0x00, //   LOGICAL_MINIMUM (0)
    0x25, 0x01, //   LOGICAL_MAXIMUM (1)
    0x75, 0x01, //   0x01 REPORT_SIZE (1 bit per report)
#ifdef CUSTOMJOYSTICK_USB_REPORT_COMPLEMENTARY_BUTTONS
    0x95, 2 * JOYSTICK_REPORT_BUTTONS, //   0x10 REPORT_COUNT (number of (single bit) reports)
#else
    0x95, JOYSTICK_REPORT_BUTTONS, //   0x08 REPORT_COUNT (number of (single bit) reports)
#endif
    0x81, 0x02, //   INPUT (Data,Var,Abs)
    0xc0        // END_COLLECTION
#endif          // ifdef JOYSTICK_REPORT_BUTTONS
    // End of Application Collection
};

#endif // CUSTOMJOYSTICK_USB_REPORT_H