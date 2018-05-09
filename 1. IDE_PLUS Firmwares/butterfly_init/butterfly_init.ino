/**
 * 
 */

#include <Arduino.h>

#ifdef _VARIANT_BUTTERFLY_STM32L433CC_
    const uint8_t RED_PIN = A5;
    const uint8_t GREEN_PIN = A3;
    const uint8_t BLUE_PIN = A4;
    
#else
    const uint8_t RED_PIN = 25;
    const uint8_t GREEN_PIN = 26;
    const uint8_t BLUE_PIN = 38;
#endif

void changeLedColor(bool R, bool G, bool B)
{
    digitalWrite(RED_PIN, (int) (!R));
    digitalWrite(GREEN_PIN, (int) (!G));
    digitalWrite(BLUE_PIN, (int) (!B));
}

void setup()
{
    Serial.begin(115200);
    Serial1.begin(115200);
    pinMode(RED_PIN, OUTPUT);
    pinMode(GREEN_PIN, OUTPUT);
    pinMode(BLUE_PIN, OUTPUT);
    changeLedColor(1, 0, 0);
    delay(7000);
    changeLedColor(0, 1, 0);
}

void loop()
{
    delay(50);
}

