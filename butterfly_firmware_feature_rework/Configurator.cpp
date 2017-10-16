#include "Configurator.h"



char Configurator::START_CONFIRM[12] = "IUCMD_START";
char Configurator::END_CONFIRM[10] = "IUCMD_END";

/* =============================================================================
    Serial Reading
============================================================================= */

/**
 * Read from USB and process the command, if one was received.
 */
void Configurator::readFromSerial(IUSerial *iuSerial)
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
            }
        }
        iuSerial->resetBuffer();    // Clear wire buffer
    }
}


/* =============================================================================
    Command Processing
============================================================================= */

/**
 * Parse the given config json and apply the configuration
 */
void Configurator::processConfiguration(char *json)
{
    JsonObject& root = jsonBuffer.parseObject(json);
    if (!root.success())
    {
        if (debugMode)
        {
            debugPrint("parseObject() failed");
        }
        return;
    }
    // Device level configuration
    JsonVariant sub_config = root["device"];
    if (sub_config.success())
    {
        conductor.configure(sub_config);
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
}

/**
 * Apply the given config to the designated features.
 */
void Configurator::configureAllFeatures(JsonVariant &config)
{
    // Deactivate all features and computers
    deactivateEverything();
    // Set up new config
    for (uint8_t i = 0; i < FEATURE_COUNT; ++i)
    {
        configureFeature(FEATURES[i], config);
    }
}

/**
 * Apply the given config to the designated sensors.
 */
void Configurator::configureAllSensors(JsonVariant &config)
{
    for (uint8_t i = 0; i < SENSOR_COUNT; ++i)
    {
        SENSORS[i]->configure(config);
    }
}

/**
 * Process the USB commands (legacy)
 */
void Configurator::processLegacyUSBCommands(char *buff)
{
    // Usage mode Mode switching
    char *result = NULL;
    switch (conductor.getUsageMode())
    {
        case UsageMode::CALIBRATION:
            if (strcmp(buff, "IUCAL_END") == 0)
            {
                iuUSB.port->println(END_CONFIRM);
                conductor.changeUsageMode(UsageMode::OPERATION);
            }
            break;
        case UsageMode::EXPERIMENT:
            if (strcmp(buff, "IUCMD_END") == 0)
            {
                iuUSB.port->println(END_CONFIRM);
                conductor.changeUsageMode(UsageMode::OPERATION);
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
                iuRGBLed.changeColor((bool) (result[7] - '0'),
                                     (bool) (result[8] - '0'),
                                     (bool) (result[9] - '0'));
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
                conductor.changeUsageMode(UsageMode::CALIBRATION);
            }
            if (strcmp(buff, "IUCMD_START") == 0)
            {
                iuUSB.port->println(START_CONFIRM);
                conductor.changeUsageMode(UsageMode::EXPERIMENT);
            }
            break;
    }
}

/**
 * Process the instructions sent over Bluetooth
 */
void Configurator::processLegacyBLECommands(char *buff)
{
    if (conductor.getStreamingMode() == StreamingMode::WIRED)
    {
        return; // Do not listen to BLE when wired
    }
    switch(buff[0])
    {
        case '0': // DEPRECATED - Set feature thresholds
            if (debugMode)
            {
                debugPrint(F("BLE command is DEPRECATED"));
            }
            break;
        case '1': // Receive the timestamp data from the bluetooth hub
            if (buff[1] == ':' && buff[12] == '.')
            {
                int flag(0), ts(0), ms(0);
                sscanf(buff, "%d:%d.%d", &flag, &ts, &ms);
                conductor.setRefDatetime(
                    (double) ts + (double) ms / (double) 1000000);
            }
            break;
        case '2': // DEPRECATED - Bluetooth parameter setting
            if (debugMode)
            {
                debugPrint(F("BLE command is DEPRECATED"));
            }
            break;
        case '3': // Record button pressed - go into EXTENSIVE mode to record FFTs
            if (buff[7] == '0' && buff[9] == '0' && buff[11] == '0' &&
                buff[13] == '0' && buff[15] == '0' && buff[17] == '0')
            {
                if (loopDebugMode)
                {
                    debugPrint("Record mode");
                }
                /* TODO - Reimplement the following */
//                IUFeature *feat = getFeatureByName("A0X");
//                if (feat)
//                {
//                    feat.streamSourceData(iuBluetooth.port,
//                                          conductor.getMacAddress(), "X");
//                }
//                feat = getFeatureByName("A0Y");
//                if (feat)
//                {
//                    feat.streamSourceData(iuBluetooth.port,
//                                          conductor.getMacAddress(), "Y");
//                }
//                feat = getFeatureByName("A0Z");
//                if (feat)
//                {
//                    feat.streamSourceData(iuBluetooth.port,
//                                          conductor.getMacAddress(), "Z");
//                }
            }
            break;
        case '4': // DEPRECATED - Exit record mode
            if (debugMode)
            {
                debugPrint(F("BLE command is DEPRECATED"));
            }
            break;
        case '5':
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
        case '6': // DEPRECATED - Set which feature are used for OperationState
            if (debugMode)
            {
                debugPrint(F("BLE command is DEPRECATED"));
            }
            break;
    }
}


/* =============================================================================
    Instanciation
============================================================================= */

Configurator configurator(12);
