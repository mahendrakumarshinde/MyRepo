#ifndef IUSENSORCONFIGURATOR_H
#define IUSENSORCONFIGURATOR_H

#include <Arduino.h>
#include "IUUtilities.h"
#include "IUABCSensor.h"
#include "IUBattery.h"
#include "IURGBLed.h"
#include "IUBMX055.h"
#include "IUBMP280.h"
#include "IUI2S.h"


class IUSensorConfigurator
{
  public:
    struct sensorConfig {
      char sensorType;
      String className;
      uint16_t samplingRate;
    };
    static const uint8_t defaultSensorCount = 5;
    static sensorConfig noSensor;                             // Handles no sensor found
    static sensorConfig defaultSensorConfigs[defaultSensorCount];
    // Constructors, destructors, getters and setters
    IUSensorConfigurator();
    IUSensorConfigurator(IUI2C *iuI2C);
    virtual ~IUSensorConfigurator();
    IUABCSensor* getSensorFromType(char sensorType);
    uint8_t getSensorCount() { return m_sensorCount; }
    IUABCSensor** getSensors() { return m_sensors; }
    // Methods
    bool addSensor(sensorConfig sconfig);
    bool createAllSensorsWithDefaultConfig();
    void wakeUpSensors();
    void acquireDataAndSendToReceivers();
    void acquireDataAndDumpThroughI2C();
    void acquireAndStoreData();
    // Public members for sensors for convenience
    IUBattery *iuBattery;
    IURGBLed  *iuRGBLed;
    IUBMX055  *iuBMX055;
    IUBMP280  *iuBMP280;
    IUI2S     *iuI2S;


  protected:
    IUI2C *m_iuI2C;
    IUABCSensor *m_sensors[defaultSensorCount];
    uint8_t m_sensorCount;

};

#endif // IUSENSORCONFIGURATOR_H
