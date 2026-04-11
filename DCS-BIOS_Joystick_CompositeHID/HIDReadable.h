#pragma once

// ---- Main items ----
#define HID_INPUT            0x81
#define HID_OUTPUT           0x91
#define HID_FEATURE          0xB1
#define HID_COLLECTION       0xA1
#define HID_END_COLLECTION   0xC0

// ---- Global items ----
#define HID_USAGE_PAGE       0x05
#define HID_LOGICAL_MIN      0x15
#define HID_LOGICAL_MAX      0x25
#define HID_LOGICAL_MAX_16   0x26
#define HID_REPORT_SIZE      0x75
#define HID_REPORT_COUNT     0x95
#define HID_REPORT_ID        0x85

// ---- Local items ----
#define HID_USAGE            0x09
#define HID_USAGE_MIN        0x19
#define HID_USAGE_MAX        0x29

// ---- Collection types ----
#define HID_COLLECTION_PHYSICAL     0x00
#define HID_COLLECTION_APPLICATION  0x01

// ---- Input flags ----
#define HID_DATA        0x00
#define HID_CONSTANT    0x01
#define HID_ARRAY       0x00
#define HID_VARIABLE    0x02
#define HID_ABSOLUTE    0x00
#define HID_RELATIVE    0x04

#define HID_INPUT_DATA_VAR_ABS  (HID_DATA | HID_VARIABLE | HID_ABSOLUTE)

// ============================
// Usage Pages
// ============================

// ============================
// Usage Page 0x01 — Generic Desktop
// ============================

#define HID_USAGE_PAGE_GENERIC_DESKTOP   0x01
#define HID_USAGE_PAGE_BUTTON            0x09

// ============================
// Generic Desktop (Usage Page 0x01) Usages
// (from 0x30 upward, plus the system block that follows in the table)
// ============================

// Axes / motion (0x30..0x3E)
#define HID_GD_X                 0x30   // Used for Steering
#define HID_GD_Y                 0x31
#define HID_GD_Z                 0x32   // Used for Throttle
#define HID_GD_RX                0x33
#define HID_GD_RY                0x34   // Used for Rudder
#define HID_GD_RZ                0x35   // Used for Brake
#define HID_GD_SLIDER            0x36   // Used for Clutch
#define HID_GD_DIAL              0x37
#define HID_GD_WHEEL             0x38
#define HID_GD_HAT_SWITCH        0x39
#define HID_GD_COUNTED_BUFFER    0x3A
#define HID_GD_BYTE_COUNT        0x3B
#define HID_GD_MOTION_WAKEUP     0x3C
#define HID_GD_START             0x3D
#define HID_GD_SELECT            0x3E

// Vector / velocity / accel-ish (0x40..0x48)
#define HID_GD_VX                0x40
#define HID_GD_VY                0x41
#define HID_GD_VZ                0x42
#define HID_GD_VBRX              0x43
#define HID_GD_VBRY              0x44
#define HID_GD_VBRZ              0x45
#define HID_GD_VNO               0x46
#define HID_GD_FEATURE_NOTIFY    0x47
#define HID_GD_RES_MULTIPLIER    0x48

// System controls (0x80..0x93)
#define HID_GD_SYSTEM_CONTROL            0x80
#define HID_GD_SYSTEM_POWER_DOWN         0x81
#define HID_GD_SYSTEM_SLEEP              0x82
#define HID_GD_SYSTEM_WAKE_UP            0x83
#define HID_GD_SYSTEM_CONTEXT_MENU       0x84
#define HID_GD_SYSTEM_MAIN_MENU          0x85
#define HID_GD_SYSTEM_APP_MENU           0x86
#define HID_GD_SYSTEM_MENU_HELP          0x87
#define HID_GD_SYSTEM_MENU_EXIT          0x88
#define HID_GD_SYSTEM_MENU_SELECT        0x89
#define HID_GD_SYSTEM_MENU_RIGHT         0x8A
#define HID_GD_SYSTEM_MENU_LEFT          0x8B
#define HID_GD_SYSTEM_MENU_UP            0x8C
#define HID_GD_SYSTEM_MENU_DOWN          0x8D
#define HID_GD_SYSTEM_COLD_RESTART       0x8E
#define HID_GD_SYSTEM_WARM_RESTART       0x8F
#define HID_GD_DPAD_UP                   0x90
#define HID_GD_DPAD_DOWN                 0x91
#define HID_GD_DPAD_RIGHT                0x92
#define HID_GD_DPAD_LEFT                 0x93

// Dock / display-ish (0xA0..0xB7)
#define HID_GD_SYSTEM_DOCK               0xA0
#define HID_GD_SYSTEM_UNDOCK             0xA1
#define HID_GD_SYSTEM_SETUP              0xA2
#define HID_GD_SYSTEM_BREAK              0xA3
#define HID_GD_SYSTEM_DEBUGGER_BREAK     0xA4
#define HID_GD_APPLICATION_BREAK         0xA5
#define HID_GD_APPLICATION_DEBUGGER_BREAK 0xA6
#define HID_GD_SYSTEM_SPEAKER_MUTE       0xA7
#define HID_GD_SYSTEM_HIBERNATE          0xA8

#define HID_GD_SYSTEM_DISPLAY_INVERT     0xB0
#define HID_GD_SYSTEM_DISPLAY_INTERNAL   0xB1
#define HID_GD_SYSTEM_DISPLAY_EXTERNAL   0xB2
#define HID_GD_SYSTEM_DISPLAY_BOTH       0xB3
#define HID_GD_SYSTEM_DISPLAY_DUAL       0xB4
#define HID_GD_SYSTEM_DISPLAY_TOGGLE     0xB5
#define HID_GD_SYSTEM_DISPLAY_SWAP       0xB6
#define HID_GD_SYSTEM_DISPLAY_LCD_AUTOSCALE 0xB7

// ============================
// Usage Page 0x02 — Simulation Controls
// ============================
#define HID_USAGE_PAGE_SIMULATION   0x02

#define HID_SIM_AILERON     0xB0
#define HID_SIM_ELEVATOR    0xB8
#define HID_SIM_RUDDER      0xBA
#define HID_SIM_THROTTLE    0xBB
#define HID_SIM_FLAPS       0xB6
#define HID_SIM_CLUTCH      0xC4
#define HID_SIM_BRAKE       0xC5
#define HID_SIM_STEERING    0xC8
