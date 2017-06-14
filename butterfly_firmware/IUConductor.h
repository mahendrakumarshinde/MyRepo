#ifndef IUCONDUCTOR_H
#define IUCONDUCTOR_H

#include <Arduino.h>

#include "IUFeature.h"
#include "IUFeatureConfigurator.h"
#include "IUSensorConfigurator.h"
// Components
#include "IURGBLed.h"
#include "IUI2C.h"
#include "IUBMD350.h"
#include "IUESP8285.h"

/**
 * A class to hold create, configure, activate and hold features
 * In current implementation, max possible number of features is fixed to 10,
 * change static class member 'maxFeatureCount' if more is needed.
 */
class IUConductor
{
  public:
    static const uint16_t defaultClockRate = 48000;           //Hz (must be compatible with I2C)
    static const uint16_t defaultDataSendPeriod = 1500;       // ms - send data every T milliseconds
    static constexpr double defaultTimestamp = 1492144654.00; //1483228800.00;    // January 1st, 2017
    static const uint16_t shortestDataSendPeriod = 500;       // ms - send data every T milliseconds
    // Constructors, destructor, getters and setters
    IUConductor();
    IUConductor(String macAddress);
    virtual ~IUConductor();
    // Time management methods
    bool setClockRate( uint16_t clockRate);
    uint16_t getClockRate() {return m_clockRate; }
    void setDataSendPeriod(uint16_t dataSendPeriod);
    bool isDataSendTime();
    double getDatetime();
    // Usage modes, acquisition modes and power modes mgmt methods
    void changePowerPreset(powerPreset::option powerPreset);
    void changeUsageMode(usageMode::option mode);
    void changeAcquisitionMode(acquisitionMode::option mode);
    void changeOperationState(operationState::option state);
    bool checkAndUpdatePowerPreset();
    bool checkAndUpdateAcquisitionMode();
    bool checkAndUpdateOperationState();
    void enableAutoSleep() { m_autoSleepEnabled = true; }
    void disableAutoSleep() { m_autoSleepEnabled = false; }
    // Setup and configuration methods
    bool initInterfaces();
    bool initConfigurators();
    bool initSensors();
    bool linkFeaturesToSensors();
    // Operation methods
    void acquireAndSendData(bool asynchronous);
    void computeFeatures();
    bool streamData(HardwareSerial *port, bool newLine=false);
    bool beginDataAcquisition();
    void endDataAcquisition();
    bool resetDataAcquisition();
    // Communication methods
    void processInstructionsFromI2C();
    void processInstructionsFromBluetooth();
    void processInstructionsFromWifi();
    // Main setup and loop
    void setup(void (*callback)());
    void loop();
    // Public members
    IUFeatureConfigurator featureConfigurator;
    IUSensorConfigurator sensorConfigurator;
    IUI2C      *iuI2C;
    IUBMD350   *iuBluetooth;
    IUESP8285  *iuWifi;
    IURGBLed   *iuRGBLed;

  protected:
    String m_macAddress;
    // Time management
    uint16_t m_clockRate; //Hz (must be compatible with I2C)
    uint16_t m_dataSendPeriod;
    uint32_t m_bluesleeplimit;
    uint32_t m_lastSynchroTime;
    uint32_t m_lastSentTime;
    double m_refDatetime;              // last datetime received from bluetooth or wifi
    // Usage modes, acquisition modes and power modes mgmt
    powerPreset::option m_powerPreset;
    usageMode::option m_usageMode;
    acquisitionMode::option m_acquisitionMode;
    operationState::option m_operationState;
      // Timers for auto-sleep management
      bool m_autoSleepEnabled;
      uint32_t m_idleStartTime;
      uint32_t m_sleepStartTime;
      uint16_t m_startSleepTimer;
      uint16_t m_endSleepTimer;
    // Operations
    void (*m_callback)();
    bool m_inDataAcquistion;
};


#endif // IUCONDUCTOR_H

