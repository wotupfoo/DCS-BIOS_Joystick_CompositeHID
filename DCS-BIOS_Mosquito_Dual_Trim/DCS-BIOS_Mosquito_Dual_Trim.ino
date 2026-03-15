#include <Arduino.h>
#include <RotaryEncoder.h>

#define DCSBIOS_DEFAULT_SERIAL
#include <DcsBios.h> // DCS World BIOS Class Rx/Tx over Serial (DcsBios::)

#define airleronTrimMsg "AIRLERON_TRIM "
#define elevatorTrimMsg "ELEVATOR_TRIM "
#define rudderTrimMsg "RUDDER_TRIM " // Here for completeness but not used

// DH-89 Mosquito Trim Wheels - Expect 0=left/down, 1=no change, 2=right/up
// Convert RotaryEncoder output into these values manaually and
// send the equivalent of a DCS-BIOS message ("[device] [0,1,2]\n") manually
// Note there is no smart handling if the message was sent. It is assumed
// that the Serial can keep up as messages are only send on change.

// DcsBios::AnalogMultiPos aileronTrim("AILERON_TRIM", AnalogPins[0], STEPS);
RotaryEncoder airleronTrim(3, 2, RotaryEncoder::LatchMode::TWO03);

// DcsBios::AnalogMultiPos rudderTrim("RUDDER_TRIM", PIN, STEPS);
RotaryEncoder elevatorTrim(5, 4, RotaryEncoder::LatchMode::TWO03);

void setup()
{
    Serial.begin(115200);
    while (!Serial)
        ; // Make sure the Serial is running
    // All pin setup done by RotaryEncoder::
}

void loop()
{
    int dir;

    Serial.print(airleronTrimMsg);
    airleronTrim.tick();
    dir = (int)airleronTrim.getDirection() + 1; // RotaryEncoder -1,0,+1 -> DCS-BIOS 0,1,2
    Serial.print(dir);
    Serial.print("\t");

    Serial.print(elevatorTrimMsg);
    elevatorTrim.tick();
    dir = (int)elevatorTrim.getDirection() + 1; // RotaryEncoder -1,0,+1 -> DCS-BIOS 0,1,2
    Serial.print(dir);
    Serial.print("\n");

    delay(20);
}