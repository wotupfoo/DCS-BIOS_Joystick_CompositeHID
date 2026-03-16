#include <Arduino.h>
#include <RotaryEncoder.h>

#define DCSBIOS_DEFAULT_SERIAL
#include <DcsBios.h> // DCS World BIOS Class Rx/Tx over Serial (DcsBios::)

#define LOOP_DELAY_MS 5          // Milliseconds of pause per loop
#define LOOP_IDLE_TIMEOUT 10    // Number of idle loops before forced refresh

// DH-89 Mosquito Trim Wheels - Expect 0=left/down, 1=no change, 2=right/up
// Convert RotaryEncoder output into these values manaually and
// send the equivalent of a DCS-BIOS message ("[device] [0,1,2]\n") manually
// Note there is no smart handling if the message was sent. It is assumed
// that the Serial can keep up as messages are only send on change.

#define ENABLE_AILERON
#ifdef ENABLE_AILERON
#define airleronTrimMsg "AIRLERON_TRIM "
// DcsBios::AnalogMultiPos aileronTrim("AILERON_TRIM", AnalogPins[0], STEPS);
RotaryEncoder airleronTrim(3, 2, RotaryEncoder::LatchMode::TWO03);
#endif

#define ENABLE_ELEVATOR
#ifdef ENABLE_ELEVATOR
#define elevatorTrimMsg "ELEVATOR_TRIM "
// DcsBios::AnalogMultiPos rudderTrim("ELEVATOR_TRIM", PIN, STEPS);
RotaryEncoder elevatorTrim(5, 4, RotaryEncoder::LatchMode::TWO03);
#endif

//#define ENABLE_RUDDER
#ifdef ENABLE_RUDDER
#define rudderTrimMsg "RUDDER_TRIM " // Here for completeness but not used
// DcsBios::AnalogMultiPos rudderTrim("RUDDER_TRIM", PIN, STEPS);
RotaryEncoder elevatorTrim(7, 6, RotaryEncoder::LatchMode::TWO03);
#endif

void setup()
{
    Serial.begin(115200);
    while (!Serial); // Make sure the Serial is running
    // All pin setup done by RotaryEncoder::
}

void loop()
{
    int dir;

#ifdef ENABLE_AILERON
    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
    // AIRLERON INPUT PROCESSING
    static int airleronTrimDir = 1;
    static int airleronTrimIdle = 0;
    airleronTrim.tick();
    dir = (int)airleronTrim.getDirection() + 1; // RotaryEncoder -1,0,+1 -> DCS-BIOS 0,1,2
    if(dir != airleronTrimDir) {
      Serial.print(airleronTrimMsg);    Serial.println(dir);
      airleronTrimDir = dir;
      airleronTrimIdle = 0;
    }
    if(dir == 1) {
      airleronTrimIdle++;
      if(airleronTrimIdle > LOOP_IDLE_TIMEOUT) {      // Periodically send out that the Trim isn't moving
        Serial.print(airleronTrimMsg);  Serial.println(1);
        airleronTrimIdle = 0;
      }
    }
#endif // ENABLE_AILERON

#ifdef ENABLE_ELEVATOR
    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
    // ELEVATOR INPUT PROCESSING
    static int elevatorTrimDir = 1;
    static int elevatorTrimIdle = 0;
    elevatorTrim.tick();
    dir = (int)elevatorTrim.getDirection() + 1; // RotaryEncoder -1,0,+1 -> DCS-BIOS 0,1,2
    if(dir != elevatorTrimDir) {
      Serial.print(elevatorTrimMsg);    Serial.println(dir);
      elevatorTrimDir = dir;
    }
    if(dir == 1) {
      elevatorTrimIdle++;
      if(elevatorTrimIdle > LOOP_IDLE_TIMEOUT) {      // Periodically send out that the Trim isn't moving
        Serial.print(elevatorTrimMsg);  Serial.println(1);
        elevatorTrimIdle = 0;
      }
    }
#endif // ENABLE_ELEVATOR

#ifdef ENABLE_RUDDER
    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
    // RUDDER INPUT PROCESSING
    static int rudderTrimDir = 1;
    static int rudderTrimIdle = 0;
    rudderTrim.tick();
    dir = (int)rudderTrim.getDirection() + 1; // RotaryEncoder -1,0,+1 -> DCS-BIOS 0,1,2
    if(dir != rudderTrimDir) {
      Serial.print(rudderTrimMsg);    Serial.println(dir);
      rudderTrimDir = dir;
    }
    if(dir == 1) {
      rudderTrimIdle++;
      if(rudderTrimIdle > LOOP_IDLE_TIMEOUT) {      // Periodically send out that the Trim isn't moving
        Serial.print(rudderTrimMsg);  Serial.println(1);
        rudderTrimIdle = 0;
      }
    }
#endif // ENABLE_RUDDER

    delay(LOOP_DELAY_MS);
}