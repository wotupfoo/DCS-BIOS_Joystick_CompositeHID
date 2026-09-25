#include "HIDCustomJoystick.h"
//================================================================================
// CustomJoystick:: (8 Axis 10-bit, 32 Button)
// Based on the joystick.cpp HIDJoystick::
//================================================================================
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

#ifndef MIN
#define MIN(a, b) (((a) < (b)) ? (a) : (b))
#endif
#ifndef MAX
#define MAX(a, b) (((a) > (b)) ? (a) : (b))
#endif
uint16_t HIDCustomJoystick::axis(uint8_t analog, uint16_t val)
{
    val = MIN(val,1023);    // Clamp to max range
    if(axisInverted[analog])
    {
        val = 1023 - val;
    }
//ALREADY UNSIGNED    val = MAX(0,val);       // Clamp to min range
    joyReport.axis[analog] = val;
    safeSendReport();
    return val;
}

void HIDCustomJoystick::invertAxis(uint8_t analog, bool inverted)
{
    axisInverted[analog] = inverted;
}