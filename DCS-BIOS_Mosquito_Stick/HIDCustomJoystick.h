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
  // 6 axis (16-bit each)
  // -------------------------
  HID_USAGE_PAGE, HID_USAGE_PAGE_GENERIC_DESKTOP,
  HID_LOGICAL_MIN, 0x00,
  HID_LOGICAL_MAX_16, 0xFF, 0x03,              // 0..1023 in a 16-bit field
  HID_REPORT_SIZE, 0x10,
  HID_REPORT_COUNT, 0x03,       // 3 Axis

    HID_USAGE, HID_GD_X,
    HID_USAGE, HID_GD_Y,
//    HID_USAGE, HID_GD_Z,
//    HID_USAGE, HID_GD_RX,
//    HID_USAGE, HID_GD_RY,
//    HID_USAGE, HID_GD_RZ,
    HID_USAGE, HID_GD_SLIDER,
//    HID_USAGE, HID_GD_DIAL,

  HID_INPUT, HID_INPUT_DATA_VAR_ABS,

HID_END_COLLECTION
};
extern const HIDReportDescriptor jRD;

typedef struct __attribute__((packed, aligned(1))) {
  uint8_t  reportID;
  uint32_t buttons;
  union {
    uint16_t axis[3];
    struct {
      uint16_t x;   // Roll
      uint16_t y;   // Pitch
//      uint16_t z;
//      uint16_t rx;
//      uint16_t ry;
//      uint16_t rz;
      uint16_t slider;  // Brake
//      uint16_t dial;  
    } stick;
  };
} JoyReport_t;

//================================================================================
//================================================================================
//	Custom Joystick Class wrapper of HIDReporter
class HIDCustomJoystick : public HIDReporter
{
protected:
  JoyReport_t joyReport;
  void safeSendReport(void);
  bool manualReport = false;
  const uint8_t num_axis = (uint8_t)(sizeof(joyReport.axis)/sizeof(joyReport.axis[0]));
  const uint8_t num_buttons = 32;
public:
  // Constructor
  HIDCustomJoystick(USBHID& HID, uint8_t reportID = HID_JOYSTICK_REPORT_ID)
    : HIDReporter(HID, &jRD, (uint8_t*)&joyReport, sizeof(joyReport), reportID) {
      joyReport.buttons = 0;
      for (uint8_t i = 0; i < num_axis; i++)
        joyReport.axis[i] = 0;
    }

  inline void send(void)
  {
    sendReport();
  }
  void setManualReportMode(bool manualReport); // in manual report mode, report only sent when send() is called
  bool getManualReportMode();
  void begin(void);
  void end(void);
  void button(uint8_t button, bool val);
  void buttons(uint32_t b);
  void axis(uint8_t analog, uint16_t val);
  uint8_t getNumAxis() { return num_axis; }
  uint8_t getNumButtons() { return num_buttons; };
};
