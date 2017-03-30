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
    static const uint8_t sensorCount = 5;
    // Sensors are ordered: battery, led, BMX055, BMP280, Sound
    static uint16_t defaultSamplingRates[sensorCount];
    // Constructors, destructors, getters and setters
    IUSensorConfigurator();
    IUSensorConfigurator(IUI2C *iuI2C);
    virtual ~IUSensorConfigurator();
    IUABCSensor** getSensors() { return m_sensors; }
    // Methods
    bool createAllSensorsWithDefaultConfig();
    void wakeUpSensors();
    void acquireDataAndSendToReceivers();
    void acquireDataAndDumpThroughI2C();
    void acquireAndStoreData();
    // Diagnostic Functions
    void exposeSensorsAndReceivers();
    // Public members for sensors for convenience
    IUBattery *iuBattery;
    IURGBLed  *iuRGBLed;
    IUBMX055  *iuBMX055;
    IUBMP280  *iuBMP280;
    IUI2S     *iuI2S;

  protected:
    IUI2C *m_iuI2C;
    IUABCSensor *m_sensors[sensorCount];

};

#endif // IUSENSORCONFIGURATOR_H
