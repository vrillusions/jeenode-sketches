// JeeNode Network
// Node ID: 2
// Location: Garage
// Sensors: Temperature (Port 1), Garage door status (Port 4)
//
// TODO: apparently 3 AA batteries only last about 9 months
// TODO: add battery status to output
//
// This reads a magnetic door sensor. At least with this style
// the sensor is normally open and the circuit closes when the
// two sensors are next to each other.
//
// Connect one wire to digital and other to ground
//
// TEMP421 code thanks to http://ka1kjz.com (my bookmark no longer points to the source
// posting though, should still be on there somewhere
// TEMP421 pinout (pins are counted if sensor is on right):
// D - pin3
// G - pin1
// + - pin2
// A - pin4


#include <JeeLib.h>

ISR(WDT_vect) { Sleepy::watchdogEvent(); }


// user settings
#define DEBUG false         // Set false to turn off all debugging
const int JEELINK_ID = 1;   // Jeelink is id 1 (or 0 to broadcast)
const int NETGROUP = 100;   // Netgroup everyone is on
const int NODE_ID = 2;      // id of THIS node


// sketch constants
const int DOOR_OPEN = HIGH;    // Door open when signal is HIGH
const int DOOR_CLOSED = LOW;   // Door closed when LOW
const int DIO_OFFSET = 3;      // DIO is port num + 3

// global variables
int batt_loop_count = 99;      // Make high so we get data first time
int has_lowbat = false;        // Start out assuming it's fine

// Secong parameter that's commented out is the rate of communication
// Can be KHZMAX (default), KHZ400, or KHZ100
PortI2C temp_port1 (1 /*, PortI2C::KHZ400 */);
DeviceI2C temp_dev1 (temp_port1, 0x2A);  // I2C Address of 0x2A

// Currently just have sensor connected to port 4
//int DIO1 = 1 + DIO_OFFSET;
//int DIO2 = 2 + DIO_OFFSET;
//int DIO3 = 3 + DIO_OFFSET;
const int DIO4 = 4 + DIO_OFFSET;


// Print the line to serial if debug is enabled. The delay is important
void debug_print(String x) {
  if (DEBUG) {
    Serial.println(x);
    delay(100);
  }
}


// Convenience function that will take in a float and return a string
String float2str(float val, int length) {
  char strval[length];
  dtostrf(val, 0, 2, strval);
  return strval;
}


// Returns the temperature of tempdev where tempdev is a DeviceI2C object
float get_temperature(DeviceI2C tempdev) {
  int temp_lo = 0;
  int temp_high = 0;
  float tempc = 0;

  tempdev.send();
  tempdev.write(0x00); // high byte is on register 0
  tempdev.receive();
  temp_high = tempdev.read(1);
  tempdev.stop();
  
  tempdev.send();
  tempdev.write(0x10); // low byte on register 0x10
  tempdev.receive();
  temp_lo = tempdev.read(1);
  tempdev.stop();
  
  tempc = (float)temp_lo / 256;
  tempc = tempc + temp_high;

  return tempc;
}


void setup() {
  if (DEBUG){
    Serial.begin(57600);
  }
  debug_print("Initializing");

  rf12_initialize(NODE_ID, RF12_915MHZ, NETGROUP);

  // Do this so the initialize the thermometer (which starts at 0.00)
  float _unused = get_temperature(temp_dev1);
  delay(200);
  
  pinMode(DIO4, INPUT_PULLUP);  
}


// Payload consists of 5 words:
// - Is battery low? (will be 1 if so)
// - Values from each of the 4 ports
void loop() {
  float tempc = 0;
  int door_status_4 = digitalRead(DIO4);
  word payload[5] = {0,0,0,0,0};

  // Update low battery every 30 minutes
  // TODO: Does this even save power?
  if (batt_loop_count >= 30) {
    has_lowbat = rf12_lowbat();
    batt_loop_count = 0;
  }
  else {
    batt_loop_count++;
  }

  tempc = get_temperature(temp_dev1);
  
  if (DEBUG) {
    if (door_status_4 == DOOR_OPEN) {
      debug_print("Door open");
    }
    else {
      debug_print("Door closed");
    }
    debug_print(float2str(tempc, 7));
  }

  payload[0] = has_lowbat;
  payload[1] = int(tempc * 100);
  payload[4] = door_status_4;
  
  rf12_sleep(RF12_WAKEUP);
  rf12_sendNow(JEELINK_ID, &payload, sizeof payload);
  rf12_sendWait(2);
  rf12_sleep(RF12_SLEEP);

  Sleepy::loseSomeTime(60000);
}
