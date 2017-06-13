#ifndef IUSENSORCONFIGURATOR_H
#define IUSENSORCONFIGURATOR_H

#include <Arduino.h>
#include "IUABCSensor.h"
#include "IUBattery.h"
#include "IUBMX055Acc.h"
#include "IUBMX055Gyro.h"
#include "IUBMX055Mag.h"
#include "IUBMP280.h"
#include "IUCAMM8Q.h"
#include "IUI2S.h"


class IUSensorConfigurator
{
  public:
    static const uint8_t sensorCount = 7;
    // Sensors are ordered: battery, Accel, Gyro, Mag, BMP280, Sound, GNSS
    static uint16_t defaultSamplingRates[sensorCount];
    static IUABCSensor::powerMode defaultPowerMode[sensorCount];

    // Constructors, destructor, getters and setters
    IUSensorConfigurator();
    IUSensorConfigurator(IUI2C *iuI2C);
    virtual ~IUSensorConfigurator();
    void resetSensorPointers();
    IUABCSensor** getSensors() { return m_sensors; }
    // Configuration and run methods
    bool createAllSensorsWithDefaultConfig();
    void acquireDataAndSendToReceivers(bool asynchronous);
    void acquireDataAndDumpThroughI2C();
    void acquireAndStoreData(bool asynchronous);
    void resetAllReceivers();
    //Power management methods
    void allSensorsWakeUp();
    void allSensorsSleep();
    void allSensorsSuspend();
    // Diagnostic methods
    void exposeSensorsAndReceivers();
    // Public members for sensors for convenience
    IUBattery    *iuBattery;
    IUBMX055Acc  *iuAccelerometer;
    IUBMX055Gyro *iuGyroscope;
    IUBMX055Mag  *iuMagnetometer;
    IUBMP280     *iuBMP280;
    IUI2S        *iuI2S;
    IUCAMM8Q     *iuGNSS;

  protected:
    IUI2C *m_iuI2C;
    IUABCSensor *m_sensors[sensorCount];

};

#endif // IUSENSORCONFIGURATOR_H
