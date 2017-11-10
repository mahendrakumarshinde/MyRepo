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
    for (uint8_t i = 0; i < Sensor::instanceCount; ++i)
    {
        Sensor::instances[i]->sleep();
    }
    iuRGBLed.changeColor(IURGBLed::SLEEP);
    STM32.stop(duration);
    iuRGBLed.changeColor(IURGBLed::BLUE);
    for (uint8_t i = 0; i < Sensor::instanceCount; ++i)
    {
        Sensor::instances[i]->wakeUp();
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
    for (uint8_t i = 0; i < Sensor::instanceCount; ++i)
    {
        Sensor::instances[i]->suspend();
    }
    iuRGBLed.changeColor(IURGBLed::SLEEP);
    STM32.stop(duration * 1000);
    iuRGBLed.changeColor(IURGBLed::BLUE);
    for (uint8_t i = 0; i < Sensor::instanceCount; ++i)
    {
        Sensor::instances[i]->wakeUp();
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
        if (m_startTime + m_autoSleepDelay < now)
        {
            sleep(m_sleepDuration);
        }
    }
    else if (m_sleepMode == sleepMode::PERIODIC)
    {
        if (m_startTime + (m_cycleTime - m_sleepDuration) < now)
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
    const char* vers = my_config["VERS"].as<char*>();
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
    // Sleep management
    bool changed = false;
    JsonVariant value = my_config["POW"];
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

/**
 * Activate the computation of a feature
 *
 * Also recursively activate the computation of all the feature's required
 * components (ie the sources of its computer).
 */
void Conductor::activateFeature(Feature* feature)
{
    feature->activate();
    uint8_t compId = feature->getComputerId();
    if (compId == 0)
    {
        return;
    }
    FeatureComputer* computer = FeatureComputer::getInstanceById(compId);
    if (computer != NULL)
    {
        // Activate the feature's computer
        computer->activate();
        // Activate the computer's sources
        for (uint8_t i = 0; i < computer->getSourceCount(); ++i)
        {
            activateFeature(computer->getSource(i));
        }
    }
    // TODO Activate sensors as well?
}

/**
 * Check whether the feature computation can be deactivated.
 *
 * A feature is deactivatable if:
 *  - it is not streaming
 *  - all it's receivers (computers who use it as source) are deactivated
 */
bool Conductor::isFeatureDeactivatable(Feature* feature)
{
    if (feature->isStreaming())
    {
        return false;
    }
    FeatureComputer* computer;
    for (uint8_t i = 0; i < feature->getReceiverCount(); ++i)
    {
        computer = FeatureComputer::getInstanceById(feature->getReceiverId(i));
        if (computer != NULL && !computer->isActive())
        {
            return false;
        }
    }
    return true;
}


/**
 * Deactivate the computation of a feature
 *
 * Will also disable the feature streaming.
 * Will also check if the computation of the feature's required components can
 * be deactivated.
 */
void Conductor::deactivateFeature(Feature* feature)
{
    feature->deactivate();
    uint8_t compId = feature->getComputerId();
    if (compId != 0)
    {
        FeatureComputer* computer = FeatureComputer::getInstanceById(compId);
        if (computer != NULL)
        {
            // If none of the computer's destinations are active, the computer
            // can be deactivated too.
            bool deactivatable = true;
            for (uint8_t i = 0; i < computer->getDestinationCount(); ++i)
            {
                deactivatable &= (!computer->getDestination(i)->isActive());
            }
            if (deactivatable)
            {
                computer->deactivate();
                Feature* antecedent;
                for (uint8_t i = 0; i < computer->getSourceCount(); ++i)
                {
                    antecedent = computer->getSource(i);
                    if (!isFeatureDeactivatable(antecedent))
                    {
                        deactivateFeature(antecedent);
                    }
                }
            }
        }
    }
}

/**
 * Deactivate all features and feature computers
 */
void Conductor::deactivateAllFeatures()
{
    for (uint8_t i = 0; i < Feature::instanceCount; ++i)
    {
        Feature::instances[i]->deactivate();
    }
    for (uint8_t i = 0; i < FeatureComputer::instanceCount; ++i)
    {
        FeatureComputer::instances[i]->deactivate();
    }
}

/**
 *
 */
void Conductor::activateGroup(FeatureGroup *group)
{
    group->activate();
    Feature *feature;
    for (uint8_t i = 0; i < group->getFeatureCount(); ++i)
    {
        feature = group->getFeature(i);
        activateFeature(feature);
        feature->enableStreaming();
    }
}

/**
 *
 */
void Conductor::deactivateGroup(FeatureGroup *group)
{
    group->deactivate();
    for (uint8_t i = 0; i < group->getFeatureCount(); ++i)
    {
        deactivateFeature(group->getFeature(i));
    }
}

/**
 *
 */
void Conductor::deactivateAllGroups()
{
    for (uint8_t i = 0; i < FeatureGroup::instanceCount; ++i)
    {
        FeatureGroup::instances[i]->deactivate();
    }
    deactivateAllFeatures();
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
 * @param asynchronous  When true, acquire data from asynchronous sensors,
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
            for (uint8_t i = 0; i < Sensor::instanceCount; ++i)
            {
                if (Sensor::instances[i]->isAsynchronous() == asynchronous)
                {
                    Sensor::instances[i]->sendData(iuUSB.port);
                }
            }
        }
        else if (loopDebugMode)
        {
            debugPrint(F("Can only send RAW DATA in WIRED mode."));
        }
    }
    // Collect the new data
    for (uint8_t i = 0; i < Sensor::instanceCount; ++i)
    {
        if (Sensor::instances[i]->isAsynchronous() == asynchronous)
        {
            Sensor::instances[i]->acquireData();
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
    for (uint8_t i = 0; i < FeatureComputer::instanceCount; ++i)
    {
        FeatureComputer::instances[i]->compute();
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
    for (uint8_t i = 0; i < Feature::instanceCount; ++i)
    {
        Feature::instances[i]->getOperationState();
        featState = Feature::instances[i]->getOperationState();
        if ((uint8_t) newState < (uint8_t) featState)
        {
            newState = featState;
        }
    }
    m_operationState = newState;
    // Light the LED to show the state
    uint32_t now = millis();
    if (m_lastLitLedTime > now || m_lastLitLedTime + showOpStateTimer < now)
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
        default:
            if (loopDebugMode)
            {
                debugPrint(F("StreamingMode not handled: "), false);
                debugPrint(m_streamingMode);
            }
    }
    double timestamp = getDatetime();
    float batteryLoad = iuBattery.getBatteryLoad();
    for (uint8_t i = 0; i < FeatureGroup::instanceCount; ++i)
    {
        // TODO Switch to new streaming format once the backend is ready
//        FeatureGroup::instances[i]->stream(port, m_operationState, timestamp);
        FeatureGroup::instances[i]->legacyStream(
            port, m_macAddress, m_operationState, batteryLoad, timestamp);
    }


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
            beginDataAcquisition();
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
            deactivateAllGroups();
            activateGroup(&calibrationGroup);
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
            deactivateAllGroups();
            activateGroup(&motorStandardGroup);
            accelRMS512Total.enableOperationState();
            accelRMS512Total.setThresholds(110, 130, 150);
//            accelRMS512X.enableOperationState();
//            accelRMS512Total.setThresholds(0.05, 1.2, 1.8);
//            accelRMS512Y.enableOperationState();
//            accelRMS512Total.setThresholds(0.05, 1.2, 1.8);
//            accelRMS512Z.enableOperationState();
//            accelRMS512Total.setThresholds(0.05, 1.2, 1.8);
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

