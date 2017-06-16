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
    static const uint16_t shortestDataSendPeriod = 512;       // ms - send data every T milliseconds
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
    // Usage, acquisition, power, streaming and state mgmt methods
    void millisleep(uint32_t duration);
    void sleep(uint32_t duration);
    void doMinimalPowerConfig();
    void doDefaultPowerConfig();
    void manageAutoSleep();
    void configureAutoSleep(bool enabled, uint16_t startSleepTimer, uint32_t sleepDuration);
    void manageSleepCycle();
    void configureSleepCycle(bool enabled, uint32_t onTime, uint32_t cycleTime);
    void changeAcquisitionMode(acquisitionMode::option mode);
    void changeStreamingMode(streamingMode::option mode);
    void changeUsagePreset(usagePreset::option usage);
    void changeOperationState(operationState::option state);
    void checkAndUpdateOperationState();
    // Setup and configuration methods
    bool initInterfaces();
    bool initConfigurators();
    bool initSensors();
    bool linkFeaturesToSensors();
    // Operation methods
    void acquireAndSendData(bool asynchronous);
    void computeFeatures();
    bool streamFeatures(bool newLine=false);
    bool storeData() {}                       // TODO => implement
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
    // Usage, acquisition, power, streaming and state mgmt
    acquisitionMode::option m_acquisitionMode;
    streamingMode::option m_streamingMode;
    usagePreset::option m_usagePreset;
    operationState::option m_operationState;
      // Timers for auto-sleep management (all in milliseconds)
      bool m_autoSleepEnabled;
      uint32_t m_idleStartTime;
      uint16_t m_startSleepTimer;
      uint32_t m_autoSleepDuration;
      // Timers for sleep cycle management (all in seconds)
      bool m_sleepCycleEnabled;
      uint32_t m_onTime;
      uint32_t m_cycleTime;
      uint32_t m_cycleStartTime;
    // Operations
    void (*m_callback)();
    bool m_inDataAcquistion;
};


#endif // IUCONDUCTOR_H

