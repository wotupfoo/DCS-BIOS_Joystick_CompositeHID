#include <USBComposite.h>

// AWESOME Video by Ben Eater on YouTube on how USB works
// "How does USB device discovery work?"
// https://www.youtube.com/watch?v=N0O5Uwc3C0o

// only works for little-endian machines, but makes the code so much more
// readable

/* Symmantic report layout
Joystick (Application)
 ├─ ReportID (HID_CUSTOM_JOYSTICK_REPORT_ID) 1-byte
 ├─ Buttons (device-level) 4-bytes
 └─ Pointer (Physical control - the moving parts on the joystick) 8x 2-bytes
	  ├─ X
	  ├─ Y
	  ├─ Z
	  ├─ Rx
	  ├─ Ry
	  ├─ Rz
	  ├─ Slider1
	  └─ Slider2
*/
// 8 AXIS inputs packed into 16 bits using 10-bit range 0..1023
// It is easier to setup the report if it's byte aligned.
#define HID_CUSTOM_JOYSTICK_REPORT_DESCRIPTOR(...)                                                                                        \
	0x05, 0x01,																			/*  Usage Page (Generic Desktop) */               \
		0x09, 0x04,																		/*  Usage (Joystick) */                           \
		0xA1, 0x01,																		/*  Collection (Application) */                   \
		0x85, MACRO_GET_ARGUMENT_1_WITH_DEFAULT(HID_JOYSTICK_REPORT_ID, ##__VA_ARGS__), /*    REPORT_ID */                                \
		0x15, 0x00,																		/* 	  Logical Minimum (0) */                      \
		0x25, 0x01,																		/*    Logical Maximum (1) */                      \
		0x75, 0x01,																		/*    Report Size (1) */                          \
		0x95, 0x20,																		/*    Report Count (32) -- stored in uint32_t */  \
		0x05, 0x09,																		/*    Usage Page (Button) */                      \
		0x19, 0x01,																		/*    Usage Minimum (Button #1) */                \
		0x29, 0x20,																		/*    Usage Maximum (Button #32) */               \
		0x81, 0x02,																		/*    Input (variable,absolute) */                \
		0x05, 0x01,																		/* Usage Page (Generic Desktop) */                \
		0x09, 0x01,																		/* Usage (Pointer) */                             \
		0xA1, 0x00,																		/* Collection (Physical) */                       \
		0x09, 0x30,																		/*    Usage (X) */                                \
		0x09, 0x31,																		/*    Usage (Y) */                                \
		0x09, 0x32,																		/*    Usage (Z) */                                \
		0x09, 0x33,																		/*    Usage (Rx) */                               \
		0x09, 0x34,																		/*    Usage (Ry) */                               \
		0x09, 0x35,																		/*    Usage (Rz) */                               \
		0x09, 0x36,																		/*    Usage (Slider) */                           \
		0x09, 0x36,																		/*    Usage (Slider) */                           \
		0x15, 0x00,																		/*    Logical Minimum (0) */                      \
		0x26, 0xFF, 0x03,																/*    Logical Maximum (1023) */                   \
		0x75, 0x10,																		/*    Report Size (16) */                         \
		0x95, 0x08,																		/*    Report Count (8) 8 analog in uint16_t[8] */ \
		0x81, 0x02,																		/*    Input (variable,absolute) */                \
		0xC0,																			/*  End Collection */                             \
		MACRO_ARGUMENT_2_TO_END(__VA_ARGS__) 0xC0

typedef struct
{
	uint32_t buttons; // 32 buttons bit stuffed into uint32_t
	uint16_t axis[8]; // 8 analogs stored in uint16_t. Allowable values are 0..1023
} __packed CustomJoystickReport_t;

extern const HIDReportDescriptor *hidReportCustomJoystick;
#define HID_CUSTOM_JOYSTICK hidReportCustomJoystick
//================================================================================
//================================================================================
//	Joystick

class HIDCustomJoystick : public HIDReporter
{
protected:
	CustomJoystickReport_t joyReport;
	bool manualReport = false;
	void safeSendReport(void);

public:
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
	void axis(uint8_t analog, uint32_t val);
	// Constructor
	HIDCustomJoystick(USBHID &HID, uint8_t reportID = HID_JOYSTICK_REPORT_ID)
		: HIDReporter(HID, hidReportJoystick, (uint8_t *)&joyReport, sizeof(joyReport), reportID)
	{
		joyReport.buttons = 0;
		for (int i = 0; i < 8; i++)
			joyReport.axis[i] = 0;
	}
};