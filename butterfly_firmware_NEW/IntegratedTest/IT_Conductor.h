#ifndef IT_CONDUCTOR_H_INCLUDED
#define IT_CONDUCTOR_H_INCLUDED


#include <ArduinoUnit.h>
#include <ArduinoJson.h>

#include "../Conductor.h"


test(Conductor__jsonParsing)
{
    StaticJsonBuffer<1600> jsonBuffer;

    char json[239] = "{\"device\":{\"VERS\":\"1.0.0\",\"POW\":0,\"TSL\":60,\"TO"
                     "FF\":300,\"TCY\":3600,\"GRP\":[\"PRSSTD\"]},"
                     "\"components\":{\"ACC\":{\"FSR\":5},\"GYR\":{\"FSR\":1}},"
                     "\"features\":{\"A9X\":{\"TRH\":[0.25,0.5,0.75]},\"A9Y\":"
                     "{\"TRH\":[10,11,12]},\"A9Z\":{\"TRH\":[0.25,0.5,0.75]}}}";
    JsonObject& root = jsonBuffer.parseObject(json);
    assertTrue(root.success());

    JsonVariant sub_config = root["device"];
    assertTrue(sub_config.success());

    sub_config = root["components"];
    assertTrue(sub_config.success());

    sub_config = root["features"];
    assertTrue(sub_config.success());
}

test(Conductor__configurationReading)
{
    char json[239] = "{\"device\":{\"VERS\":\"1.0.0\",\"POW\":0,\"TSL\":60,\"TO"
                     "FF\":300,\"TCY\":3600,\"GRP\":[\"PRSSTD\"]},"
                     "\"components\":{\"ACC\":{\"FSR\":5},\"GYR\":{\"FSR\":1}},"
                     "\"features\":{\"A9X\":{\"TRH\":[0.25,0.5,0.75]},\"A9Y\":"
                     "{\"TRH\":[10,11,12]},\"A9Z\":{\"TRH\":[0.25,0.5,0.75]}}}";

    assertTrue(conductor.processConfiguration(json));

    // Check Device Functions
    assertTrue(healthCheckGroup.isActive());
    assertTrue(pressStandardGroup.isActive());
    assertFalse(calibrationGroup.isActive());
    assertFalse(motorStandardGroup.isActive());

    assertEqual(conductor.getSleepMode(), 0);
    assertEqual(conductor.getAutoSleepDelay(), 60);
    assertEqual(conductor.getSleepDuration(), 300);
    assertEqual(conductor.getCycleTime(), 3600);

    // Check feature config
    Feature *feature = Feature::getInstanceByName("A9X");
    assertEqual(round(feature->getThreshold(0) * 100), 25);
    assertEqual(round(feature->getThreshold(1) * 100), 50);
    assertEqual(round(feature->getThreshold(2) * 100), 75);

    feature = Feature::getInstanceByName("A9Y");
    assertEqual(round(feature->getThreshold(0) * 100), 1000);
    assertEqual(round(feature->getThreshold(1) * 100), 1100);
    assertEqual(round(feature->getThreshold(2) * 100), 1200);

    feature = Feature::getInstanceByName("A9Z");
    assertEqual(round(feature->getThreshold(0) * 100), 25);
    assertEqual(round(feature->getThreshold(1) * 100), 50);
    assertEqual(round(feature->getThreshold(2) * 100), 75);

    // Check sensor config
    assertEqual(iuAccelerometer.getScale(),
                iuAccelerometer.scaleOption::AFS_4G);
    assertEqual(iuGyroscope.getScale(),
                iuGyroscope.scaleOption::GFS_1000DPS);
}

test(Conductor__featureAndGroups)
{



}

test(Conductor__timeManagement)
{



}

test(Conductor__operation)
{



}

#endif // IT_CONDUCTOR_H_INCLUDED
