#ifndef IT_BUTTERFLY_H
#define IT_BUTTERFLY_H

#include <ArduinoUnit.h>
#include "../InstancesButterfly.h"


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


#endif // IT_BUTTERFLY_H
