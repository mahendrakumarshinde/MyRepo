#ifndef TEST_SENSOR_H_INCLUDED
#define TEST_SENSOR_H_INCLUDED


#include <ArduinoUnit.h>
#include "../Sensor.h"


test(Sensor__base_class_test)
{
    Feature dest1("001", 2, 1);
    Feature dest2("002", 2, 1);
    Sensor sensor("SEN", 2, &dest1, &dest2);

    /***** Values after initialization *****/
    assertTrue(sensor.isNamed("SEN"));
    assertEqual(sensor.getDestinationCount(), 2);
    assertTrue(sensor.getMaxDestinationCount() >= 3);
    assertEqual(sensor.getResolution(), 1);
    assertEqual(dest1.getResolution(), sensor.getResolution());
    assertEqual(dest2.getResolution(), sensor.getResolution());

    /***** Destinations' resolutions are updated with the sensor's one *****/
    sensor.setResolution(10);
    assertEqual(dest1.getResolution(), sensor.getResolution());
    assertEqual(dest2.getResolution(), sensor.getResolution());
}


test(Sensor__HighFreqSensor)
{
    Feature dest1("001", 2, 1);
    Feature dest2("002", 2, 1);
    HighFreqSensor sensor("ASN", 2, &dest1, &dest2);

    /***** Values after initialization *****/
    assertTrue(sensor.isNamed("ASN"));
    assertTrue(sensor.isHighFrequency());
    assertEqual(dest1.getSamplingRate(), sensor.getSamplingRate());
    assertEqual(dest2.getSamplingRate(), sensor.getSamplingRate());

    /***** Destinations' resolutions are updated with the sensor's one *****/
    sensor.setSamplingRate(10);
    assertEqual(dest1.getSamplingRate(), sensor.getSamplingRate());
    assertEqual(dest2.getSamplingRate(), sensor.getSamplingRate());
}


test(Sensor__LowFreqSensor)
{
    Feature dest1("001", 2, 1);
    Feature dest2("002", 2, 1);
    LowFreqSensor sensor("SYN", 2, &dest1, &dest2);

    /***** Values after initialization *****/
    assertTrue(sensor.isNamed("SYN"));
    assertFalse(sensor.isHighFrequency());
    assertEqual(sensor.getSamplingPeriod(), 1000);
    assertEqual(round(sensor.getSamplingRate() * 10000),
                round(10000.0f / (float) sensor.defaultSamplingPeriod))  ;
}

#endif // TEST_SENSOR_H_INCLUDED
