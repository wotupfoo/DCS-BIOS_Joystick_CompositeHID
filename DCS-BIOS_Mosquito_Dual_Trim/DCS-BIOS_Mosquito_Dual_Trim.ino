#include <Arduino.h>
#include <RotaryEncoder.h>

#define DCSBIOS_DEFAULT_SERIAL
#include <DcsBios.h> // DCS World BIOS Class Rx/Tx over Serial (DcsBios::)

#define airleronTrimMsg "AIRLERON_TRIM "
#define elevatorTrimMsg "ELEVATOR_TRIM "
#define rudderTrimMsg "RUDDER_TRIM "  // Here for completeness but not used

// DH-89 Mosquito Trim Wheels - Expect 0=left/down, 1=no change, 2=right/up
// Convert RotaryEncoder output into these values manaually and 
// send the equivalent of a DCS-BIOS message ("[device] [0,1,2]\n") manually
// Note there is no smart handling if the message was sent. It is assumed
// that the Serial can keep up as messages are only send on change.

//DcsBios::AnalogMultiPos aileronTrim("AILERON_TRIM", AnalogPins[0], STEPS);
RotaryEncoder airleronTrim(PB2, PB3, RotaryEncoder::LatchMode::TWO03);

//DcsBios::AnalogMultiPos rudderTrim("RUDDER_TRIM", PIN, STEPS);
RotaryEncoder elevatorTrim(PB3, PB4, RotaryEncoder::LatchMode::TWO03);

void setup() {
  while(!Serial);   // Make sure the Serial is running
  // All pin setup done by RotaryEncoder::
}

void loop()
{
  int newPos;
  int dir;
  static long apos = 0;
  static long epos = 0;

  airleronTrim.tick();
  newPos = airleronTrim.getPosition();
  if (apos != newPos) {
    dir = (int)elevatorTrim.getDirection() + 1; // RotaryEncoder -1,0,+1 -> DCS-BIOS 0,1,2
    Serial.print(airleronTrimMsg);
    Serial.print(newPos);
    Serial.print(dir);
    Serial.println();
    apos = newPos;
  }

  elevatorTrim.tick();
  newPos = elevatorTrim.getPosition();
  if (epos != newPos) {
    dir = (int)elevatorTrim.getDirection() + 1; // RotaryEncoder -1,0,+1 -> DCS-BIOS 0,1,2
    Serial.print(airleronTrimMsg);
    Serial.print(newPos);
    Serial.print(dir);
    Serial.println();
    epos = newPos;
  }

}