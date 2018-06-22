/**
 * 
 */

#include <Arduino.h>
#include <IUButterflyI2S.h>  /* I2S digital audio */
#include "IUI2C.h"

const uint8_t RED_PIN = 25;
const uint8_t GREEN_PIN = 26;
const uint8_t BLUE_PIN = 38;

const uint8_t BMD350_RESET_PIN = 39;
const uint8_t BMD350_AT_CMD_PIN = 40;

const uint8_t LSM6DSM_ADDRESS          = 0x6A;
const uint8_t LSM6DSM_WHO_AM_I         = 0x0F;
const uint8_t LSM6DSM_I_AM             = 0x6A;

#define BLESerial Serial3

bool testPassed = true;


void changeLedColor(bool R, bool G, bool B)
{
    digitalWrite(RED_PIN, (int) (!R));
    digitalWrite(GREEN_PIN, (int) (!G));
    digitalWrite(BLUE_PIN, (int) (!B));
}

void BMD350_softReset()
{
    digitalWrite(BMD350_RESET_PIN, LOW); // reset BMD-350
    delay(1000); // wait a while
    digitalWrite(BMD350_RESET_PIN, HIGH); // restart BMD-350
}

/**
 * Go into AT Command Interface Mode
 */
void BMD350_enterATCommandInterface()
{
    digitalWrite(BMD350_AT_CMD_PIN, LOW);
    delay(10);
    BMD350_softReset();
    delay(3000); // Wait for power cycle to complete
}

/**
 * Exit AT Command interface (see enterATCommandInterface doc)
 */
void BMD350_exitATCommandInterface()
{
    digitalWrite(BMD350_AT_CMD_PIN, HIGH);
    delay(10);
    BMD350_softReset();
    delay(3000); // Wait for power cycle to complete
}

/**
 * Returns true if received a response before timeout, else false.
 */
bool waitAnswerWithTimeout(uint32_t timeout=5000)
{
    uint32_t startT = millis();
    while(!BLESerial.available())
    {
        delay(100);
        if (millis() - startT > timeout)
        {
            Serial.println("Timeout exceeded");
            return false;
        }
    }
    return true;
}


void setup()
{
    Serial.begin(115200);
    iuI2C.begin();
    BLESerial.begin(57600);
    pinMode(RED_PIN, OUTPUT);
    pinMode(GREEN_PIN, OUTPUT);
    pinMode(BLUE_PIN, OUTPUT);
    pinMode(BMD350_AT_CMD_PIN, OUTPUT);
    pinMode(BMD350_RESET_PIN, OUTPUT);
    changeLedColor(1, 1, 1);
    delay(3000);

    if (!iuI2C.checkComponentWhoAmI("LSM6DSM", LSM6DSM_ADDRESS, LSM6DSM_WHO_AM_I, LSM6DSM_I_AM)) {
        Serial.println("LSM6DSM not found");
        testPassed = false;
    }

    BMD350_enterATCommandInterface();
    BLESerial.write("at\r");
    if (waitAnswerWithTimeout()) {
        Serial.print("BMD350 AT command working? "); 
        while ( BLESerial.available() ) { Serial.write(BLESerial.read() ); }
    } else {
        Serial.println("BMD350 did not answer before timeout");
        testPassed = false;
    }
    BMD350_exitATCommandInterface();
    
}

void loop()
{
    if (testPassed) {
        changeLedColor(0, 1, 0);
    } else {
        changeLedColor(1, 0, 0);
    }
    delay(50);
}

