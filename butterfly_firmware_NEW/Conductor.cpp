#include "Conductor.h"


char Conductor::START_CONFIRM[12] = "IUCMD_START";
char Conductor::END_CONFIRM[10] = "IUCMD_END";

/* =============================================================================
    Constructors and destructors
============================================================================= */

Conductor::Conductor(const char* macAddress) :
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
    m_usageMode(UsageMode::COUNT),
    m_acquisitionMode(AcquisitionMode::NONE),
    m_streamingMode(StreamingMode::COUNT)
{
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
    Serial Reading & command processing
============================================================================= */

/**
 * Read from USB and process the command, if one was received.
 */
void Conductor::readFromSerial(IUSerial *iuSerial)
{
    while(true)
    {
        iuSerial->readToBuffer();
        if (!iuSerial->hasNewMessage())
        {
            break;
        }
        char *buffer = iuSerial->getBuffer();
        // Buffer Size, including NUL char '\0'
        uint16_t buffSize =  iuSerial->getCurrentBufferLength();
        if(setupDebugMode)
        {
            debugPrint(F("USB input is: "), false);
            debugPrint(buffer);
        }
        if (buffer[0] == '{' && buffer[buffSize - 1] == '}')
        {
            processConfiguration(buffer);
        }
        else
        {
            // Also check for legacy commands
            switch (iuSerial->interface)
            {
                case InterfaceType::INT_USB:
                    processLegacyUSBCommands(buffer);
                    break;
                case InterfaceType::INT_BLE:
                    processLegacyBLECommands(buffer);
                    break;
                default:
                    if (loopDebugMode)
                    {
                        debugPrint(F("Unhandled interface type: "), false);
                        debugPrint(iuSerial->interface);
                    }
            }
        }
        iuSerial->resetBuffer();    // Clear wire buffer
    }
}

/**
 * Parse the given config json and apply the configuration
 */
bool Conductor::processConfiguration(char *json)
{
    JsonObject& root = jsonBuffer.parseObject(json);
    if (!root.success())
    {
        if (debugMode)
        {
            debugPrint("parseObject() failed");
        }
        return false;
    }
    // Device level configuration
    bool valid = false;
    JsonVariant sub_config = root["device"];
    if (sub_config.success())
    {
        valid = configureMainOptions(sub_config);
    }
    if (!valid)
    {
        return false;
    }
    // Component configuration
    sub_config = root["components"];
    if (sub_config.success())
    {
        configureAllSensors(sub_config);
    }
    // Feature configuration
    sub_config = root["features"];
    if (sub_config.success())
    {
        configureAllFeatures(sub_config);
    }
    return true;
}

/**
 * Device level configuration
 *
 * @return True if the configuration is valid, else false.
 */
bool Conductor::configureMainOptions(JsonVariant &my_config)
{
    // Firmware and config version check
    const char* vers = my_config["VERS"].as<char*>();
    if (vers == NULL || strcmp(FIRMWARE_VERSION, vers) > 0)
    {
        if (debugMode)
        {
            debugPrint(F("Config error: received config version '"), false);
            debugPrint(vers, false);
            debugPrint(F("', expected '"), false);
            debugPrint(FIRMWARE_VERSION, false);
            debugPrint(F("'"));
        }
        return false;
    }
    JsonVariant value = my_config["GRP"];
    if (value.success())
    {
        deactivateAllGroups();
        activateGroup(&healthCheckGroup);  // Heartbeat is always active
        FeatureGroup *group;
        const char* groupName;
        for (uint8_t i = 0; i < FeatureGroup::instanceCount; ++i)
        {
            groupName = value[i];
            if (groupName != NULL)
            {
                group = FeatureGroup::getInstanceByName(groupName);
                if (group)
                {
                    activateGroup(group);
                }
            }
        }
    }
    // Sleep management
    bool resetCycleTime = false;
    value = my_config["POW"];
    if (value.success())
    {
        m_sleepMode = (sleepMode) (value.as<int>());
        resetCycleTime = true;
    }
    value = my_config["TSL"];
    if (value.success())
    {
        m_autoSleepDelay = (uint32_t) (value.as<int>());
        resetCycleTime = true;
    }
    value = my_config["TOFF"];
    if (value.success())
    {
        m_sleepDuration = (uint32_t) (value.as<int>());
        resetCycleTime = true;
    }
    value = my_config["TCY"];
    if (value.success())
    {
        m_cycleTime = (uint32_t) (value.as<int>());
        resetCycleTime = true;
    }
    if (resetCycleTime)
    {
        m_startTime = millis();
    }
    return true;
}

/**
 * Apply the given config to the designated sensors.
 */
void Conductor::configureAllSensors(JsonVariant &config)
{
    JsonVariant my_config;
    for (uint8_t i = 0; i < Sensor::instanceCount; ++i)
    {
        my_config = config[Sensor::instances[i]->getName()];
        if (my_config.success())
        {
            Sensor::instances[i]->configure(my_config);
        }
    }
}

/**
 * Apply the given config to the designated features.
 */
void Conductor::configureAllFeatures(JsonVariant &config)
{
    JsonVariant my_config;
    Feature *feature;
    FeatureComputer *computer;
    for (uint8_t i = 0; i < Feature::instanceCount; ++i)
    {
        feature = Feature::instances[i];
        /* Disable all operationStates by default, they will be reactivated in
        feature->configure if needed. */
        feature->disableOperationState();
        my_config = config[feature->getName()];
        if (my_config.success())
        {
            feature->configure(my_config);  // Configure the feature
            computer = FeatureComputer::getInstanceById(
                feature->getComputerId());
            computer->configure(my_config);  // Configure the computer
            if (debugMode)
            {
                debugPrint(F("Configured feature "), false);
                debugPrint(feature->getName());
            }
        }
    }
}

/**
 * Process the USB commands (legacy)
 */
void Conductor::processLegacyUSBCommands(char *buff)
{
    // Usage mode Mode switching
    char *result = NULL;
    switch (m_usageMode)
    {
        case UsageMode::CALIBRATION:
            if (strcmp(buff, "IUCAL_END") == 0)
            {
                iuUSB.port->println(END_CONFIRM);
                changeUsageMode(UsageMode::OPERATION);
            }
            break;
        case UsageMode::EXPERIMENT:
            if (strcmp(buff, "IUCMD_END") == 0)
            {
                iuUSB.port->println(END_CONFIRM);
                changeUsageMode(UsageMode::OPERATION);
                return;
            }
            result = strstr(buff, "Arange");
            if (result != NULL)
            {
                switch (result[7] - '0')
                {
                    case 0:
                        iuAccelerometer.setScale(iuAccelerometer.AFS_2G);
                        break;
                    case 1:
                        iuAccelerometer.setScale(iuAccelerometer.AFS_4G);
                        break;
                    case 2:
                        iuAccelerometer.setScale(iuAccelerometer.AFS_8G);
                        break;
                    case 3:
                        iuAccelerometer.setScale(iuAccelerometer.AFS_16G);
                        break;
                }
                return;
            }
            result = strstr(buff, "rgb");
            if (result != NULL)
            {
                iuRGBLed.unlock();
                iuRGBLed.changeColor((bool) (result[7] - '0'),
                                     (bool) (result[8] - '0'),
                                     (bool) (result[9] - '0'));
                iuRGBLed.lock();
                return;
            }
            result = strstr(buff, "acosr");
            if (result != NULL)
            {
                // Change audio sampling rate
                int A = result[6] - '0';
                int B = result[7] - '0';
                uint16_t samplingRate = (uint16_t) ((A * 10 + B) * 1000);
                iuI2S.setSamplingRate(samplingRate);
                return;
            }
            result = strstr(buff, "accsr");
            if (result != NULL)
            {
                int A = result[6] - '0';
                int B = result[7] - '0';
                int C = result[8] - '0';
                int D = result[9] - '0';
                int samplingRate = (A * 1000 + B * 100 + C * 10 + D);
                iuAccelerometer.setSamplingRate(samplingRate);
            }
            break;
        case UsageMode::OPERATION:
            if (strcmp(buff, "IUCAL_START") == 0)
            {
                iuUSB.port->println(START_CONFIRM);
                changeUsageMode(UsageMode::CALIBRATION);
            }
            if (strcmp(buff, "IUCMD_START") == 0)
            {
                iuUSB.port->println(START_CONFIRM);
                changeUsageMode(UsageMode::EXPERIMENT);
            }
            break;
        default:
            if (loopDebugMode)
            {
                debugPrint(F("Unhandled usage mode: "), false);
                debugPrint(m_usageMode);
            }
    }
}

/**
 * Process the instructions sent over Bluetooth
 */
void Conductor::processLegacyBLECommands(char *buff)
{
    if (m_streamingMode == StreamingMode::WIRED)
    {
        return;  // Do not listen to BLE when wired
    }
    switch(buff[0])
    {
        case '1':  // Receive the timestamp data from the bluetooth hub
            if (buff[1] == ':' && buff[12] == '.')
            {
                int flag(0), ts(0), ms(0);
                sscanf(buff, "%d:%d.%d", &flag, &ts, &ms);
                setRefDatetime((double) ts + (double) ms / (double) 1000000);
            }
            break;
        case '5':  // Get status
            if (buff[7] == '0' && buff[9] == '0' && buff[11] == '0' &&
                buff[13] == '0' && buff[15] == '0' && buff[17] == '0')
            {
                iuBluetooth.port->print("HB,");
                iuBluetooth.port->print(conductor.getMacAddress());
                iuBluetooth.port->print(",");
                if (iuI2C.isError())
                {
                    iuBluetooth.port->print("I2CERR");
                }
                else
                {
                    iuBluetooth.port->print("ALL_OK");
                }
                iuBluetooth.port->print(";");
                iuBluetooth.port->flush();
            }
            break;
        case '0':  // DEPRECATED - Set feature thresholds
        case '2':  // DEPRECATED - Bluetooth parameter setting
        case '3':  // DEPRECATED - Collect acceleration raw data
//            if (buff[7] == '0' && buff[9] == '0' && buff[11] == '0' &&
//                buff[13] == '0' && buff[15] == '0' && buff[17] == '0')
//            {
//                if (loopDebugMode)
//                {
//                    debugPrint("Record mode");
//                }
//                Feature *feature = Feature::getInstanceByName("A0X");
//                if (feature)
//                {
//                    feature.stream(iuBluetooth.port);
//                }
//                feature = Feature::getInstanceByName("A0Y");
//                if (feature)
//                {
//                    feature.stream(iuBluetooth.port);
//                }
//                feature = Feature::getInstanceByName("A0Z");
//                if (feature)
//                {
//                    feature.stream(iuBluetooth.port);
//                }
//            }
//            break;
        case '4':  // DEPRECATED - Exit record mode
        case '6': // DEPRECATED - Set which feature are used for OperationState
        default:
            if (debugMode)
            {
                debugPrint(F("BLE command is DEPRECATED"));
            }
            break;
    }
}


/* =============================================================================
    Features and groups Management
============================================================================= */

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
    Mode management
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
    Serial.print("Current acquisition mode: ");
    Serial.println(m_acquisitionMode);
    if (m_acquisitionMode == mode)
    {
        Serial.println("m_acquisitionMode already");
        return; // Nothing to do
    }
    Serial.println("m_acquisitionMode changed");
    m_acquisitionMode = mode;
    switch (m_acquisitionMode)
    {
        case AcquisitionMode::RAWDATA:
            iuRGBLed.changeColor(IURGBLed::CYAN);
            resetDataAcquisition();
            break;
        case AcquisitionMode::FEATURE:
            iuRGBLed.changeColor(IURGBLed::BLUE);
            resetDataAcquisition();
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
    Serial.print("Current usage mode: ");
    Serial.println(m_usageMode);
    if (m_usageMode == usage)
    {
        Serial.println("Already in mode");
        return; // Nothing to do
    }
    Serial.println("Changing mode");
    m_usageMode = usage;
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
            iuRGBLed.changeColor(IURGBLed::PURPLE);
            iuRGBLed.lock();
            break;
        case UsageMode::OPERATION:
            iuRGBLed.unlock();
            iuRGBLed.changeColor(IURGBLed::BLUE);
            deactivateAllGroups();
            activateGroup(&healthCheckGroup);
            activateGroup(&motorStandardGroup);
            // TODO - Set up default feature thresholds - Remove?
            accelRMS512Total.enableOperationState();
            accelRMS512Total.setThresholds(110, 130, 150);
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
    Serial.print("New acquisition mode: ");
    Serial.println(UsageMode::acquisitionModeDetails[m_usageMode]);
    changeAcquisitionMode(UsageMode::acquisitionModeDetails[m_usageMode]);
    Serial.print("New streaming mode: ");
    Serial.println(UsageMode::streamingModeDetails[m_usageMode]);
    changeStreamingMode(UsageMode::streamingModeDetails[m_usageMode]);
    if (loopDebugMode)
    {
        debugPrint("\nSet up for " + msg + "\n");
    }
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
    for (uint8_t i = 0; i < Feature::instanceCount; ++i)
    {
        Feature::instances[i]->reset();
    }
    // Delay to make sure that the I2S callback function is called one more time
    delay(500);
}

/**
 * End and restart the data acquisition
 *
 * @return the output of beginDataAcquisition (if it was successful or not)
 */
bool Conductor::resetDataAcquisition()
{
    Serial.println("resetDataAcquisition");
    endDataAcquisition();
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
    // If EXPERIMENT mode, send last data batch before collecting the new data
    if (m_usageMode == UsageMode::EXPERIMENT)
    {
        if (loopDebugMode && (m_acquisitionMode != AcquisitionMode::RAWDATA ||
                              m_streamingMode != StreamingMode::WIRED))
        {
            debugPrint(F("EXPERIMENT should be RAW DATA + WIRED mode."));
        }
        if (asynchronous)
        {
            Serial.println("async dd");
            iuI2S.sendData(iuUSB.port);
            iuAccelerometer.sendData(iuUSB.port);
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
    Debugging
============================================================================= */

/**
 * Expose current configurations
 */
void Conductor::exposeAllConfigurations()
{
    #ifdef DEBUGMODE
    for (uint8_t i = 0; i < Sensor::instanceCount; ++i)
    {
        Sensor::instances[i]->expose();
    }
    debugPrint("");
    for (uint8_t i = 0; i < Feature::instanceCount; ++i)
    {
        Feature::instances[i]->exposeConfig();
        Feature::instances[i]->exposeCounters();
        debugPrint("_____");
    }
    debugPrint("");
    for (uint8_t i = 0; i < FeatureComputer::instanceCount; ++i)
    {
        FeatureComputer::instances[i]->exposeConfig();
    }
    #endif
}
