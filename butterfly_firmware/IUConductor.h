#ifndef IUCONDUCTOR_H
#define IUCONDUCTOR_H

#include <Arduino.h>

#include "IUFeature.h"
#include "IUFeatureConfigurator.h"
#include "IUSensorConfigurator.h"
// Components
#include "IURGBLed.h"
#include "IUBMD350.h"
#include "IUESP8285.h"
#include "IUI2C.h"
#include "IUBattery.h"
#include "IUBMX055.h"
#include "IUBMP280.h"
#include "IUI2S.h"

/**
 * A class to hold create, configure, activate and hold features
 * In current implementation, max possible number of features is fixed to 10,
 * change static class member 'maxFeatureCount' if more is needed.
 */
class IUConductor
{
  public:
    static const uint16_t defaultClockRate = 48000;           //Hz (must be compatible with I2C)
    static const uint16_t defaultDataSendPeriod = 1500;      // ms - send data every T milliseconds
    static constexpr double defaultTimestamp = 1492144654.00; //1483228800.00;    // January 1st, 2017
    static const uint16_t shortestDataSendPeriod = 500;      // ms - send data every T milliseconds
    // Constructors, destructors, getters and setters
    IUConductor();
    IUConductor(String macAddress);
    virtual ~IUConductor();
    bool setClockRate( uint16_t clockRate);
    uint16_t getClockRate() {return m_clockRate; }
    operationState getOperationState() { return m_opState; }
    operationMode getOperationMode() { return m_opMode; }
    void enableAutoSleep() { m_autoSleepEnabled = true; }
    void disableAutoSleep() { m_autoSleepEnabled = false; }
    bool isAutoSleepEnabled() { return m_autoSleepEnabled; }
    void setDataSendPeriod(uint16_t dataSendPeriod);
    bool isDataSendTime();
    double getDatetime();
    // Methods
    bool initInterfaces();
    bool initConfigurators();
    bool initSensors();
    bool linkFeaturesToSensors();
    void switchToUsage(usageMode mode);
    void switchToMode(operationMode mode);
    void switchToState(operationState state);
    bool checkAndUpdateMode();
    bool checkAndUpdateState();
    bool acquireAndSendData();
    void computeFeatures();
    bool streamData(HardwareSerial *port, bool newLine=false);
    bool beginDataAcquisition();
    void endDataAcquisition();
    bool resetDataAcquisition();
    void processInstructionsFromI2C();
    void processInstructionsFromBluetooth();
    void processInstructionsFromWifi();
    void setup(void (*callback)());
    void loop();
    // Diagnostic Functions

    // Public members
    IUFeatureConfigurator featureConfigurator;
    IUSensorConfigurator sensorConfigurator;
    IUI2C      *iuI2C;
    IUBMD350   *iuBluetooth;
    IUESP8285  *iuWifi;


  protected:
    String m_macAddress;
    usageMode m_usageMode;
    operationMode m_opMode;
    operationState m_opState;
    uint16_t m_clockRate; //Hz (must be compatible with I2C)
    // Data acquisition
    void (*m_callback)();
    bool m_inDataAcquistion;
    // Time tracking, all durations are in milliseconds
    uint16_t m_dataSendPeriod;
    uint32_t m_bluesleeplimit;
    uint32_t m_lastSynchroTime;
    uint32_t m_lastSentTime;
    double m_refDatetime;              // last datetime received from bluetooth or wifi
    // Timers for Sleep mode
    bool m_autoSleepEnabled;
    uint32_t m_idleStartTime;
    uint32_t m_sleepStartTime;
    uint16_t m_startSleepTimer;
    uint16_t m_endSleepTimer;

};


#endif // IUCONDUCTOR_H






