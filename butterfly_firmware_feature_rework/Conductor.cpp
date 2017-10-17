#include "Conductor.h"


/* =============================================================================
    Constructors and destructors
============================================================================= */

Conductor::Conductor(const char* version, const char* macAddress) :
    m_sleepMode(sleepMode::NONE),
    m_startTime(0),
    m_autoSleepDelay(60000),
    m_sleepDuration(10000),
    m_cycleTime(3600000),
    m_lastSynchroTime(0),
    m_refDatetime(Conductor::defaultTimestamp),
    m_operationState(OperationState::IDLE),
    m_inDataAcquistion(false),
    m_lastLitLedTime(0),
    m_usageMode(UsageMode::OPERATION),
    m_acquisitionMode(AcquisitionMode::NONE),
    m_streamingMode(StreamingMode::BLE)
{
    strcpy(m_version, version);
    strcpy(m_macAddress, macAddress);
}


/* =============================================================================
    Hardware & power management
============================================================================= */


/**
 * Put device in a short sleep (reduced power consumption but fast start up).
 *
 * Wake up time is around 100ms
 */
void Conductor::sleep(uint32_t duration)
{
    for (uint8_t i = 0; i < SENSOR_COUNT; ++i)
    {
        SENSORS[i]->sleep();
    }
    iuRGBLed.changeColor(IURGBLed::SLEEP);
    STM32.stop(duration);
    iuRGBLed.changeColor(IURGBLed::BLUE);
    for (uint8_t i = 0; i < SENSOR_COUNT; ++i)
    {
        SENSORS[i]->wakeUp();
    }
}

/**
 * Suspend the whole device for a long time.
 *
 * This suspends all the components and stop the STM32).
 * Wake up time is around 7750ms.
 */
void Conductor::suspend(uint32_t duration)
{
    if (iuBluetooth.getPowerMode() != PowerMode::SUSPEND)
    {
        iuBluetooth.suspend();
    }
    if (iuWiFi.getPowerMode() != PowerMode::SUSPEND)
    {
        iuWiFi.suspend();
    }
    for (uint8_t i = 0; i < SENSOR_COUNT; ++i)
    {
        SENSORS[i]->suspend();
    }
    iuRGBLed.changeColor(IURGBLed::SLEEP);
    STM32.stop(duration * 1000);
    iuRGBLed.changeColor(IURGBLed::BLUE);
    for (uint8_t i = 0; i < SENSOR_COUNT; ++i)
    {
        SENSORS[i]->wakeUp();
    }
}

/**
 * Automatically put the device to sleep, depending on its sleepMode
 *
 * Details of sleepModes:
 *  - NONE: The device never sleeps
 *  - AUTO: The device sleeps for autoSleepDuration after having been idle for
 *      more than autoSleepStart.
 *  - PERIODIC: The device alternates betweeen active and sleeping phases.
 */
void Conductor::manageSleepCycles()
{
    uint32_t now = millis();
    if (m_startTime > now)
    {
        m_startTime = 0; // Manage millis uint32_t overflow
    }
    if (m_sleepMode == sleepMode::AUTO)
    {
        if (m_startTime + m_autoSleepDelay > now)
        {
            sleep(m_sleepDuration);
        }
    }
    else if (m_sleepMode == sleepMode::PERIODIC)
    {
        if (m_startTime + (m_cycleTime - m_sleepDuration) > now)
        {
            suspend(m_sleepDuration);
            m_startTime = millis();
        }
    }
}


/* =============================================================================
    Configuration
============================================================================= */

/**
 * Device level configuration
 *
 * @return True if the configuration is valid, else false.
 */
bool Conductor::configure(JsonVariant &my_config)
{
    // Firmware and config version check
    const char* vers = my_config["VERS"].asString();
    if (vers == NULL || strcmp(m_version, vers) > 0)
    {
        if (debugMode)
        {
            debugPrint(F("Config error: received config version '"), false);
            debugPrint(vers, false);
            debugPrint(F("', expected '"), false);
            debugPrint(m_version, false);
            debugPrint(F("'"));
        }
        return false;
    }
    // Data send Period
    JsonVariant value = my_config["DS1"];
    if (value.success())
    {
        featureGroup1.setDataSendPeriod((uint16_t) (value.as<int>()));
    }
    value = my_config["DS2"];
    if (value.success())
    {
        featureGroup2.setDataSendPeriod((uint16_t) (value.as<int>()));
    }
    value = my_config["DS3"];
    if (value.success())
    {
        featureGroup3.setDataSendPeriod((uint16_t) (value.as<int>()));
    }
    // Sleep management
    bool changed = false;
    value = my_config["POW"];
    if (value.success())
    {
        m_sleepMode = (sleepMode) (value.as<int>());
        changed = true;
    }
    value = my_config["TSL"];
    if (value.success())
    {
        m_autoSleepDelay = (uint32_t) (value.as<int>());
        changed = true;
    }
    value = my_config["TOFF"];
    if (value.success())
    {
        m_sleepDuration = (uint32_t) (value.as<int>());
        changed = true;
    }
    value = my_config["TCY"];
    if (value.success())
    {
        m_cycleTime = (uint32_t) (value.as<int>());
        changed = true;
    }
    if (changed)
    {
        m_startTime = millis();
    }
    return true;
}


/* =============================================================================
    Time management
============================================================================= */

/**
 * Set the refDatetime and update the lastSynchrotime to now.
 */
void Conductor::setRefDatetime(double refDatetime)
{
    m_refDatetime = refDatetime;
    m_lastSynchroTime = millis();
    if (loopDebugMode)
    {
        debugPrint("Time sync: ", false);
        debugPrint(getDatetime());
    }
}

/**
 * Get the timestamp corresponding to now (from millis and lastSynchroTime)
 */
double Conductor::getDatetime()
{
    uint32_t now = millis();
    // TODO millis doesn't take into account time while STM32.stop()
    return m_refDatetime + (double) (now - m_lastSynchroTime) / 1000.;
}


/* =============================================================================
    Operations
============================================================================= */

/**
 * Start data acquisition by beginning I2S data acquisition
 * NB: Other sensor data acquisition depends on I2S drumbeat
 */
bool Conductor::beginDataAcquisition()
{
    if (m_inDataAcquistion)
    {
        return true; // Already in data acquisition
    }
    // m_inDataAcquistion should be set to true BEFORE triggering the data
    // acquisition, otherwise I2S buffer won't be emptied in time
    m_inDataAcquistion = true;
    if (!iuI2S.triggerDataAcquisition(m_callback))
    {
        m_inDataAcquistion = false;
    }
    return m_inDataAcquistion;
}

/**
 * End data acquisition by disabling I2S data acquisition
 *
 * Note that asynchronous sensor data acquisition depends on I2S drumbeat.
 */
void Conductor::endDataAcquisition()
{
  if (!m_inDataAcquistion)
  {
    return; // nothing to do
  }
  iuI2S.endDataAcquisition();
  m_inDataAcquistion = false;
  // Delay to make sure that the I2S callback function is called one more time
  delay(50);
}

/**
 * End and restart the data acquisition
 *
 * @return the output of beginDataAcquisition (if it was successful or not)
 */
bool Conductor::resetDataAcquisition()
{
  endDataAcquisition();
  /* TODO Reset feature counters */
  delay(500);
  return beginDataAcquisition();
}

/**
 * Data acquisition function
 *
 * Method formerly benchmarked for (accel + sound) at 10microseconds.
 * @param  When asynchronous is true, acquire data from asynchronous sensors,
 *  else acquire data from synchronous sensors.
 */
void Conductor::acquireData(bool asynchronous)
{
    if (!m_inDataAcquistion || m_acquisitionMode == AcquisitionMode::NONE)
    {
        return;
    }
    if (iuI2C.isError())
    {
        if (debugMode)
        {
            debugPrint(F("I2C error"));
        }
        iuI2C.resetErrorFlag();  // Reset I2C error
        iuI2S.readData();         // Empty I2S buffer to continue
        return;
    }
    // If RAWDATA mode, send last data batch before collecting the new data
    if (m_acquisitionMode == AcquisitionMode::RAWDATA)
    {
        if (m_streamingMode == StreamingMode::WIRED)
        {
            for (uint8_t i = 0; i < SENSOR_COUNT; ++i)
            {
                if (SENSORS[i]->isAsynchronous() == asynchronous)
                {
                    SENSORS[i]->sendData(iuUSB.port);
                }
            }
        }
        else if (loopDebugMode)
        {
            debugPrint(F("Can only send RAW DATA in WIRED mode."));
        }
    }
    // Collect the new data
    for (uint8_t i = 0; i < SENSOR_COUNT; ++i)
    {
        if (SENSORS[i]->isAsynchronous() == asynchronous)
        {
            SENSORS[i]->acquireData();
        }
    }
}

/**
 * Compute each feature that is ready to be computed
 *
 * NB: Does nothing when not in "FEATURE" AcquisitionMode.
 */
void Conductor::computeFeatures()
{
    if (m_acquisitionMode != AcquisitionMode::FEATURE)
    {
        return;
    }
    // Run feature computers
    for (uint8_t i = 0; i < FEATURE_COMPUTER_COUNT; ++i)
    {
        if (FEATURE_COMPUTERS[i]->isActive())
        {
            FEATURE_COMPUTERS[i]->compute();
        }
    }
    // Acknowledge sections of feature buffers (free them for recording)
    for (uint8_t i = 0; i < FEATURE_COUNT; ++i)
    {
        if (FEATURES[i]->isActive())
        {
            FEATURES[i]->acknowledgeIfReady();
        }
    }
}

/**
 * Update the global operation state from feature operation states.
 *
 * The conductor operation state is based on the highest state from features.
 * The OperationState is only updated when in AcquisitionMode::FEATURE.
 * Also updates the LED color accordingly.
 * NB: Does nothing when not in "FEATURE" AcquisitionMode.
 */
void Conductor::updateOperationState()
{
    if (m_acquisitionMode != AcquisitionMode::FEATURE)
    {
        return;
    }
    OperationState::option newState = OperationState::IDLE;
    OperationState::option featState;
    for (uint8_t i = 0; i < FEATURE_COUNT; i++)
    {
        FEATURES[i]->getOperationState();
        featState = FEATURES[i]->getOperationState();
        if ((uint8_t) newState < (uint8_t) featState)
        {
            newState = featState;
        }
    }
    m_operationState = newState;
    // Light the LED to show the state
    uint32_t now = millis();
    if (m_lastLitLedTime > now || m_lastLitLedTime + showOpStateTimer > now)
    {
        iuRGBLed.showOperationState(m_operationState);
        m_lastLitLedTime = now;
    }
}

/**
 * Send feature data through Serial, BLE or WiFi depending on StreamingMode
 *
 * NB: If the AcquisitionMode is not FEATURE, does nothing.
 * @return true if data was sent, else false
 */
void Conductor::streamFeatures()
{
    if (m_acquisitionMode != AcquisitionMode::FEATURE)
    {
        return;
    }
    HardwareSerial *port = NULL;
    switch (m_streamingMode)
    {
        case StreamingMode::WIRED:
            port = iuUSB.port;
            break;
        case StreamingMode::BLE:
            port = iuBluetooth.port;
            break;
        case StreamingMode::WIFI:
            port = iuWiFi.port;
            break;
    }
    double timestamp = getDatetime();
    featureGroup1.stream(port, m_operationState, timestamp);
    featureGroup2.stream(port, m_operationState, timestamp);
    featureGroup3.stream(port, m_operationState, timestamp);
}


/* =============================================================================
    Configuration and Mode management
============================================================================= */

/**
 * Switch to a new operation mode
 *
 * 1. When switching to "run", "record" or "data collection" mode, start data
 *    acquisition with beginDataAcquisition().
 * 2. When switching to any other modes, end data acquisition
 *    with endDataAcquisition().
 */
void Conductor::changeAcquisitionMode(AcquisitionMode::option mode)
{
    if (m_acquisitionMode == mode)
    {
        return; // Nothing to do
    }
    m_acquisitionMode = mode;
    /* TODO => normally we should resetDataAcquisition in RAWDATA and FEATURE
    modes, but that blocks so far, need to check why */
    switch (m_acquisitionMode)
    {
        case AcquisitionMode::RAWDATA:
            iuRGBLed.changeColor(IURGBLed::CYAN);
            break;
        case AcquisitionMode::FEATURE:
            iuRGBLed.changeColor(IURGBLed::BLUE);
            beginDataAcquisition();
            break;
        case AcquisitionMode::NONE:
            endDataAcquisition();
            break;
        default:
            if (loopDebugMode)
            {
                debugPrint(F("Invalid acquisition Mode"));
            }
            break;
    }
}

/**
 * Switch to a StreamingMode
 */
void Conductor::changeStreamingMode(StreamingMode::option mode)
{
    if (m_streamingMode == mode)
    {
        return; // Nothing to do
    }
    m_streamingMode = mode;
    if (loopDebugMode)
    {
        debugPrint(F("\nStreaming mode is: "), false);
        switch (m_streamingMode)
        {
        case StreamingMode::WIRED:
            debugPrint(F("USB"));
            break;
        case StreamingMode::BLE:
            debugPrint(F("BLE"));
            break;
        case StreamingMode::WIFI:
            debugPrint(F("WIFI"));
            break;
        case StreamingMode::STORE:
            debugPrint(F("SPI STORAGE"));
            break;
        default:
            debugPrint(F("Invalid streaming Mode"));
            break;
        }
    }
}

/**
 * Switch to a new Usage Mode
 */
void Conductor::changeUsageMode(UsageMode::option usage)
{
    if (m_usageMode == usage)
    {
        return; // Nothing to do
    }
    m_usageMode = usage;
    changeAcquisitionMode(UsageMode::acquisitionModeDetails[m_usageMode]);
    changeStreamingMode(UsageMode::streamingModeDetails[m_usageMode]);
    String msg;
    switch (m_usageMode)
    {
        case UsageMode::CALIBRATION:
            enableCalibrationFeatures();
            iuRGBLed.changeColor(IURGBLed::CYAN);
            iuRGBLed.lock();
            msg = "calibration";
            break;
        case UsageMode::EXPERIMENT:
            msg = "experiment";
            break;
        case UsageMode::OPERATION:
            iuRGBLed.unlock();
            iuRGBLed.changeColor(IURGBLed::BLUE);
            enableMotorFeatures();
            iuAccelerometer.resetScale();
            msg = "operation";
            break;
        default:
            if (loopDebugMode)
            {
                debugPrint(F("Invalid usage mode preset"));
            }
            return;
    }
    if (loopDebugMode)
    {
        debugPrint("\nSet up for " + msg + "\n");
    }
}


/* =============================================================================
    Main Setup and loop
============================================================================= */

/**
 * Handle most of the device setup
 *
 * This function is to be called in the main setup function.
 */
void Conductor::setup(void (*callback)())
{
    // Interfaces
    if (debugMode)
    {
        debugPrint(F("\nInitializing interfaces..."));
    }
    iuI2C.setupHardware();
    if (setupDebugMode)
    {
        iuI2C.scanDevices();
        debugPrint("");
    }
    iuBluetooth.setupHardware();
    iuWiFi.setupHardware();
    if(debugMode)
    {
        memoryLog(F("=> Successfully initialized interfaces"));
    }
    if (setupDebugMode)
    {
        debugPrint(' ');
        iuBluetooth.exposeInfo();
    }

    // Default feature configuration
    if (debugMode)
    {
        debugPrint(F("\nSetting up default feature configuration..."));
    }
    setUpComputerSource();
    enableMotorFeatures();
    if (debugMode)
    {
        memoryLog(F("=> Succesfully configured default features"));
    }

    // Sensors
    if (debugMode)
    {
        debugPrint(F("\nInitializing sensors..."));
    }
    uint16_t callbackRate = iuI2S.getCallbackRate();
    for (uint8_t i = 0; i < SENSOR_COUNT; ++i)
    {
        SENSORS[i]->setupHardware();
        if (SENSORS[i]->isAsynchronous())
        {
            SENSORS[i]->setCallbackRate(callbackRate);
        }
    }
    if (debugMode)
    {
      memoryLog(F("=> Successfully initialized sensors"));
    }

    //
    if (setupDebugMode)
    {
        exposeAllConfigurations();
        if (iuI2C.isError())
        {
            debugPrint(F("\nI2C Satus: Error"));
        }
        else
        {
            debugPrint(F("\nI2C Satus: OK"));
        }
        debugPrint(F("\n***Finished setup at (ms): "), false);
        debugPrint(millis(), false);
        debugPrint(F("***\n"));
    }

    m_callback = callback;
    changeAcquisitionMode(AcquisitionMode::FEATURE);
    iuRGBLed.showOperationState(m_operationState);  // Start LED
}
