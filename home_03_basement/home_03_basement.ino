// JeeNode Network
///////////////////////////////////////////////////////////////////////////////
// Node ID: 3
// Location: Basement
// Sensors: DHT22 physically connected to port 1
//          DHT22 logically has temperature on port 1 and humidity on port 2
//          Water sensor on port 3
//
// For the water sensor, connect it to analog and ground. And when setting the
// threshold I noticed the following values while testing:
// <800 the sensor is fully submerged
// <900 the sensor is picking up just enough water to take it away from 1023
// >950 is dry
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
const int NODE_ID = 3;      // id of THIS node
// When analog input is below this value then water has been detected
const int WATER_SENSOR_THRESHOLD = 950;
const int WATER_SENSOR_PIN = A2;  // A2 = Port 3 analog
const int DHT22_PORT = 1;   // what port is the DHT connected to.  This port +1 will
                            // be used for humidity
const bool PRECISE = true;  // Set true for DHT22 and false for DHT11

// sketch constants
const int WATER_DETECTED = 1;
const int NO_WATER_DETECTED = 0;

// global variables
int batt_loop_count = 99;      // Make high so we get data first time
int has_lowbat = false;        // Start out assuming it's fine

DHTxx dht (DHT22_PORT+3);

// Print the line to serial if debug is enabled. The delay is important
void debug_print(String x) {
  if (DEBUG) {
    Serial.println(x);
    delay(100);
  }
}

void setup() {
  if (DEBUG){
    Serial.begin(57600);
  }
  debug_print("Initializing");

  // omit this call to avoid linking in the CRC calculation code
  // disable since it can waste battery power
  //sht21_port1.enableCRC();

  rf12_initialize(NODE_ID, RF12_915MHZ, NETGROUP);

  pinMode(WATER_SENSOR_PIN, INPUT);
  digitalWrite(WATER_SENSOR_PIN, HIGH);
  delay(100);
}


// Payload consists of 5 words:
// - Is battery low? (will be 1 if so)
// - Values from each of the 4 ports
void loop() {
  word payload[5] = {0,0,0,0,0};
  int error = 0;
  int temp, humi;
  int water_sensor_3 = analogRead(WATER_SENSOR_PIN);

  // process water sensor data
  if (water_sensor_3 < WATER_SENSOR_THRESHOLD) {
    payload[3] = WATER_DETECTED;
    debug_print("Water detected");
  }
  else {
    payload[3] = NO_WATER_DETECTED;
    debug_print("No water detected");
  }

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
