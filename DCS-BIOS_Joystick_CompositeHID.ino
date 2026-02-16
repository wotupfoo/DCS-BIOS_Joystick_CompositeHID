


/*
-- Example code from USBComposite_stmf1 github project making a composite device
USBHID HID; // create instance of USBHID plugin
HIDKeyboard Keyboard(HID); // create a profile
HIDJoystick Joystick1(HID); // create a profile
HIDJoystick Joystick2(HID); // create a profile
HIDMouse Mouse(HID); // create a profile

HID.begin();
*/

// ================================================================
// Device Drivers
// ================================================================
#include "USBHID.h"             // USB HID Interface (USBHID::)
//#include "USBComposite_stm32f1-master\USBHID.h"

#include "USBCompositeSerial.h" // USB HID CDC Sub-Interface (USBCompositeSerial::)
//#include "USBComposite_stm32f1-master\USBCompositeSerial.h"
#include "CustomJoystick_USB_Report.h"  // USB HID Custom 12bit Joystick Report

// Create HID Class
USBHID HID;
#if 0 // Select if you want a standard 8bit Joystick or a custom 12bit Joystick
// Create custom HID Endpoint using a custom Joystick report (the default is 8bit and this is 12 bit)
HIDReportAbs joystickReport(HID, reportDescription, sizeof(reportDescription), 1);

// Create CDC Virtual Com Port
USBCompositeSerial CompositeSerial;

const int analogPins[] = {PA0, PA1, PA2, PA3, PA4, PA5, PA6, PA7};
const int digitalPins[] = {PB0, PB1, PB10, PB11, PB12, PB13, PB14, PB15};
float filteredValues[8];
const float alpha = 0.15;
const int deadband = 4; // Ignore changes smaller than this to suppress noise floors

// ================================================================
// Application Class(es)
// ================================================================
// BEFORE incliding DcsBios.h, select one of the supported Serial port implementations
    // ATmegaU Polling based (Ardunio UNO etc) - not our board
    //#define DCSBIOS_STANDARD_SERIAL
    // ATmegaU IRQ based (Ardunio UNO etc) - not our board
    //#define DCSBIOS_IRQ_SERIAL
    // DCS-BIOS, use a custom serial driver
    //#define DCSBIOS_CUSTOM_SERIAL
    // DCS-BIOS, use the CompositeSerial (my implementation) serial driver
    #define DCSBIOS_USBCOMPOSITE_STM32F1_SERIAL
#include "DcsBios.h"            // DCS World BIOS Class Rx/Tx over Serial (DcsBios::)
#include "dcs-bios-arduino-library-master\src\DcsBios.h"
void dcsbiosSendChar(char c) { CompositeSerial.write(c); }

// ================================================================
// ARDUINO CODE
// ================================================================
void setup() {
    CompositeSerial.begin();
    HID.begin();
    setupDcsBios();
    for (int i = 0; i < 8; i++) {
        pinMode(analogPins[i], INPUT_ANALOG);
        pinMode(digitalPins[i], INPUT_PULLUP);
        filteredValues[i] = analogRead(analogPins[i]);
    }
}

void loop() {
    DcsBios::loop();
    bool changed = false;

    // 1. Process Analog with Change Detection
    for (int i = 0; i < 8; i++) {
        int raw = analogRead(analogPins[i]);
        filteredValues[i] = (alpha * raw) + ((1.0 - alpha) * filteredValues[i]);
        uint16_t currentVal = (uint16_t)filteredValues[i];

        // Only mark as changed if it exceeds the noise deadband
        if (abs((int)currentVal - (int)lastReport.axis[i]) > deadband) {
            report.axis[i] = currentVal;
            changed = true;
        } else {
            report.axis[i] = lastReport.axis[i]; // Keep stable
        }
    }

    // 2. Process Buttons with Change Detection
    report.buttons = 0;
    for (int i = 0; i < 8; i++) {
        if (digitalRead(digitalPins[i]) == LOW) {
            report.buttons |= (1 << i);
        }
    }
    if (report.buttons != lastReport.buttons) changed = true;

    // 3. Conditional Send
    if (changed) {
        joystickReport.send(&report, sizeof(report));
        lastReport = report; // Sync
    }

    delay(5); // Fast polling, but 'changed' logic prevents USB flooding
}




void loop() {
    DcsBios::loop();
    bool changed = false;

    // 1. Process Analog (Filtered & Deadband)
    for (int i = 0; i < 8; i++) {
        int raw = analogRead(analogPins[i]);
        filteredValues[i] = (alpha * raw) + ((1.0 - alpha) * filteredValues[i]);
        uint16_t currentVal = (uint16_t)filteredValues[i];

        if (abs((int)currentVal - (int)lastReport.axis[i]) > deadband) {
            report.axis[i] = currentVal;
            changed = true;
        } else {
            report.axis[i] = lastReport.axis[i];
        }
    }

    // 2. Process Digital Pins (Including Shared Logic for PB0)
    uint8_t currentButtons = 0;
    for (int i = 0; i < 8; i++) {
        if (digitalRead(digitalPins[i]) == LOW) {
            currentButtons |= (1 << i);
        }
    }
    report.buttons = currentButtons;

    // --- SHARED INPUT LOGIC (PB0 / Digital Pin 0) ---
    // Check if the state of PB0 (Button 1) has changed specifically
    bool pb0State = (report.buttons & 0x01);
    bool pb0LastState = (lastReport.buttons & 0x01);

    if (pb0State != pb0LastState) {
        changed = true; // Trigger HID update
        
        // If PB0 is pressed (LOW/True), send Gear Down to DCS-BIOS
        // If released, you could send "GEAR_LEVER 0" for Gear Up
        if (pb0State) {
            sendDcsBiosMessage("GEAR_LEVER 1\n");
        } else {
            sendDcsBiosMessage("GEAR_LEVER 0\n");
        }
    }
    // ------------------------------------------------

    // 3. Conditional HID Send
    if (currentButtons != lastReport.buttons) changed = true;

    if (changed) {
        joystickReport.send(&report, sizeof(report));
        lastReport = report;
    }

    delay(5);
}

// Helper to ensure DCS-BIOS strings go to our Composite Serial
void sendDcsBiosMessage(const char* msg) {
    CompositeSerial.print(msg);
}