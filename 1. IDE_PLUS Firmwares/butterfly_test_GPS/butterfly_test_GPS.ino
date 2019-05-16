/**
 * 
 */

#include <Arduino.h>
#include <GNSS.h>

#ifdef _VARIANT_BUTTERFLY_STM32L433CC_
    const uint8_t RED_PIN = A5;
    const uint8_t GREEN_PIN = A3;
    const uint8_t BLUE_PIN = A4;
    
#else
    const uint8_t RED_PIN = 25;
    const uint8_t GREEN_PIN = 26;
    const uint8_t BLUE_PIN = 38;
#endif

#define PPS_PIN 44


GNSSLocation location;
int gnssAvailable;
//bool ppsFlag = false;

void setup()
{
    Serial.begin(115200);
    pinMode(RED_PIN, OUTPUT);
    pinMode(GREEN_PIN, OUTPUT);
    pinMode(BLUE_PIN, OUTPUT);

    delay(5000);
    
    changeLedColor(1, 0, 0);
    
    // Start GNSS
    Serial.print("Start GNSS setup: ");
    Serial.println(millis());
    
    Serial.print("GNSS begin: ");
    GNSS.begin(Serial2, GNSS.MODE_UBLOX, GNSS.RATE_1HZ);
    while (!GNSS.done()) { }
    Serial.println(millis());

    // Choose satellites
    Serial.print("GNSS set constellations: ");
    Serial.println(GNSS.setConstellation(GNSS.CONSTELLATION_GPS));
    while (!GNSS.done()) { }
    Serial.println(millis());
    
    Serial.print("GNSS set SBAS: ");
    Serial.println(GNSS.setSBAS(true));
    while (!GNSS.done()) { }
    Serial.println(millis());

    // Set power mode
    Serial.print("GNSS set power: ");
    Serial.println(GNSS.wakeup());
//    Serial.println(GNSS.sleep());
//    Serial.println(GNSS.setPeriodic(10, 20, true));  // set periodic wake and sleep mode
    while (!GNSS.done()) { } // wait for set to complete
    Serial.println(millis());
    
    changeLedColor(0, 0, 0);
    Serial.print("End of setup: ");
    Serial.println(millis());

//    Serial.print("GNSS update PPS: ");
//    GNSS.ppsUpdate(100);
//    while (!GNSS.done()) { }
//    Serial.println(millis());
}

void loop()
{
    delay(1000);
    Serial.print("Start read: ");
    Serial.println(millis());
    changeLedColor(0, 1, 0);
    location = GNSS.read();
    delay(50);
    changeLedColor(0, 0, 0);
    Serial.print("End read: ");
    Serial.println(millis());
    
    if (location)
    {
        Serial.println("is available!");    
        Serial.print("fixType: ");
        Serial.println(location.fixType());
        
        Serial.print("fixQuality: ");
        Serial.println(location.fixQuality());
        
        Serial.print("year: ");
        Serial.println(location.year());
        
        Serial.print("month: ");
        Serial.println(location.month());
        
        Serial.print("day: ");
        Serial.println(location.day());
        
        Serial.print("hour: ");
        Serial.println(location.hour());
        
        Serial.print("minute: ");
        Serial.println(location.minute());
        
        Serial.print("second: ");
        Serial.println(location.second());
        
        Serial.print("millis: ");
        Serial.println(location.millis());
        
        Serial.print("correction: ");
        Serial.println(location.correction());
        
        Serial.print("latitude: ");
        Serial.println(location.latitude());
        
        Serial.print("longitude: ");
        Serial.println(location.longitude());
        
        Serial.print("altitude: ");
        Serial.println(location.altitude());
        
        Serial.print("separation: ");
        Serial.println(location.separation());
        
        Serial.print("speed: ");
        Serial.println(location.speed());
        
        Serial.print("course: ");
        Serial.println(location.course());
        
        Serial.print("climb: ");
        Serial.println(location.climb());
        
        Serial.print("ehpe: ");
        Serial.println(location.ehpe());
        
        Serial.print("evpe: ");
        Serial.println(location.evpe());
        
        Serial.print("pdop: ");
        Serial.println(location.pdop());
        
        Serial.print("hdop: ");
        Serial.println(location.hdop());
        
        Serial.print("vdop: ");
        Serial.println(location.vdop());
        
        Serial.print("ticks: ");
        Serial.println(location.ticks());
    }
    else
    {
        Serial.println("is not available");
    }
}

void changeLedColor(bool R, bool G, bool B)
{
    digitalWrite(RED_PIN, (int) (!R));
    digitalWrite(GREEN_PIN, (int) (!G));
    digitalWrite(BLUE_PIN, (int) (!B));
}

