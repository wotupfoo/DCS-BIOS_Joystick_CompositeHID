#include <USBComposite.h>
#include "HIDReadable.h"

static const uint8_t joystickReportDescriptor[] = {
  // Usage Page (Generic Desktop), Usage (Joystick), Collection (Application)
  HID_USAGE_PAGE, HID_USAGE_PAGE_GENERIC_DESKTOP,
  HID_USAGE, 0x04,                               // Joystick
  HID_COLLECTION, HID_COLLECTION_APPLICATION,

    HID_REPORT_ID, HID_JOYSTICK_REPORT_ID,

    // -------------------------
    // 32 Buttons (4 bytes)
    // -------------------------
    HID_USAGE_PAGE, HID_USAGE_PAGE_BUTTON,
    HID_USAGE_MIN, 0x01,
    HID_USAGE_MAX, 0x20,
    HID_LOGICAL_MIN, 0x00,
    HID_LOGICAL_MAX, 0x01,
    HID_REPORT_SIZE, 0x01,
    HID_REPORT_COUNT, 0x20,
    HID_INPUT, HID_INPUT_DATA_VAR_ABS,

    // -------------------------
    // 6 Axes (16-bit each)
    // -------------------------
    HID_USAGE_PAGE, HID_USAGE_PAGE_GENERIC_DESKTOP,
    HID_LOGICAL_MIN, 0x00,
    HID_LOGICAL_MAX_16, 0xFF, 0x03,              // 0..1023 in a 16-bit field
    HID_REPORT_SIZE, 0x10,
    HID_REPORT_COUNT, 0x06,

      HID_USAGE, HID_GD_X,
      HID_USAGE, HID_GD_Y,
      HID_USAGE, HID_GD_Z,
      HID_USAGE, HID_GD_RX,
      HID_USAGE, HID_GD_RY,
      HID_USAGE, HID_GD_RZ,

    HID_INPUT, HID_INPUT_DATA_VAR_ABS,

    // -------------------------
    // 2 Sliders (16-bit each)
    // -------------------------
    HID_LOGICAL_MIN, 0x00,
    HID_LOGICAL_MAX_16, 0xFF, 0x03,
    HID_REPORT_SIZE, 0x10,
    HID_REPORT_COUNT, 0x02,

      HID_USAGE, HID_GD_SLIDER,
      HID_USAGE, HID_GD_DIAL,

    HID_INPUT, HID_INPUT_DATA_VAR_ABS,

  HID_END_COLLECTION
};

HIDReportDescriptor jRD = {joystickReportDescriptor,sizeof(joystickReportDescriptor)};

typedef struct __attribute__((packed, aligned(1))) {
    uint8_t  reportID;
    uint32_t buttons;
    union {
      uint16_t axes[6];
      struct {
        uint16_t x;
        uint16_t y;
        uint16_t z;
        uint16_t rx;
        uint16_t ry;
        uint16_t rz;
      } stick;
    };
    union {
      uint16_t non_sticks[2];
      struct {
        uint16_t slider;
        uint16_t dial;
      } non_stick;
    };
} JoyReport_t;


USBHID HID;

JoyReport_t joyReport;
/*
HIDReporter(USBHID& _HID, 
            const HIDReportDescriptor* r, 
            uint8_t* _buffer, 
            unsigned _size, 
            uint8_t _reportID, 
            bool forceReportID=false);
*/
HIDReporter joy(HID,
                &jRD,
                (uint8_t*)&joyReport,
                sizeof(joyReport),
                HID_JOYSTICK_REPORT_ID);


void setup() {
  // Serial.begin(115200);
  // delay(1000);
  // Serial.print("sizeof(JoyReport_t)=");
  // Serial.println(sizeof(JoyReport_t));
  // while(1);
  HID.registerComponent();
  USBComposite.begin();  
  while (!USBComposite);
}

void loop() {
  joyReport.buttons = 1 | 2;
  joyReport.stick.x = 0;
  joyReport.stick.y = 0;
  joyReport.stick.z = 0;
  joyReport.stick.rx = 0;
  joyReport.stick.ry = 0; 
  joyReport.stick.rz = 0; 
  joyReport.non_stick.slider = 0;
  joyReport.non_stick.dial = 0;
  joy.sendReport(); 
  delay(500);
  joyReport.buttons = 0;
  joyReport.stick.x = 1023;
  joyReport.stick.y = 1023;
  joyReport.stick.z = 1023;
  joyReport.stick.rx = 1023;
  joyReport.stick.ry = 1023;  
  joyReport.stick.rz = 1023; 
  joyReport.non_stick.slider = 1023;
  joyReport.non_stick.dial = 1023;
  joy.sendReport(); 
  delay(500);
}

