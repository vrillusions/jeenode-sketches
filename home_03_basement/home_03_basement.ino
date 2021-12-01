// JeeNode Network
///////////////////////////////////////////////////////////////////////////////
// Node ID: 3
// Location: Basement
// Sensors: SHT21 physically connected to port 1
//          SHT21 logically has humidity on port 1 and temperature on port 2
//          Water sensor on port 3
//
// Humidity will be returned as a whole number already.  So 2105 means 21.05%
// humidity because floats are multiplied by 100 to get an integer
//
// For the water sensor, connect it to analog and ground. And when setting the
// threshold I noticed the following values while testing:
// <800 the sensor is fully submerged
// <900 the sensor is picking up just enough water to take it away from 1023
// >950 is dry
//
// SHT21
// Needs to be rewired to match the port interface. Pinout:
// D - SDA
// G - Gnd
// + - Vin
// A - SCL
//

#include <JeeLib.h>
#include <PortsSHT21.h>

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


// sketch constants
const int WATER_DETECTED = 1;
const int NO_WATER_DETECTED = 0;

// global variables
int batt_loop_count = 99;      // Make high so we get data first time
int has_lowbat = false;        // Start out assuming it's fine


SHT21 sht21_port1 (1);


// Print the line to serial if debug is enabled. The delay is important
void debug_print(String x) {
  if (DEBUG) {
    Serial.println(x);
    delay(100);
  }
}


// spend a little time in power down mode while the SHT11 does a measurement
static void shtDelay () {
  Sleepy::loseSomeTime(32); // must wait at least 20 ms
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
  float humi, temp;
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

  // get SHT21 values
  sht21_port1.measure(SHT21::HUMI, shtDelay);
  sht21_port1.measure(SHT21::TEMP, shtDelay);
  sht21_port1.calculate(humi, temp);
  payload[1] = int(humi * 100);
  payload[2] = int(temp * 100);
  debug_print("Humidity: " + String(humi) + "% Temperature: " + String(temp) + "C");

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

  Sleepy::loseSomeTime(60000);
}
