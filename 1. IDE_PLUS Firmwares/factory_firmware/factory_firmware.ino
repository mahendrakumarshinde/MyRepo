/**
 * 
 */

#include <Arduino.h>
#include "RGBLed.h"
#include "LedManager.h"

/***** Firmware version *****/
const char FIRMWARE_VERSION[8] = "2.0.0";

#ifdef _VARIANT_BUTTERFLY_STM32L433CC_
    const uint8_t RED_PIN = A5;
    const uint8_t GREEN_PIN = A3;
    const uint8_t BLUE_PIN = A4;
#else
    const uint8_t RED_PIN = 25;
    const uint8_t GREEN_PIN = 26;
    const uint8_t BLUE_PIN = 38;
#endif

APA102RGBLedStrip rgbLedStrip(16);
GPIORGBLed rgbLed(25, 26, 38);

LedManager ledManager(&rgbLed,&rgbLedStrip);


void changeLedColor(bool R, bool G, bool B)
{
    digitalWrite(RED_PIN, (int) (!R));
    digitalWrite(GREEN_PIN, (int) (!G));
    digitalWrite(BLUE_PIN, (int) (!B));
}

void setup()
{
    Serial.begin(115200);
    pinMode(RED_PIN, OUTPUT);
    pinMode(GREEN_PIN, OUTPUT);
    pinMode(BLUE_PIN, OUTPUT);   

    rgbLed.setup();
    ledManager.setBaselineStatus(&STATUS_NO_STATUS);
    ledManager.showStatus(&STATUS_NO_STATUS);
    ledManager.stopColorOverride();
}

void loop()
{
    changeLedColor(1, 0, 0);
    ledManager.stopColorOverride();
    ledManager.overrideColor(RGB_RED);
    delay(1000);
    changeLedColor(0, 1, 0);
    ledManager.stopColorOverride();
    ledManager.overrideColor(RGB_GREEN);
    delay(1000);
    changeLedColor(0, 0, 1);
    ledManager.stopColorOverride();
    ledManager.overrideColor(RGB_BLUE);
    delay(1000);
}

