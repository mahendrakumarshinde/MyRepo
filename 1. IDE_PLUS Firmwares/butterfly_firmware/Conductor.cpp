#include "Conductor.h"


char Conductor::START_CONFIRM[11] = "IUOK_START";
char Conductor::END_CONFIRM[9] = "IUOK_END";

IUFlash::storedConfig Conductor::CONFIG_TYPES[Conductor::CONFIG_TYPE_COUNT] = {
    IUFlash::CFG_DEVICE,
    IUFlash::CFG_COMPONENT,
    IUFlash::CFG_FEATURE};

/* =============================================================================
    Hardware & power management
============================================================================= */


/**
 * Put device in a light sleep / low power mode.
 *
 * The STM32 will be put to sleep (reduced power consumption but fast start up).
 *
 * Wake up time is around 100ms
 */
void Conductor::sleep(uint32_t duration)
{
    for (uint8_t i = 0; i < Sensor::instanceCount; ++i)
    {
        Sensor::instances[i]->setPowerMode(PowerMode::SLEEP);
    }
    rgbLed.setPowerMode(PowerMode::SLEEP);
    STM32.stop(duration);
    rgbLed.setPowerMode(PowerMode::REGULAR);
    for (uint8_t i = 0; i < Sensor::instanceCount; ++i)
    {
        Sensor::instances[i]->setPowerMode(PowerMode::REGULAR);
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
    iuBluetooth.setPowerMode(PowerMode::SUSPEND);
    iuWiFi.setPowerMode(PowerMode::SUSPEND);
    for (uint8_t i = 0; i < Sensor::instanceCount; ++i)
    {
        Sensor::instances[i]->setPowerMode(PowerMode::SUSPEND);
    }
    rgbLed.setPowerMode(PowerMode::SUSPEND);
    STM32.stop(duration * 1000);
    rgbLed.setPowerMode(PowerMode::REGULAR);
    iuBluetooth.setPowerMode(PowerMode::REGULAR);
    iuWiFi.setPowerMode(PowerMode::REGULAR);
    for (uint8_t i = 0; i < Sensor::instanceCount; ++i)
    {
        Sensor::instances[i]->setPowerMode(PowerMode::REGULAR);
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
    if (m_sleepMode == sleepMode::AUTO && now - m_startTime > m_autoSleepDelay)
    {
        sleep(m_sleepDuration);
    }
    else if (m_sleepMode == sleepMode::PERIODIC &&
             now - m_startTime > m_cycleTime - m_sleepDuration)
    {
        suspend(m_sleepDuration);
        m_startTime = now;
    }
}


/* =============================================================================
    Led colors
============================================================================= */


void Conductor::resetLed()
{
    rgbLed.unlockColors();
    m_colorSequence[0] = RGB_BLACK;
    m_colorFadeIns[0] = 0;
    m_colorDurations[0] = 3000;
    if (iuWiFi.isConnected())
    {
        m_colorSequence[1] = RGB_WHITE;
        m_colorFadeIns[1] = 0;
        m_colorDurations[1] = 100;
    }
    else
    {
        m_colorSequence[1] = RGB_BLACK;
        m_colorFadeIns[1] = 0;
        m_colorDurations[1] = 0;
    }
    rgbLed.startNewColorQueue(2, m_colorSequence, m_colorFadeIns,
                              m_colorDurations);
    showOperationStateOnLed();
}

void Conductor::overrideLedColor(RGBColor color)
{
    rgbLed.unlockColors();
    rgbLed.deleteColorQueue();
    rgbLed.queueColor(color, 0, 10000);
    rgbLed.lockColors();
}

void Conductor::showOperationStateOnLed()
{
    if (rgbLed.lockedColors())
    {
        return;
    }
    switch (m_operationState)
    {
        case OperationState::IDLE:
            m_colorSequence[0] = RGB_BLUE;
            break;
        case OperationState::NORMAL:
            m_colorSequence[0] = RGB_GREEN;
            break;
        case OperationState::WARNING:
            m_colorSequence[0] = RGB_ORANGE;
            break;
        case OperationState::DANGER:
            m_colorSequence[0] = RGB_RED;
            break;
    }
    rgbLed.replaceColor(0, m_colorSequence[0], m_colorFadeIns[0],
                        m_colorDurations[0]);
}

void Conductor::showStatusOnLed(RGBColor color)
{
    m_colorSequence[0] = RGB_BLACK;
    m_colorFadeIns[0] = 25;
    m_colorDurations[0] = 50;
    m_colorSequence[1] = color;
    m_colorFadeIns[1] = 25;
    m_colorDurations[1] = 50;
    rgbLed.startNewColorQueue(2, m_colorSequence, m_colorFadeIns,
                              m_colorDurations);
}


/* =============================================================================
    Local storage (flash) management
============================================================================= */

/**
 * Update a config from flash storage.
 *
 * Accepted config types are:
 *  - IUFlash::CFG_DEVICE
 *  - IUFlash::CFG_COMPONENT
 *  - IUFlash::CFG_FEATURE
 * @return bool success
 */
bool Conductor::configureFromFlash(IUFlash::storedConfig configType)
{
    StaticJsonBuffer<JSON_BUFFER_SIZE> jsonBuffer;
    JsonVariant config = JsonVariant(
            iuFlash.loadConfigJson(configType, jsonBuffer));
    bool success = config.success();
    if (success)
    {
        switch (configType)
        {
            case IUFlash::CFG_DEVICE:
                configureMainOptions(config);
                break;
            case IUFlash::CFG_COMPONENT:
                configureAllSensors(config);
                break;
            case IUFlash::CFG_FEATURE:
                configureAllFeatures(config);
                break;
            case IUFlash::CFG_WIFI0:
            case IUFlash::CFG_WIFI1:
            case IUFlash::CFG_WIFI2:
            case IUFlash::CFG_WIFI3:
            case IUFlash::CFG_WIFI4:
                iuWiFi.configure(config);
                break;
            default:
                if (debugMode)
                {
                    debugPrint("Unhandled config type: ", false);
                    debugPrint((uint8_t) configType);
                }
                success = false;
                break;
        }
    }
    if (debugMode && success)
    {
        debugPrint("Successfully loaded config type #", false);
        debugPrint((uint8_t) configType);
    }
    return success;
}

/**
 *
 */
void Conductor::sendConfigChecksum(IUFlash::storedConfig configType)
{
    if (!(m_streamingMode == StreamingMode::WIFI ||
          m_streamingMode == StreamingMode::WIFI_AND_BLE))
    {
        if (debugMode)
        {
            debugPrint("Config checksum can only be sent via WiFi.");
        }
        return;
    }
    size_t configMaxLen = 200;
    char config[configMaxLen];
    strcpy(config, "");
    size_t charCount = 0;
    if (iuFlash.available())
    {
        charCount = iuFlash.readConfig(configType, config, configMaxLen);
    }
    // Charcount = 0 if flash is unavailable or file not found => Checksum of
    // empty string will be sent, and that will trigger a config refresh
    unsigned char* md5hash = MD5::make_hash(config, charCount);
    char *md5str = MD5::make_digest(md5hash, 16);
    size_t md5Len = 33;
    size_t fullStrLen = md5Len + 44;
    char fullStr[fullStrLen];
    for (size_t i = 0; i < fullStrLen; i++)
    {
        fullStr[i] = 0;
    }
    int written = -1;
    switch (configType)
    {
        case IUFlash::CFG_DEVICE:
            written = snprintf(
                fullStr, fullStrLen, "{\"mac\":\"%s\",\"main\":\"%s\"}",
                m_macAddress.toString().c_str(), md5str);
            break;
        case IUFlash::CFG_COMPONENT:
            written = snprintf(
                fullStr, fullStrLen, "{\"mac\":\"%s\",\"components\":\"%s\"}",
                m_macAddress.toString().c_str(), md5str);
            break;
        case IUFlash::CFG_FEATURE:
            written = snprintf(
                fullStr, fullStrLen, "{\"mac\":\"%s\",\"features\":\"%s\"}",
                m_macAddress.toString().c_str(), md5str);
            break;
        default:
            if (debugMode)
            {
                debugPrint("Unhandled config type: ", false);
                debugPrint((uint8_t) configType);
            }
            break;
    }
    if (written > 0 && written < fullStrLen)
    {
        // Send checksum
        iuWiFi.sendMSPCommand(MSPCommand::PUBLISH_CONFIG_CHECKSUM, fullStr,
                              written);
    }
    else if (loopDebugMode)
    {
        debugPrint("Failed to format config checksum: ", false);
        debugPrint(fullStr);
    }
    //free memory
    free(md5hash);
    free(md5str);
}

/**
 *
 */
void Conductor::periodicSendConfigChecksum()
{
    if (m_streamingMode == StreamingMode::WIFI ||
         m_streamingMode == StreamingMode::WIFI_AND_BLE)
    {
        uint32_t now = millis();
        if (now - m_configTimerStart > SEND_CONFIG_CHECKSUM_TIMER)
        {
            sendConfigChecksum(CONFIG_TYPES[m_nextConfigToSend]);
            m_nextConfigToSend++;
            m_nextConfigToSend %= CONFIG_TYPE_COUNT;
            m_configTimerStart = now;
        }
    }
}


/* =============================================================================
    Serial Reading & command processing
============================================================================= */

/**
 * Read from serial and process the command, if one was received.
 */
void Conductor::readFromSerial(StreamingMode::option interfaceType,
                               IUSerial *iuSerial)
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
        if (loopDebugMode && iuSerial->getProtocol() != IUSerial::MS_PROTOCOL)
        {
            debugPrint(millis(), false);
            debugPrint(F(": Interface "), false);
            debugPrint(interfaceType, false);
            debugPrint(F(" input is: "), false);
            debugPrint(buffer);
        }
        switch (interfaceType)
        {
            case StreamingMode::WIRED:
                processUSBMessages(buffer);
                break;
            case StreamingMode::BLE:
                processBLEMessages(buffer);
                m_lastBLEmessage = millis();
                updateStreamingMode();
                break;
            case StreamingMode::WIFI:
                processWIFIMessages(buffer);
                break;
            default:
                if (loopDebugMode)
                {
                    debugPrint(F("Unhandled interface type: "), false);
                    debugPrint(interfaceType);
                }
        }
        iuSerial->resetBuffer();  // Clear buffer
    }
}

/**
 * Parse the given config json and apply the configuration
 */
bool Conductor::processConfiguration(char *json, bool saveToFlash)
{
    StaticJsonBuffer<JSON_BUFFER_SIZE> jsonBuffer;
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
    JsonVariant subConfig = root["main"];
    if (subConfig.success())
    {
        configureMainOptions(subConfig);
        if (saveToFlash)
        {
            iuFlash.saveConfigJson(IUFlash::CFG_DEVICE, subConfig);
        }
    }
    // Component configuration
    subConfig = root["components"];
    if (subConfig.success())
    {
        configureAllSensors(subConfig);
        if (saveToFlash)
        {
            iuFlash.saveConfigJson(IUFlash::CFG_COMPONENT, subConfig);
        }
    }
    // Feature configuration
    subConfig = root["features"];
    if (subConfig.success())
    {
        configureAllFeatures(subConfig);
        if (saveToFlash)
        {
            iuFlash.saveConfigJson(IUFlash::CFG_FEATURE, subConfig);
        }
    }
    return true;
}

/**
 * Device level configuration
 *
 * @return True if the configuration is valid, else false.
 */
void Conductor::configureMainOptions(JsonVariant &config)
{
    JsonVariant value = config["GRP"];
    if (value.success())
    {
        deactivateAllGroups();
        bool mainGroupFound = false;
        // activateGroup(&healthCheckGroup);  // Health check is always active
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
                    if (!mainGroupFound)
                    {
                        m_mainFeatureGroup = group;
                        mainGroupFound = true;
                    }
                }
            }
        }
    }
    // Raw Data publication period
    value = config["RAW"];
    if (value.success())
    {
        m_rawDataPublicationTimer = (uint32_t) (value.as<int>()) * 1000;
    }
    // Sleep management
    bool resetCycleTime = false;
    value = config["POW"];
    if (value.success())
    {
        m_sleepMode = (sleepMode) (value.as<int>());
        resetCycleTime = true;
    }
    value = config["TSL"];
    if (value.success())
    {
        m_autoSleepDelay = (uint32_t) (value.as<int>()) * 1000;
        resetCycleTime = true;
    }
    value = config["TOFF"];
    if (value.success())
    {
        m_sleepDuration = (uint32_t) (value.as<int>()) * 1000;
        resetCycleTime = true;
    }
    value = config["TCY"];
    if (value.success())
    {
        m_cycleTime = (uint32_t) (value.as<int>()) * 1000;
        resetCycleTime = true;
    }
    if (resetCycleTime)
    {
        m_startTime = millis();
    }
}

/**
 * Apply the given config to the designated sensors.
 */
void Conductor::configureAllSensors(JsonVariant &config)
{
    JsonVariant myConfig;
    for (uint8_t i = 0; i < Sensor::instanceCount; ++i)
    {
        myConfig = config[Sensor::instances[i]->getName()];
        if (myConfig.success())
        {
            Sensor::instances[i]->configure(myConfig);
        }
    }
}

/**
 * Apply the given config to the designated features.
 */
void Conductor::configureAllFeatures(JsonVariant &config)
{
    JsonVariant myConfig;
    Feature *feature;
    FeatureComputer *computer;
    for (uint8_t i = 0; i < Feature::instanceCount; ++i)
    {
        feature = Feature::instances[i];
        /* Disable all operationStates by default, they will be reactivated in
        feature->configure if needed. */
        feature->disableOperationState();
        myConfig = config[feature->getName()];
        if (myConfig.success())
        {
            feature->configure(myConfig);  // Configure the feature
            computer = FeatureComputer::getInstanceById(
                feature->getComputerId());
            if (computer)
            {
                computer->configure(myConfig);  // Configure the computer
            }
            if (debugMode)
            {
                debugPrint(F("Configured feature "), false);
                debugPrint(feature->getName());
            }
        }
    }
}

/**
 * Process the commands
 */
void Conductor::processCommands(char *buff)
{
    switch(buff[0])
    {
        case 'A': // ping device
            if (strcmp(buff, "ALIVE") == 0)
            {
                // Blink the device to let the user know that is is live
                showStatusOnLed(RGB_WHITE);
                uint32_t startT = millis();
                uint32_t current = startT;
                while (current - startT < 3000)
                {
                    rgbLed.manageColorTransitions();
                    delay(10);
                    current = millis();
                }
                resetLed();
            }
            break;
        case 'G': // set feature group
            if (strncmp(buff, "GROUP-", 6) == 0)
            {
                FeatureGroup *group = FeatureGroup::getInstanceByName(&buff[6]);
                if (group)
                {
                    deactivateAllGroups();
                    m_mainFeatureGroup = group;
                    activateGroup(m_mainFeatureGroup);
                    if (loopDebugMode)
                    {
                        debugPrint("New feature group activated: ", false);
                        debugPrint(m_mainFeatureGroup->getName());
                    }
                }
            }
            break;
        case 'I': // ping device
            if (strcmp(buff, "IDE-HARDRESET") == 0)
            {
                iuBluetooth.write("WIFI-DISCONNECTED;");
                delay(10);
                STM32.reset();
            }
            else if (strcmp(buff, "IDE-GET-VERSION") == 0)
            {
                iuBluetooth.write("IDE-VERSION-");
                iuBluetooth.write(FIRMWARE_VERSION);
                iuBluetooth.write(';');
            }
            break;
        case '3':  // Collect acceleration raw data
            if (buff[7] == '0' && buff[9] == '0' && buff[11] == '0' &&
                buff[13] == '0' && buff[15] == '0' && buff[17] == '0')
            {
                if (loopDebugMode)
                {
                    debugPrint("Record mode");
                }
                sendAccelRawData(0);  // Axis X
                sendAccelRawData(1);  // Axis Y
                sendAccelRawData(2);  // Axis Z
            }
            break;
        default:
            break;
    }
}


/**
 * Process the legacy commands
 */
void Conductor::processLegacyCommands(char *buff)
{
    // TODO Command protocol redefinition required
    switch(buff[0])
    {
        case '0': // Set Thresholds
            if (buff[4] == '-' && buff[9] == '-' && buff[14] == '-')
            {
                int idx(0), th1(0), th2(0), th3(0);
                sscanf(buff, "%d-%d-%d-%d", &idx, &th1, &th2, &th3);
                Feature *feat = m_mainFeatureGroup->getFeature(idx);
                if (feat)
                {
                    if (idx == 1 || idx == 2 || idx == 3)
                    {
                        feat->setThresholds((float) th1 / 100.,
                                            (float) th2 / 100.,
                                            (float) th3 / 100.);
                    }
                    else
                    {
                        feat->setThresholds((float) th1, (float) th2,
                                            (float) th3);
                    }
                    if (loopDebugMode)
                    {
                        debugPrint(feat->getName(), false);
                        debugPrint(':', false);
                        debugPrint(feat->getThreshold(0), false);
                        debugPrint(" - ", false);
                        debugPrint(feat->getThreshold(1), false);
                        debugPrint(" - ", false);
                        debugPrint(feat->getThreshold(2));
                    }
                }
            }
            break;
        case '1':  // Receive the timestamp data from the bluetooth hub
            if (buff[1] == ':' && buff[12] == '.')
            {
                setRefDatetime(&buff[2]);
            }
            break;
        case '2':  // Bluetooth parameter setting
            if (buff[1] == ':' && buff[7] == '-' && buff[13] == '-')
            {
                int dataRecTimeout(0), paramtag(0);
                int startSleepTimer(0), dataSendPeriod(0);
                sscanf(buff, "%d:%d-%d-%d", &paramtag, &dataSendPeriod,
                       startSleepTimer, &dataRecTimeout);
                // We currently only use the data send period option
                m_mainFeatureGroup->setDataSendPeriod(
                    (uint16_t) dataSendPeriod);
            }
            break;
        case '6': // Set which feature are used for OperationState
            if (buff[7] == ':' && buff[9] == '.' && buff[11] == '.' &&
                buff[13] == '.' && buff[15] == '.' && buff[17] == '.')
            {
                int parametertag(0);
                int fcheck[6] = {0, 0, 0, 0, 0, 0};
                sscanf(buff, "%d:%d.%d.%d.%d.%d.%d", &parametertag, &fcheck[0],
                       &fcheck[1], &fcheck[2], &fcheck[3], &fcheck[4],
                       &fcheck[5]);
                Feature *feat;
                for (uint8_t i = 0; i < 6; i++)
                {
                    feat = m_mainFeatureGroup->getFeature(i);
                    if (feat)
                    {
                        if (fcheck[i] > 0)
                        {
                            feat->enableOperationState();
                        }
                        else
                        {
                            feat->disableOperationState();
                        }
                    }
                }
            }
            break;
        default:
            break;
    }
}

/**
 * Process the USB commands
 */
void Conductor::processUSBMessages(char *buff)
{
    if (buff[0] == '{')
    {
        processConfiguration(buff, true);
    }
    else if (strncmp(buff, "WIFI-", 5) == 0)
    {
        processUserMessageForWiFi(buff, &iuUSB);
    }
    else if (strncmp(buff, "MCUINFO", 7) == 0)
    {
        streamMCUUInfo(iuUSB.port);
    }
    else
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
                    overrideLedColor(RGBColor(255 * (result[7] - '0'),
                                              255 * (result[8] - '0'),
                                              255 * (result[9] - '0')));
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
}

/**
 * Process the instructions sent over Bluetooth
 */
void Conductor::processBLEMessages(char *buff)
{
    if (m_streamingMode == StreamingMode::WIRED)
    {
        return;  // Do not listen to BLE when wired
    }
    if (buff[0] == '{')
    {
        processConfiguration(buff, true);
    }
    else if (strncmp(buff, "WIFI-", 5) == 0)
    {
        processUserMessageForWiFi(buff, &iuBluetooth);
    }
    else
    {
        processCommands(buff);
        processLegacyCommands(buff);
    }
}

/**
 *
 */
void Conductor::processUserMessageForWiFi(char *buff,
                                          IUSerial *feedbackSerial)
{
    if (strncmp(buff, "WIFI-GET-MAC", 13) == 0)
    {
        if ((uint64_t) iuWiFi.getMacAddress() > 0)
        {
            feedbackSerial->write("WIFI-MAC-");
            feedbackSerial->write(iuWiFi.getMacAddress().toString().c_str());
            feedbackSerial->write(';');
        }
    }
    else
    if (strcmp(buff, "WIFI-DISABLE") == 0)
    {
        iuWiFi.setPowerMode(PowerMode::DEEP_SLEEP);
        updateStreamingMode();
    }
    else
    {
        iuWiFi.setPowerMode(PowerMode::REGULAR);
        // We want the WiFi to do something, so need to make sure it's available
        if (!iuWiFi.isAvailable())
        {
            showStatusOnLed(RGB_PURPLE); // Show the status to the user
            uint32_t startT = millis();
            uint32_t current = startT;
            // Wait for up to 3sec the WiFi wake up
            while (!iuWiFi.isAvailable() && current - startT < 3000)
            {
                readFromSerial(StreamingMode::WIFI, &iuWiFi);
                rgbLed.manageColorTransitions();
                delay(10);
                current = millis();
            }
            resetLed();
        }
        // Process message
        iuWiFi.processUserMessage(buff, &iuFlash);
        // Show status
        if (iuWiFi.isAvailable() && iuWiFi.isWorking())
        {
            showStatusOnLed(RGB_PURPLE);
        }
    }
}

/**
 * Process the instructions from the WiFi chip
 */
void Conductor::processWIFIMessages(char *buff)
{
    if (iuWiFi.processChipMessage())
    {
        if (iuWiFi.isWorking()) {
            showStatusOnLed(RGB_PURPLE);
        } else {
            resetLed();
        }
        updateStreamingMode();
    }
    switch (iuWiFi.getMspCommand())
    {
        case MSPCommand::ASK_BLE_MAC:
            if (loopDebugMode) { debugPrint("ASK_BLE_MAC"); }
            iuWiFi.sendBleMacAddress(m_macAddress);
            break;
        case MSPCommand::WIFI_ALERT_CONNECTED:
            if (loopDebugMode) { debugPrint("WIFI-CONNECTED;"); }
            iuBluetooth.write("WIFI-CONNECTED;");
            break;
        case MSPCommand::WIFI_ALERT_DISCONNECTED:
            iuBluetooth.write("WIFI-DISCONNECTED;");
            break;
        case MSPCommand::WIFI_ALERT_NO_SAVED_CREDENTIALS:
            if (loopDebugMode)
            {
                debugPrint("WIFI_ALERT_NO_SAVED_CREDENTIALS");
            }
            iuBluetooth.write("WIFI-NOSAVEDCRED;");
            break;
        case MSPCommand::SET_DATETIME:
            if (loopDebugMode) { debugPrint("SET_DATETIME"); }
            setRefDatetime(buff);
            break;
        case MSPCommand::CONFIG_FORWARD_CONFIG:
            if (loopDebugMode) { debugPrint("CONFIG_FORWARD_CONFIG"); }
            processConfiguration(buff, true);
            break;
        case MSPCommand::CONFIG_FORWARD_CMD:
            if (loopDebugMode) { debugPrint("CONFIG_FORWARD_CMD"); }
            processCommands(buff);
            break;
        case MSPCommand::CONFIG_FORWARD_LEGACY_CMD:
            if (loopDebugMode) { debugPrint("CONFIG_FORWARD_LEGACY_CMD"); }
            processCommands(buff);
            processLegacyCommands(buff);
            break;
        case MSPCommand::WIFI_CONFIRM_ACTION:
            if (loopDebugMode)
            {
                debugPrint("WIFI_CONFIRM_ACTION: ", false);
                debugPrint(buff);
            }
            break;
        default:
            // pass
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

/**
 *
 */
void Conductor::configureGroupsForOperation()
{
    if (!configureFromFlash(IUFlash::CFG_DEVICE))
    {
        // Config not found, default to mainFeatureGroup
        deactivateAllGroups();
        activateGroup(m_mainFeatureGroup);

    }
    if (!configureFromFlash(IUFlash::CFG_FEATURE))
    {
        // Config not found, default to DEFAULT_ACCEL_ENERGY_THRESHOLDS
        accelRMS512Total.setThresholds(DEFAULT_ACCEL_ENERGY_NORMAL_TH,
                                       DEFAULT_ACCEL_ENERGY_WARNING_TH,
                                       DEFAULT_ACCEL_ENERGY_HIGH_TH);
        accelRMS512Total.enableOperationState();
    }
    // TODO: The following should be written in flash or sent from cloud
    // RMS computer: keep mean
    accel128ComputerX.setRemoveMean(false);
    accel128ComputerY.setRemoveMean(false);
    accel128ComputerZ.setRemoveMean(false);
    // RMS computer: normalize
    accel128ComputerX.setNormalize(true);
    accel128ComputerY.setNormalize(true);
    accel128ComputerZ.setNormalize(true);
    // RMS computer: output is squared
    accel128ComputerX.setSquaredOutput(true);
    accel128ComputerY.setSquaredOutput(true);
    accel128ComputerZ.setSquaredOutput(true);
    // MultiSourceSumComputer: sum 3 axis and don't divide
    accelRMS128TotalComputer.setNormalize(false);
    accelRMS128TotalComputer.setRMSInput(false);
    // SectionSumComputer: normalize
    accel512ComputerX.setNormalize(true);
    accel512ComputerY.setNormalize(true);
    accel512ComputerZ.setNormalize(true);
    accel512TotalComputer.setNormalize(true);
    // SectionSumComputer: output of 128ms feature is squared, not RMS
    accel512ComputerY.setRMSInput(false);
    accel512ComputerX.setRMSInput(false);
    accel512ComputerZ.setRMSInput(false);
    accel512TotalComputer.setRMSInput(false);
}

/**
 *
 */
void Conductor::configureGroupsForCalibration()
{
    deactivateAllGroups();
    activateGroup(&calibrationGroup);
    // RMS computer: remove mean
    accel128ComputerX.setRemoveMean(true);
    accel128ComputerY.setRemoveMean(true);
    accel128ComputerZ.setRemoveMean(true);
    // RMS computer: normalize
    accel128ComputerX.setNormalize(true);
    accel128ComputerY.setNormalize(true);
    accel128ComputerZ.setNormalize(true);
    // RMS computer: output is RMS (not squared)
    accel128ComputerX.setSquaredOutput(false);
    accel128ComputerY.setSquaredOutput(false);
    accel128ComputerZ.setSquaredOutput(false);
    // MultiSourceSumComputer: RMS of 3 axis
    accelRMS128TotalComputer.setNormalize(false);
    accelRMS128TotalComputer.setRMSInput(true);
    // SectionSumComputer: don't normalize (RMS will be used)
    accel512ComputerX.setNormalize(true);
    accel512ComputerY.setNormalize(true);
    accel512ComputerZ.setNormalize(true);
    accel512TotalComputer.setNormalize(true);
    // SectionSumComputer: RMS of 4 128ms features
    accel512ComputerY.setRMSInput(true);
    accel512ComputerX.setRMSInput(true);
    accel512ComputerZ.setRMSInput(true);
    accel512TotalComputer.setRMSInput(true);
}


/* =============================================================================
    Time management
============================================================================= */

/**
 * Set the refDatetime and update the lastSynchrotime to now.
 */
void Conductor::setRefDatetime(double refDatetime)
{
    if (refDatetime >  0)
    {
        m_refDatetime = refDatetime;
        m_lastSynchroTime = millis();
        if (loopDebugMode)
        {
            debugPrint("Time sync: ", false);
            debugPrint(getDatetime());
        }
    }
}

void Conductor::setRefDatetime(const char* timestamp)
{
    setRefDatetime(atof(timestamp));
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

bool Conductor::isBLEConnected()
{
    return (m_lastBLEmessage > 0 &&
            millis() - m_lastBLEmessage < BLEconnectionTimeout);
}


/**
 * Switch to a StreamingMode
 */
void Conductor::updateStreamingMode()
{
    StreamingMode::option newMode = StreamingMode::NONE;
    switch (m_usageMode)
    {
        case UsageMode::CALIBRATION:
        case UsageMode::EXPERIMENT:
            newMode = StreamingMode::WIRED;
            break;
        case UsageMode::OPERATION:
        case UsageMode::OPERATION_BIS:
            if (isBLEConnected()) {
                if (iuWiFi.isConnected()) {
                    newMode = StreamingMode::WIFI_AND_BLE;
                } else {
                    newMode = StreamingMode::BLE;
                }
            } else if (iuWiFi.isConnected()) {
                newMode = StreamingMode::WIFI;
            }
            break;
    }
    if (m_streamingMode == newMode)
    {
        return; // Nothing to do
    }
    m_streamingMode = newMode;
    if (loopDebugMode)
    {
        debugPrint(F("\nStreaming mode is: "), false);
        switch (m_streamingMode)
        {
        case StreamingMode::NONE:
            debugPrint(F("None"));
            break;
        case StreamingMode::WIRED:
            debugPrint(F("USB"));
            break;
        case StreamingMode::BLE:
            debugPrint(F("BLE"));
            break;
        case StreamingMode::WIFI:
            debugPrint(F("WIFI"));
            break;
        case StreamingMode::WIFI_AND_BLE:
            debugPrint(F("WIFI & BLE"));
            break;
        case StreamingMode::STORE:
            debugPrint(F("Flash storage"));
            break;
        default:
            debugPrint(F("Unhandled streaming Mode"));
            break;
        }
    }
}

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
    switch (m_acquisitionMode)
    {
        case AcquisitionMode::RAWDATA:
            overrideLedColor(RGB_CYAN);
            resetDataAcquisition();
            break;
        case AcquisitionMode::FEATURE:
            resetLed();
            resetDataAcquisition();
            break;
        case AcquisitionMode::NONE:
            endDataAcquisition();
            overrideLedColor(RGB_BLACK);
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
 * Switch to a new Usage Mode
 */
void Conductor::changeUsageMode(UsageMode::option usage)
{
    if (m_usageMode == usage)
    {
        return; // Nothing to do
    }
    m_usageMode = usage;
    String msg;
    switch (m_usageMode)
    {
        case UsageMode::CALIBRATION:
            configureGroupsForCalibration();
            overrideLedColor(RGB_CYAN);
            msg = "calibration";
            break;
        case UsageMode::EXPERIMENT:
            msg = "experiment";
            overrideLedColor(RGB_PURPLE);
            break;
        case UsageMode::OPERATION:
        case UsageMode::OPERATION_BIS:
            resetLed();
            configureGroupsForOperation();
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
    changeAcquisitionMode(UsageMode::acquisitionModeDetails[m_usageMode]);
    updateStreamingMode();
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
 * NB: Driven sensor data acquisition depends on I2S drumbeat
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
 * End data acquisition by disabling I2S data acquisition.
 * NB: Driven sensor data acquisition depends on I2S drumbeat
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
    endDataAcquisition();
    return beginDataAcquisition();
}

/**
 * Data acquisition function
 *
 * Method formerly benchmarked for (accel + sound) at 10microseconds.
 * @param inCallback  Set to true if the function is called from the I2S
 *  callback loop. In that case, only the I2S will be read (to allow the
 *  triggering of the next callback). If false, the function is called from main
 *  loop and all sensors can be read (including slow readings).
 */
void Conductor::acquireData(bool inCallback)
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
    bool force = false;
    // If EXPERIMENT mode, send last data batch before collecting the new data
    if (m_usageMode == UsageMode::EXPERIMENT)
    {
        if (loopDebugMode && (m_acquisitionMode != AcquisitionMode::RAWDATA ||
                              m_streamingMode != StreamingMode::WIRED))
        {
            debugPrint(F("EXPERIMENT should be RAW DATA + USB mode."));
        }
        if (inCallback)
        {
            iuI2S.sendData(iuUSB.port);
            iuAccelerometer.sendData(iuUSB.port);
        }
        force = true;
    }
    // Collect the new data
    for (uint8_t i = 0; i < Sensor::instanceCount; ++i)
    {
        Sensor::instances[i]->acquireData(inCallback, force);
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
        Feature::instances[i]->updateOperationState();
        featState = Feature::instances[i]->getOperationState();
        if ((uint8_t) newState < (uint8_t) featState)
        {
            newState = featState;
        }
    }
    m_operationState = newState;
    showOperationStateOnLed();
}

/**
 * Send feature data through a Serial port, depending on StreamingMode
 *
 * NB: If the AcquisitionMode is not FEATURE, does nothing.
 */
void Conductor::streamFeatures()
{
    if (m_acquisitionMode != AcquisitionMode::FEATURE)
    {
        return;
    }
    IUSerial *ser1 = NULL;
    bool sendFeatureGroupName1 = false;
    IUSerial *ser2 = NULL;
    bool sendFeatureGroupName2 = false;
    switch (m_streamingMode)
    {
        case StreamingMode::NONE:
            break;
        case StreamingMode::WIRED:
            ser1 = &iuUSB;
            break;
        case StreamingMode::BLE:
            ser1 = &iuBluetooth;
            break;
        case StreamingMode::WIFI:
            ser1 = &iuWiFi;
            sendFeatureGroupName1 = true;
            break;
        case StreamingMode::WIFI_AND_BLE:
            ser1 = &iuWiFi;
            sendFeatureGroupName1 = true;
            ser2 = &iuBluetooth;
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
        if (ser1)
        {
            if (m_streamingMode == StreamingMode::WIFI ||
                m_streamingMode == StreamingMode::WIFI_AND_BLE)
            {
                FeatureGroup::instances[i]->bufferAndStream(
                    ser1, IUSerial::MS_PROTOCOL, m_macAddress,
                    m_operationState, batteryLoad, timestamp,
                    sendFeatureGroupName1);
            }
            else
            {
                FeatureGroup::instances[i]->legacyStream(ser1, m_macAddress,
                    m_operationState, batteryLoad, timestamp,
                    sendFeatureGroupName1);
            }
        }
        if (ser2)
        {
            FeatureGroup::instances[i]->legacyStream(ser2, m_macAddress,
                m_operationState, batteryLoad, timestamp,
                sendFeatureGroupName2, 1);
        }
    }
}

/**
 * Send the acceleration raw data.
 *
 * @param axis 0, 1 or 3, corresponding to axis X, Y or Z
 */
void Conductor::sendAccelRawData(uint8_t axisIdx)
{
    if (axisIdx > 2)
    {
        return;
    }
    char accelEnergyName[4] = "A00";
    char axis[4] = "XYZ";
    accelEnergyName[2] = axis[axisIdx];
    Feature *accelEnergy = Feature::getInstanceByName(accelEnergyName);
    if (accelEnergy == NULL)
    {
        return;
    }
    if (m_streamingMode == StreamingMode::BLE)
    {
        iuBluetooth.write("REC,");
        iuBluetooth.write(m_macAddress.toString().c_str());
        iuBluetooth.write(',');
        iuBluetooth.write(axis[axisIdx]);
        accelEnergy->stream(&iuBluetooth, 4);
        delay(10);
    }
    else if (m_streamingMode == StreamingMode::WIFI ||
             m_streamingMode == StreamingMode::WIFI_AND_BLE)
    {
        uint16_t maxLen = 3500;
        char txBuffer[maxLen];
        for (uint16_t i =0; i < maxLen; i++)
        {
            txBuffer[i] = 0;
        }
        txBuffer[0] = axis[axisIdx];
        uint16_t idx = 1;
        idx += accelEnergy->sendToBuffer(txBuffer, idx, 4);
        txBuffer[idx] = 0; // Terminate string (idx incremented in sendToBuffer)
        iuWiFi.sendMSPCommand(MSPCommand::PUBLISH_RAW_DATA, txBuffer);
        delay(10);
    }
}

/**
 * Handle periodical publication of accel raw data.
 */
void Conductor::periodicSendAccelRawData()
{
    uint32_t now = millis();
    if (now - m_rawDataPublicationStart > m_rawDataPublicationTimer)
    {
        sendAccelRawData(0);
        sendAccelRawData(1);
        sendAccelRawData(2);
        m_rawDataPublicationStart = now;
    }
}



/* =============================================================================
    Debugging
============================================================================= */

/**
 * Write info from on the MCU state in destination.
 */
void Conductor::getMCUInfo(char *destination)
{
    float vdda = STM32.getVREF();
    float temperature = STM32.getTemperature();
    if (debugMode)
    {
        debugPrint("VDDA = ", false);
        debugPrint(vdda);
        debugPrint("MCU Temp = ", false);
        debugPrint(temperature);
    }
    size_t len = strlen(destination);
    for (size_t i = 0; i < len; i++)
    {
        destination[i] = '\0';
    }
    strcpy(destination, "{\"VDDA\":");
    strcat(destination, String(vdda).c_str());
    strcat(destination, ", \"temp\":");
    strcat(destination, String(temperature).c_str());
    strcat(destination, "}");
}

void  Conductor::streamMCUUInfo(HardwareSerial *port)
{
    char destination[50];
    getMCUInfo(destination);
    // TODO Change to "ST" ?
    port->print("HB,");
    port->print(destination);
    port->print(';');
}

/**
 * Expose current configurations
 */
void Conductor::exposeAllConfigurations()
{
    #ifdef IUDEBUG_ANY
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
