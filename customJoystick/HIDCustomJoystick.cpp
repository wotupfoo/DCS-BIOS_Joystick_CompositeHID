#include "USBComposite.h"
#include "HIDCustomJoystick.h"

// This code requires gcc on low-endian devices.

//================================================================================
//================================================================================
//	Joystick

void HIDCustomJoystick::begin(void)
{
}

void HIDCustomJoystick::end(void)
{
}

void HIDCustomJoystick::setManualReportMode(bool mode)
{
    manualReport = mode;
}

bool HIDCustomJoystick::getManualReportMode()
{
    return manualReport;
}

void HIDCustomJoystick::safeSendReport()
{
    if (!manualReport)
    {
        sendReport();
    }
}

void HIDCustomJoystick::button(uint8_t button, bool val)
{
    uint32_t mask = ((uint32_t)1 << (button - 1));

    if (val)
    {
        joyReport.buttons |= mask;
    }
    else
    {
        joyReport.buttons &= ~mask;
    }
    safeSendReport();
}

void HIDCustomJoystick::buttons(uint32_t b)
{
    joyReport.buttons = b;
    safeSendReport();
}

void HIDCustomJoystick::axis(uint8_t analog, uint32_t val)
{
    joyReport.axis[analog] = val;
    safeSendReport();
}
