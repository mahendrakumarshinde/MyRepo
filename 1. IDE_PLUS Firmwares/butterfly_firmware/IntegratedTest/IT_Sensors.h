#ifndef TEST_SENSOR_H_INCLUDED
#define TEST_SENSOR_H_INCLUDED


#include <ArduinoUnit.h>
#include "../Sensor.h"
#include "../IUBattery.h"
#include "../IUBMP280.h"
#include "../IUBMX055Acc.h"
#include "../IUBMX055Gyro.h"
#include "../IUBMX055Mag.h"
#include "../IUCAMM8Q.h"
#include "../IUI2C.h"
#include "../IURTDExtension.h"


test(Sensors__declarations)
{
    #if defined(RTD_DAUGHTER_BOARD)
        assertEqual(Sensor::instanceCount, 8);
    #else
        assertEqual(Sensor::instanceCount, 7);
    #endif

    assertEqual(strcmp(Sensor::getInstanceByName("BAT")->getName(), "BAT"), 0);
    assertEqual(strcmp(Sensor::getInstanceByName("ACC")->getName(), "ACC"), 0);
    assertEqual(strcmp(Sensor::getInstanceByName("GYR")->getName(), "GYR"), 0);
    assertEqual(strcmp(Sensor::getInstanceByName("MAG")->getName(), "MAG"), 0);
    assertEqual(strcmp(Sensor::getInstanceByName("GPS")->getName(), "GPS"), 0);
    assertEqual(strcmp(Sensor::getInstanceByName("MIC")->getName(), "MIC"), 0);
    #ifdef RTD_DAUGHTER_BOARD
        assertEqual(strcmp(Sensor::getInstanceByName("RTD")->getName(), "RTD"),
                    0);
    #endif // RTD_DAUGHTER_BOARD
}

inline void i2sCallbackTestFunction()
{
    iuI2S.acquireData();
}

test(Sensors__I2SCallbackLoop)
{
    iuI2S.triggerDataAcquisition(i2sCallbackTestFunction);
    delay(5000);
    iuI2S.endDataAcquisition();
    assertTrue(false);
}

#endif // TEST_SENSOR_H_INCLUDED
