/**
 * 
 */

#include <Arduino.h>
#include "RGBLed.h"

APA102RGBLedStrip rgbLedStrip(16);
GPIORGBLed rgbLed(25, 26, 38);

void setup()
{
    Serial.begin(115200);
    rgbLedStrip.setup();
    rgbLedStrip.startNewColorQueue(3);
    rgbLedStrip.unlockColors();
    rgbLedStrip.replaceColor(0, RGB_RED, 0, 1000);
    rgbLedStrip.replaceColor(1, RGB_GREEN, 0, 1000);
    rgbLedStrip.replaceColor(2, RGB_BLUE, 0, 1000);

    rgbLed.setup();
    rgbLed.startNewColorQueue(3);
    rgbLed.unlockColors();
    rgbLed.replaceColor(0, RGB_RED, 0, 1000);
    rgbLed.replaceColor(1, RGB_GREEN, 0, 1000);
    rgbLed.replaceColor(2, RGB_BLUE, 0, 1000);
}

void loop()
{
    rgbLedStrip.manageColorTransitions();
    rgbLed.manageColorTransitions();
    rgbLed.updateColors();
    delay(10);
}

