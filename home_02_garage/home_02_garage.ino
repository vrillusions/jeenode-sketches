// JeeNode Network
// Node ID: 2
// Location: Garage
// Sensors: DHT22 physically connected to port 1
//          DHT22 logically has temperature on port 1 and humidity on port 2
//          Garage door status (Port 4)
//
// TODO: apparently 3 AA batteries only last about 9 months
//
// This reads a magnetic door sensor. At least with this style
// the sensor is normally open and the circuit closes when the
// two sensors are next to each other.
//
// Connect one wire to digital and other to ground
//
// DHT22
// Needs to be rewired to match the port interface. Pinout:
// D - OUT
// G - -
// + - +
//


#include <JeeLib.h>

ISR(WDT_vect) { Sleepy::watchdogEvent(); }


// user settings
#define DEBUG false         // Set false to turn off all debugging
const int JEELINK_ID = 1;   // Jeelink is id 1 (or 0 to broadcast)
const int NETGROUP = 100;   // Netgroup everyone is on
const int NODE_ID = 2;      // id of THIS node
const int DHT22_PORT = 1;   // what port is the DHT connected to.  This port +1 will
                            // be used for humidity
const bool PRECISE = true;  // Set true for DHT22 and false for DHT11

// sketch constants
const int DOOR_OPEN = HIGH;    // Door open when signal is HIGH
const int DOOR_CLOSED = LOW;   // Door closed when LOW
const int DIO_OFFSET = 3;      // DIO is port num + 3

// global variables
int batt_loop_count = 99;      // Make high so we get data first time
int has_lowbat = false;        // Start out assuming it's fine

DHTxx dht (DHT22_PORT+3);

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


void setup() {
  if (DEBUG){
    Serial.begin(57600);
  }
  debug_print("Initializing");

  rf12_initialize(NODE_ID, RF12_915MHZ, NETGROUP);

  pinMode(DIO4, INPUT_PULLUP);  
}


// Payload consists of 5 words:
// - Is battery low? (will be 1 if so)
// - Values from each of the 4 ports
void loop() {
  int temp, humi;
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

  if (DEBUG) {
    if (door_status_4 == DOOR_OPEN) {
      debug_print("Door open");
    }
    else {
      debug_print("Door closed");
    }
  }

  payload[0] = has_lowbat;
  payload[4] = door_status_4;

  // If unable to retrieve sensor info (common on initial startup) then don't
  // report anything
  if (dht.reading(temp, humi, PRECISE)) {
    // Default output is only to one decimal point but set to two for consistency
    payload[DHT22_PORT] = temp * 10;
    payload[DHT22_PORT+1] = humi * 10;
    debug_print("Temperature: " + String(payload[DHT22_PORT]*0.01) + "C " + 
      "Humidity: " + String(payload[DHT22_PORT+1]*0.01) + "%");

    // XXX: Untested what happens with dht sensor if response handles negative
    // temperatures correctly
    // // The value returned is unsigned so need to make it signed for
    // // negative temperatures
    // if (tempc > 128.0) {
    //   tempc -= 256;
    // }

    if (DEBUG) {
      debug_print("Not sending data");
      debug_print(" ");
    }
    else {
      rf12_sleep(RF12_WAKEUP);
      rf12_sendNow(JEELINK_ID, &payload, sizeof payload);
      rf12_sendWait(2);
      rf12_sleep(RF12_SLEEP);
    }
  }
  
  Sleepy::loseSomeTime(60000);
}
