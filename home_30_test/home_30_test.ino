// JeeNode Network
///////////////////////////////////////////////////////////////////////////////
// Node ID: 30
// Location: Test
// Sensors: DHT22 physically connected to port 1
//          DHT22 logically has temperature on port 1 and humidity on port 2
//
// Humidity will be returned as a whole number already.  So 2105 means 21.05%
// humidity because floats are multiplied by 100 to get an integer
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
                            // true will output to serial but not broadcast anything
const int JEELINK_ID = 1;   // Jeelink is id 1 (or 0 to broadcast)
const int NETGROUP = 100;   // Netgroup everyone is on
const int NODE_ID = 30;     // id of THIS node
const int DHT22_PORT = 1;   // what port is the DHT connected to.  This port +1 will
                            // be used for humidity
const bool PRECISE = true;  // Set true for DHT22 and false for DHT11


// global variables
int batt_loop_count = 99;      // Make high so we get data first time
int has_lowbat = false;        // Start out assuming it's fine


// Print the line to serial if debug is enabled. The delay is important
void debug_print(String x) {
  if (DEBUG) {
    Serial.println(x);
    delay(100);
  }
}


DHTxx dht (DHT22_PORT+3);

void setup() {
  if (DEBUG){
    Serial.begin(57600);
  }
  debug_print("Initializing");

  rf12_initialize(NODE_ID, RF12_915MHZ, NETGROUP);
}


// Payload consists of 5 words:
// - Is battery low? (will be 1 if so)
// - Values from each of the 4 ports
void loop() {
  word payload[5] = {0,0,0,0,0};
  int temp, humi;

  // Update low battery every 30 minutes
  if (batt_loop_count >= 30) {
    has_lowbat = rf12_lowbat();
    batt_loop_count = 0;
  }
  else {
    batt_loop_count++;
  }
  payload[0] = has_lowbat;

  // If unable to retrieve sensor info (common on initial startup) then don't
  // report anything
  if (dht.reading(temp, humi, PRECISE)) {
    // Default output is only to one decimal point but set to two for consistency
    payload[DHT22_PORT] = temp * 10;
    payload[DHT22_PORT+1] = humi * 10;
    debug_print("Temperature: " + String(payload[DHT22_PORT]*0.01) + "C " + 
      "Humidity: " + String(payload[DHT22_PORT+1]*0.01) + "%");
      
    if (DEBUG) {
      debug_print("Not sending data");
      debug_print(' ');
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
