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

// Connect one wire to digital and other to ground

#include <JeeLib.h>

ISR(WDT_vect) { Sleepy::watchdogEvent(); }


// user settings
//#define DEBUG        // Comment out to disable
const int JEELINK_ID = 1;   // Jeelink is id 1 (or 0 to broadcast)
const int NETGROUP = 100;   // Netgroup everyone is on
const int NODE_ID = 2;      // id of THIS node


// sketch constants
const int DOOR_OPEN = HIGH;    // Door open when signal is HIGH
const int DOOR_CLOSED = LOW;   // Door closed when LOW
const int DIO_OFFSET = 3;      // DIO is port num + 3


// Basic macro for debugging
// Ideally change this to a function once I figure out how to accept anything
#ifdef DEBUG
  #define DEBUG_PRINT(x) Serial.println(x)
#else
  #define DEBUG_PRINT(x)
#endif


// TEMP421 code thanks to http://ka1kjz.com (my bookmark no longer points to the source
// posting though, should still be on there somewhere
// TEMP421 pinout (pins are counted if sensor is on right):
// D - pin3
// G - pin1
// + - pin2
// A - pin4

// Secong parameter that's commented out is the rate of communication
// Can be KHZMAX (default), KHZ400, or KHZ100
PortI2C temp_port1 (1 /*, PortI2C::KHZ400 */);
DeviceI2C temp_dev1 (temp_port1, 0x2A);  // I2C Address of 0x2A


// Currently just have sensor connected to port 4
//int DIO1 = 1 + DIO_OFFSET;
//int DIO2 = 2 + DIO_OFFSET;
//int DIO3 = 3 + DIO_OFFSET;
const int DIO4 = 4 + DIO_OFFSET;


// Returns the temperature of tempdev where tempdev is a DeviceI2C object
int get_temperature(DeviceI2C tempdev) {
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
#ifdef DEBUG
  Serial.begin(57600);
#endif
  DEBUG_PRINT("Initializing");

  rf12_initialize(NODE_ID, RF12_915MHZ, NETGROUP);

  pinMode(DIO4, INPUT_PULLUP);
}


void loop() {
  float tempc = 0;
  int door_status_4 = digitalRead(DIO4);
  word payload[4] = {0,0,0,0};

  // 4th word is index 3
  payload[3] = door_status_4;

#ifdef DEBUG
  if (door_status_4 == DOOR_OPEN) {
    DEBUG_PRINT("Door open");
  }
  else {
    DEBUG_PRINT("Door closed");
  }
#endif

  tempc = get_temperature(temp_dev1);
  DEBUG_PRINT(tempc);
  payload[0] = int(tempc * 100);

  rf12_sleep(RF12_WAKEUP);
  rf12_sendNow(JEELINK_ID, &payload, sizeof payload);
  rf12_sendWait(2);
  rf12_sleep(RF12_SLEEP);

  Sleepy::loseSomeTime(60000);
}
