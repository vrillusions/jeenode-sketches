// A simple thermostat using a TI TMP421 temp sensor via I2C.
// Output is standard RED/GREEN/YELLOW LED status indicator
// as seen at http://www.ka1kjz.com/787/
//
// see http://ka1kjz.com/
// 03/18/2010 Ronald C. Barnes (ka1kjz@ka1kjz.com)
//

#include <JeeLib.h>
ISR(WDT_vect) { Sleepy::watchdogEvent(); }

// *******************************
// **** USER SETTABLE DEFINES ****
// *******************************
#define SETPOINT 80.00   // set temp here (either C of F)
#define DIFFERENTIAL 3   // set differential here (C or F)
#define UNITS tempF      // uncomment to make Farenheit
//#define UNITS tempC    // uncomment to make Celsius

// instantiate an I2C on port 1
PortI2C myport (1 /*, PortI2C::KHZ400 */);
DeviceI2C temperature (myport, 0x2A);  // I2C addy of 0X2A

float tempC = 0;   // a place to store the celsius temp
float tempF = 0;   // a place to store the farenheit temp

int temp_hi = 0;   // a place to store the temp high byte
int temp_lo = 0;   // a place to store thetemp low byte

// **** GRAB TEMPERATURE FROM TMP 421
// **** LEAVES RESULTS IN GLOBALS tempC & tempF
void getTemperature()
{
  temperature.send();             // go into send mode
  temperature.write(0x00);        // high byte is on register 0
  temperature.receive();          // grab it
  temp_hi = temperature.read(1);  // store it
  temperature.stop();             // done

  temperature.send();             // go into send mode
  temperature.write(0x10);        // low byte is on register 0x10
  temperature.receive();          // grab it
  temp_lo = temperature.read(1);  // store it
  temperature.stop();             // done

  tempC = (float)temp_lo / 256;   // make low byte into remainder
  tempC = tempC + temp_hi;        // add high byte to make total temp
  tempF = (tempC * 9 / 5) + 32;   // convert to farenheit
}

void setup()
{
  Serial.begin(57600);            // initialize serial port
  Serial.println("Initializing"); // let us know...
  rf12_initialize(20, RF12_915MHZ, 5);  // id 20, netgroup 5
#ifdef DEGC
  Serial.println("Celsius Mode"); // inform us of Celsius Mode
  Serial.print(DIFFERENTIAL);
  Serial.println(" deg C differential"); // and report differential
#endif
#ifdef DEGF
  Serial.println("Farenheit Mode");  // inform us of Farenheit Mode
  Serial.print(DIFFERENTIAL);
  Serial.println(" deg F differential"); // and report differential
#endif
}

void loop()
{
  getTemperature();                  // go grab the temperature
  //Serial.print(temp_hi);               //
  //Serial.print(temp_lo);               //
  Serial.print(tempC);               //
  Serial.print(" Deg C  ");          // report it out the serial port
  Serial.print(tempF);               //
  Serial.println(" Deg F");          //

  //attempt to implement network broadcasts but then junk prints on display
  rf12_sleep(RF12_WAKEUP);
  rf12_sendNow(0, &tempC, sizeof tempC);
  rf12_sendWait(2);
  rf12_sleep(RF12_SLEEP);

  //delay(10000);
  Sleepy::loseSomeTime(60000);
}
