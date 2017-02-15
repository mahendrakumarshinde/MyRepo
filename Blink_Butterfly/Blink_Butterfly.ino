/* LED Blink, for Dragonfly
 
   This example code is in the public domain.
*/
#include <Wire.h>

// Butterfly
//#define myLed1 38 // green led
//#define myLed2 13 // red led
//#define myLed3 26 // blue led  
// IU pcb
#define myLed1 8  // green led
#define myLed2 6  // red led
#define myLed3 9  // blue led 


float VDDA, Temperature;

void setup() 
{
  Serial.begin(38400);
 // while (!SerialUSB) { }
delay(2000);
Serial.println("Serial enabled!");
 
  pinMode(myLed1, OUTPUT);
  digitalWrite(myLed1, HIGH);  // start with leds off, since active LOW
  pinMode(myLed2, OUTPUT);
  digitalWrite(myLed2, HIGH);
  pinMode(myLed3, OUTPUT);
  digitalWrite(myLed3, HIGH);
}

void loop() 
{
    digitalWrite(myLed2, !digitalRead(myLed2)); // toggle red led on
delay(100);                                    // wait 1 second
    digitalWrite(myLed2, !digitalRead(myLed2)); // toggle red led off
delay(1000);
digitalWrite(myLed3, !digitalRead(myLed3));
delay(100);
    digitalWrite(myLed3, !digitalRead(myLed3));
delay(1000);
    digitalWrite(myLed2, !digitalRead(myLed2));
delay(100);
    digitalWrite(myLed2, !digitalRead(myLed2));
delay(1000);

VDDA = STM32.getVREF();
Temperature = STM32.getTemperature();

Serial.print("VDDA == "); Serial.println(VDDA, 2); 
Serial.print("STM32L4 MCU Temperature == "); Serial.println(Temperature, 2);

}

// I2C read/write functions for the MPU9250 sensors

        void writeByte(uint8_t address, uint8_t subAddress, uint8_t data) {
        uint8_t temp[2];
        temp[0] = subAddress;
        temp[1] = data;
        Wire.transfer(address, &temp[0], 2, NULL, 0); 
        }

        uint8_t readByte(uint8_t address, uint8_t subAddress) {
        uint8_t temp[1];
        Wire.transfer(address, &subAddress, 1, &temp[0], 1);
        return temp[0];
        }

        void readBytes(uint8_t address, uint8_t subAddress, uint8_t count, uint8_t * dest) {
        Wire.transfer(address, &subAddress, 1, dest, count); 
        }


