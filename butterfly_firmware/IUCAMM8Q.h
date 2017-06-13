/*
  Infinite Uptime Firmware

  Update:
    13/06/2017

  Component:
    Name:
      CAM-M8Q
    Description:
      GNSS
*/

#ifndef IUCAMM8Q_H
#define IUCAMM8Q_H


#include <Arduino.h>

#include <GNSS.h>
#include "IUABCSensor.h"
#include "IUI2C.h"

class IUCAMM8Q : public IUABCSensor
{
  public:
    static IUABCSensor::sensorTypeOptions sensorType = IUABCSensor::GNSS;
    enum dataSendOption : uint8_t {latitude    = 0,
                                   longitude   = 1,
                                   altitude    = 2,
                                   satellites  = 3,
                                   pdop        = 4,
                                   fixType     = 5,
                                   fixQuality  = 6,
                                   year        = 7,
                                   month       = 8,
                                   day         = 9,
                                   hour        = 10,
                                   minute      = 11,
                                   second      = 12,
                                   millisecond = 13,
                                   optionCount = 14};

    static const uint16_t defaultSamplingRate = 2; // Hz
    static const uint32_t defaultOnTime = 10;
    static const uint32_t defaultPeriod = 10;
    static const bool defaultForcedMode = true;

    // Constructor, destructor, getters and setters
    IUCAMM8Q(IUI2C *iuI2C);
    virtual ~IUCAMM8Q() {}
    virtual char getSensorType() { return (char) sensorType; }
    // Hardware & power management methods
    virtual void wakeUp();
    virtual void sleep();
    virtual void suspend();
    void setPeriodic(uint32_t onTime, uint32_t period, bool forced=false);
    void enterForcedMode();
    void exitForcedMode();
    // Data acquisition methods
    virtual void readData();
    // Communication methods
    virtual void sendToReceivers();
    virtual void dumpDataThroughI2C();
    virtual void dumpDataForDebugging();
    // Diagnostic Functions
    virtual void exposeCalibration();


  protected:
    // Configuration
    IUI2C *m_iuI2C;
    uint32_t m_onTime;
    uint32_t m_period;
    bool m_forcedMode;
    // Data acquisition
    GNSSLocation m_location;

};

#endif // IUCAMM8Q_H
