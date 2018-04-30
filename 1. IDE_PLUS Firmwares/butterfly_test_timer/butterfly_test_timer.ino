/**
 * 
 */

#include <Arduino.h>
#include <Timer.h>

#ifdef _VARIANT_BUTTERFLY_STM32L433CC_
    const uint8_t RED_PIN = A5;
    const uint8_t GREEN_PIN = A3;
    const uint8_t BLUE_PIN = A4;
    
#else
    const uint8_t RED_PIN = 25;
    const uint8_t GREEN_PIN = 26;
    const uint8_t BLUE_PIN = 38;
#endif


Timer t;

int ledEvent;

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
    changeLedColor(1, 0, 0);
    delay(7000);
    changeLedColor(0, 1, 0);

    
    changeLedColor(0, 0, 0);
    int tickEvent = t.every(2000, doSomething);
    Serial.print("2 second tick started id=");
    Serial.println(tickEvent);
    
    pinMode(13, OUTPUT);
    ledEvent = t.oscillate(RED_PIN, 50, HIGH);
    Serial.print("LED event started id=");
    Serial.println(ledEvent);
    
    int afterEvent = t.after(10000, doAfter);
    Serial.print("After event started id=");
    Serial.println(afterEvent); 
}

void loop()
{
    delay(50);
    t.update();
}

void doSomething()
{
    Serial.print("2 second tick: millis()=");
    Serial.println(millis());
}

void doAfter()
{
    Serial.println("stop the led event");
    t.stop(ledEvent);
    t.oscillate(RED_PIN, 500, HIGH, 5);
}
