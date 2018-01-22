/**
 * 
 */

#include <Arduino.h>
#include <GNSS.h>

const uint8_t RED_PIN = A5;
const uint8_t GREEN_PIN = A3;
const uint8_t BLUE_PIN = A4;

void changeLedColor(bool R, bool G, bool B)
{
    Serial.begin(115200);
    digitalWrite(RED_PIN, (int) (!R));
    digitalWrite(GREEN_PIN, (int) (!G));
    digitalWrite(BLUE_PIN, (int) (!B));
}

void setup()
{
    pinMode(RED_PIN, OUTPUT);
    pinMode(GREEN_PIN, OUTPUT);
    pinMode(BLUE_PIN, OUTPUT);
    changeLedColor(1, 0, 0);
    delay(7000);
    GNSS.begin(Serial2, GNSS.MODE_UBLOX, GNSS.RATE_1HZ);
    while (!GNSS.done()) { }
    // GNSS deactivated for now, irregardless of the power mode
    GNSS.sleep();
    Serial.println("GNSS put to sleep");
    changeLedColor(0, 1, 0);
}

void loop()
{
    
}
