#include<string.h>
#include "Conductor.h"
#include "rBase64.h"
#include "FFTConfiguration.h"
#include "RawDataState.h"
#include "IUOTA.h"
#include <MemoryFree.h>
#include "stm32l4_iap.h"
#include "IUBMD350.h"
#include "AdvanceFeatureComputer.h"


extern IUOTA iuOta;
const char* fingerprintData;
const char* fingerprints_X;
const char* fingerprints_Y;
const char* fingerprints_Z;

// Modbus Streaming Features buffer
float modbusFeaturesDestinations[8];

int m_temperatureOffset;
int m_audioOffset;
        
char Conductor::START_CONFIRM[11] = "IUOK_START";
char Conductor::END_CONFIRM[9] = "IUOK_END";

IUFlash::storedConfig Conductor::CONFIG_TYPES[Conductor::CONFIG_TYPE_COUNT] = {
    IUFlash::CFG_FEATURE,
    IUFlash::CFG_DEVICE};


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
    for (uint8_t i = 0; i < Sensor::instanceCount; ++i) {
        Sensor::instances[i]->setPowerMode(PowerMode::SLEEP);
    }
    ledManager.overrideColor(RGB_BLACK);
    STM32.stop(duration);
    ledManager.stopColorOverride();
    for (uint8_t i = 0; i < Sensor::instanceCount; ++i) {
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
    for (uint8_t i = 0; i < Sensor::instanceCount; ++i) {
        Sensor::instances[i]->setPowerMode(PowerMode::SUSPEND);
    }
    ledManager.overrideColor(RGB_BLACK);
    STM32.stop(duration * 1000);
    ledManager.stopColorOverride();
    iuBluetooth.setPowerMode(PowerMode::REGULAR);
    iuWiFi.setPowerMode(PowerMode::REGULAR);
    for (uint8_t i = 0; i < Sensor::instanceCount; ++i) {
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
    if (m_sleepMode == sleepMode::AUTO && now - m_startTime > m_autoSleepDelay) {
        sleep(m_sleepDuration);
    }
    else if (m_sleepMode == sleepMode::PERIODIC &&
             now - m_startTime > m_cycleTime - m_sleepDuration) {
        suspend(m_sleepDuration);
        m_startTime = now;
    }
}

/* =============================================================================
    Local storage (flash) management
============================================================================= */

/**
 * Update a config from flash storage.
 *
 * @return bool success
 */
bool Conductor::configureFromFlash(IUFlash::storedConfig configType)
{
    const size_t bufferSize = 600;
    StaticJsonBuffer<bufferSize> jsonBuffer;
    JsonVariant config = JsonVariant(
            iuFlash.loadConfigJson(configType, jsonBuffer));
   bool success = config.success();
    if (success) {
        switch (configType) {
            case IUFlash::CFG_DEVICE:
                configureMainOptions(config);
                break;
            case IUFlash::CFG_COMPONENT:
                configureAllSensors(config);
                break;
            case IUFlash::CFG_FEATURE:
                configureAllFeatures(config);
                break;
            case IUFlash::CFG_OP_STATE:
                opStateComputer.configure(config);
                activateFeature(&opStateFeature);
                break;
            case IUFlash::CFG_WIFI0:
            case IUFlash::CFG_WIFI1:
            case IUFlash::CFG_WIFI2:
            case IUFlash::CFG_WIFI3:
            case IUFlash::CFG_WIFI4:
                iuWiFi.configure(config);
                        break;
            case IUFlash::CFG_MODBUS_SLAVE:
                debugPrint("CONFIGURING THE MODBUS SLAVE");
                iuModbusSlave.setupModbusDevice(config);
                break;
            // case IUFlash::CFG_DIG:
            //     availableDiagnosticConfig = config;
            //     availableDiagnosticConfig.printTo(Serial);
            //     debugPrint("Loaded DIG config successfully");
            //     break;
            case IUFlash::CFG_RPM:
                configureRPM(config);
                break;
            default:
                if (debugMode) {
                    debugPrint("Unhandled config type: ", false);
                    debugPrint((uint8_t) configType);
                }
                success = false;
                break;
        }
    }
    if (debugMode && success) {
        debugPrint("Successfully loaded config type #", false);
        debugPrint((uint8_t) configType);
    }else if(debugMode && ! success)
    {
        debugPrint("Configs not Found #",false);
        debugPrint((uint8_t) configType);
    }
    
    return success;
}

/**
 *
 */
String Conductor::sendConfigChecksum(IUFlash::storedConfig configType, JsonObject &inputConfig)
{
    if (!(m_streamingMode == StreamingMode::WIFI ||
          m_streamingMode == StreamingMode::WIFI_AND_BLE)) {
        if (debugMode) {
            debugPrint("Config checksum can only be sent via WiFi.");
        }
        return "error";
    }
    size_t configMaxLen = 500;
    char config[configMaxLen];
    strcpy(config, "");
    size_t charCount = 0;
    if (iuFlash.available()) {
        charCount = iuFlash.readConfig(configType, config, configMaxLen);
    }
    // Charcount = 0 if flash is unavailable or file not found => Checksum of
    // empty string will be sent, and that will trigger a config refresh
    unsigned char* md5hash = MD5::make_hash(config, charCount);
    char *md5str = MD5::make_digest(md5hash, 16);
    size_t md5Len = 33;
    String fullStr;
    if(strcmp(md5str,GetStoredMD5(configType,inputConfig)) != 0)
    {
        switch (configType) {
            case IUFlash::CFG_DEVICE:
                fullStr = "device";
                debugPrint("Config Error");
                break;
            // case IUFlash::CFG_COMPONENT:
            //     break;
            case IUFlash::CFG_FEATURE:
                fullStr = "features";
                debugPrint("Config Error");
                break;
            // case IUFlash::CFG_OP_STATE:
            //     break;
            default:
                if (debugMode) {
                    debugPrint("Unhandled config type: ", false);
                    debugPrint((uint8_t) configType);
                }
                break;
        }
        // switch (configType) {
        //     case IUFlash::CFG_DEVICE:
        //         written = snprintf(
        //             fullStr, fullStrLen, "{\"mac\":\"%s\",\"main\":\"%s\"}",
        //             m_macAddress.toString().c_str(), md5str);
        //         break;
        //     case IUFlash::CFG_COMPONENT:
        //         written = snprintf(
        //             fullStr, fullStrLen, "{\"mac\":\"%s\",\"components\":\"%s\"}",
        //             m_macAddress.toString().c_str(), md5str);
        //         break;
        //     case IUFlash::CFG_FEATURE:
        //         written = snprintf(
        //             fullStr, fullStrLen, "{\"mac\":\"%s\",\"features\":\"%s\"}",
        //             m_macAddress.toString().c_str(), md5str);
        //         break;
        //     case IUFlash::CFG_OP_STATE:
        //         written = snprintf(
        //             fullStr, fullStrLen, "{\"mac\":\"%s\",\"opState\":\"%s\"}",
        //             m_macAddress.toString().c_str(), md5str);
        //         break;
        //     default:
        //         if (debugMode) {
        //             debugPrint("Unhandled config type: ", false);
        //             debugPrint((uint8_t) configType);
        //         }
        //         break;
        // }
        free(md5hash);
        free(md5str);
        return fullStr;
    }
    else{
        return "NULL";
    }
    // if (written > 0 && written < fullStrLen) {
    //     // Send checksum
    //     iuWiFi.sendMSPCommand(MSPCommand::PUBLISH_CONFIG_CHECKSUM, fullStr,
    //                           written);
    // } else if (loopDebugMode) {
    //     debugPrint(F("Failed to format config checksum: "), false);
    //     debugPrint(fullStr);
    // }
    //free memory
    
}

/**
 *
 */
void Conductor::periodicSendConfigChecksum()
{
    if (m_streamingMode == StreamingMode::WIFI ||
         m_streamingMode == StreamingMode::WIFI_AND_BLE) {
        uint32_t now = millis();
        int index = 0;
        String result;
        bool requestConfig = false;
        char resultArray[200];        

        StaticJsonBuffer<600> jsonBuffer;
        JsonObject& inputConfig = iuFlash.loadConfigJson(iuFlash.CFG_HASH, jsonBuffer);
        JsonObject& resultConfig = jsonBuffer.createObject();
        // resultConfig["DEVICEID"] = m_macAddress.toString().c_str();
        JsonArray& configType = resultConfig.createNestedArray("CONFIGTYPE");
        if (now - m_configTimerStart > SEND_CONFIG_CHECKSUM_TIMER) {
            for(int i = 0 ; i < CONFIG_TYPE_COUNT; i++){
                result = sendConfigChecksum(CONFIG_TYPES[i],inputConfig);
                if(strcmp(result.c_str(),"NULL") !=0)
                {
                    configType.add(result);
                    debugPrint("Invalid Config  :",false);debugPrint(result);
                    index++;
                    requestConfig = true;
                }else{
                    debugPrint("Valid Config  :",false);debugPrint(result);
                }
            }
            
            // m_nextConfigToSend++;
            // m_nextConfigToSend %= CONFIG_TYPE_COUNT;
            m_configTimerStart = now;
        }
        if(requestConfig){
            
            String rString;
            resultConfig.printTo(rString);
            sprintf(resultArray,"%s%s%s%s%s","{\"DEVICEID\":\"", m_macAddress.toString().c_str(),"\",",rString.c_str(),"}");
            iuWiFi.sendMSPCommand(MSPCommand::PUBLISH_CONFIG_CHECKSUM, resultArray);
        }
    }
}


/* =============================================================================
    Serial Reading & command processing
============================================================================= */

/**
 * Parse the given config json and apply the configuration
 */
bool Conductor::processConfiguration(char *json, bool saveToFlash)
{
    // String to hold the result of validation of the config json
    char validationResultString[400];

    //StaticJsonBuffer<JSON_BUFFER_SIZE> jsonBuffer;            // make it dynamic
    //const size_t bufferSize = JSON_ARRAY_SIZE(2) + JSON_OBJECT_SIZE(3) + 60;        // dynamically allociated memory
    // const size_t bufferSize = JSON_OBJECT_SIZE(2) + 2*JSON_OBJECT_SIZE(4) + 11*JSON_OBJECT_SIZE(5) + JSON_OBJECT_SIZE(13) + 1396;
    // DynamicJsonBuffer jsonBuffer(bufferSize);
    DynamicJsonBuffer jsonBuffer;
    //Serial.print("JSON 1 SIZE :");Serial.println(bufferSize);
    
    JsonObject& root = jsonBuffer.parseObject(json);
    String jsonChar;
    root.printTo(jsonChar);
    JsonVariant variant = root;
    
    // variant.prettyPrintTo(Serial);
            
    char messageId[60];
    if(root.success()) {
        // Get the message id, to be used in acknowledgements
        strcpy(messageId, ((JsonObject&)root)["messageId"].as<char*>());
        if(debugMode) {
            debugPrint("Config message received with messageID: ", false); debugPrint(messageId);
        }
    } else {
        if (debugMode) {
            debugPrint("parseObject() failed");
        }
        return false;
    }
    // Device level configuration
    JsonVariant subConfig = root["main"];
    subConfig.printTo(jsonChar);
    
    if (subConfig.success()) {
        // This subconfig has to be validated for the provided fields, note the that subconfig may contain only a subset of all the fields present in device config
        // i.e. Complete device conf: {"main":{"GRP":["MOTSTD"],"RAW":1800,"POW":0,"TSL":60,"TOFF":10,"TCY":20,"DSP":512}}
        // the received config may contain only DSP or RAW or any subset of the fields.
        // The incorrect fields should be weeded out, corresponding error messages should be attached in the validationResultString.
        bool validConfig = iuFlash.validateConfig(IUFlash::CFG_DEVICE, subConfig, validationResultString, (char*) m_macAddress.toString().c_str(), getDatetime(), messageId);
        if(validConfig) {
            configureMainOptions(subConfig);
            if (saveToFlash) {
                iuFlash.updateConfigJson(IUFlash::CFG_DEVICE, subConfig);
                // Devices with all versions of firmware will save this config, 
                // for v1.1.3 onwards, json contains "DSP" field for configuring dataSendPeriod
                // for v1.1.2 and below, this "DSP" field will be ignored by device
            }
        }
        iuWiFi.sendMSPCommand(MSPCommand::CONFIG_ACK, validationResultString);
    }
    // Component configuration
    subConfig = root["components"];
    if (subConfig.success()) {
        configureAllSensors(subConfig);
        if (saveToFlash) {
            iuFlash.saveConfigJson(IUFlash::CFG_COMPONENT, subConfig);
        }
    }
    // Feature configuration
    subConfig = root["features"];
    if (subConfig.success()) {
        configureAllFeatures(subConfig);
        if (saveToFlash) {
            iuFlash.saveConfigJson(IUFlash::CFG_FEATURE, subConfig);
            setThresholdsFromFile();
            //send ACK on ide_pluse/command_response/
            const char* messageId;
            messageId = root["messageId"]  ;
            
            snprintf(ack_config, 200, "{\"messageId\":\"%s\",\"messageTye\":\"features-config-ack\",\"macId\":\"%s\"}", messageId,m_macAddress.toString().c_str());
            
            //Serial.println(ack_config);
            if(iuWiFi.isConnected()){
                iuWiFi.sendMSPCommand(MSPCommand::RECEIVE_DIAGNOSTIC_ACK, ack_config);
            }else if (!iuEthernet.isEthernetConnected && StreamingMode::ETHERNET)    // Ethernet is connected
            {       debugPrint("Sending Fetures ACK over Ethernet");
                    snprintf(ack_config, 200, "{\"deviceId\":\"%s\",\"transport\":%d,\"messageType\":%d,\"payload\": \"{\\\"macId\\\":\\\"%s\\\",\\\"messageId\\\":\\\"%s\\\"}\"}",
                      m_macAddress.toString().c_str(),0, 2, m_macAddress.toString().c_str(),messageId);
                   
                    debugPrint(ack_config,true);
                    iuEthernet.write(ack_config); 
                    iuEthernet.write("\n");
            } 
        }
    }
    // Feature configuration
    subConfig = root["opState"];
    if (subConfig.success()) {
        opStateComputer.configure(subConfig);
        activateFeature(&opStateFeature);
        if (saveToFlash) {
            iuFlash.saveConfigJson(IUFlash::CFG_OP_STATE, subConfig);
        }
    }
    // Diagnostic Thresholds configuration
    subConfig = root["thresholds"];
    if (subConfig.success()) {
        configureAllFeatures(subConfig);
        if (saveToFlash) {
            //iuFlash.saveConfigJson(IUFlash::CFG_FEATURE, subConfig);
            //send ACK on ide_pluse/command_response/
            const char* messageId;
            messageId = root["messageId"]  ;
            snprintf(ack_config, 200, "{\"messageId\":\"%s\",\"messageTye\":\"thresholds-config-ack\",\"macId\":\"%s\"}", messageId,m_macAddress.toString().c_str());
            
            //Serial.println(ack_config);
            if(iuWiFi.isConnected()){
                 iuWiFi.sendMSPCommand(MSPCommand::RECEIVE_DIAGNOSTIC_ACK, ack_config);
            }else if (!iuEthernet.isEthernetConnected && StreamingMode::ETHERNET)    // Ethernet is connected
            {       debugPrint("Sending Fingerpritns Threshold ACK over Ethernet");
                    snprintf(ack_config, 200, "{\"deviceId\":\"%s\",\"transport\":%d,\"messageType\":%d,\"payload\": \"{\\\"macId\\\":\\\"%s\\\",\\\"messageId\\\":\\\"%s\\\"}\"}",
                      m_macAddress.toString().c_str(),0, 2, m_macAddress.toString().c_str(),messageId);
                   
                    debugPrint(ack_config,true);
                    iuEthernet.write(ack_config); 
                    iuEthernet.write("\n");
                   
            } 
        }
    }
     // MQTT Server configuration
    subConfig = root["mqtt"];
    if (subConfig.success()) {
        //configureAllFeatures(subConfig);
        bool dataWritten = false;
        // iuFlash.writeInternalFlash(1,CONFIG_MQTT_FLASH_ADDRESS,jsonChar.length(),(const uint8_t*)jsonChar.c_str());
        if (saveToFlash) {
            //DOSFS.begin();
            File mqttFile = DOSFS.open("MQTT.conf", "w");
            if (mqttFile)
            {
                
                if (loopDebugMode) {
                 debugPrint(F("Writting into file: "), true);
                }
                mqttFile.print(jsonChar);
                mqttFile.close();
                dataWritten = true;
            }
            else if (loopDebugMode) {
                 debugPrint("Failed to write into file: MQTT.conf ");
                
            }  
        
        }
        if(dataWritten == true){
          iuFlash.writeInternalFlash(1,CONFIG_MQTT_FLASH_ADDRESS,jsonChar.length(),(const uint8_t*)jsonChar.c_str());
          configureMQTTServer("MQTT.conf");
          //send Ack to BLE
          iuBluetooth.write("MQTT-RECEVIED");
          // get the latest account id and send to wifi
         /* JsonObject& config = configureJsonFromFlash("MQTT.conf",1);      // get the accountID
          m_accountId = config["accountid"];
          iuWiFi.sendMSPCommand(MSPCommand::SEND_ACCOUNTID, m_accountId); 

          Serial.print("READING ACCOUNTID :");Serial.println(  m_accountId );
         */ 
        }
        
    }
    //Diagostic Fingerprint configurations
    subConfig = root["fingerprints"];
    if (subConfig.success()) {
        //opStateComputer.configure(subConfig);
        //activateFeature(&opStateFeature);
        bool dataWritten = false;
        if (saveToFlash) {
          
        //   Serial.println("INSIDE SAVE TO FNGERPRINTS....");
          File fingerprints = DOSFS.open("finterprints.conf","w");
          if (fingerprints)
            {
                debugPrint("Writing to fingerptins.conf ...");
                fingerprints.print(jsonChar);
                availableFingerprints = jsonChar;
                fingerprints.close();
                dataWritten = true;
            }
            else if (loopDebugMode) {
                 debugPrint(F("Failed to write into file: "), false);
                 //Serial.println("Error Writting to fingerprints.conf");
                 debugPrint("Error Writting to fingerprints.conf");
            }  
        
        }
        if(dataWritten == true){
          debugPrint("Reading from fingerprints.conf file ...........");
          JsonObject& config = iuDiagnosticEngine.configureFingerPrintsFromFlash("finterprints.conf",dataWritten);   //iuDiagnosticEngine

           
            if (loopDebugMode) { debugPrint(F("Send Diagnostic configuration Acknowledge to wifi")); }
            const char* messageId;
            messageId = config["messageId"]  ;
            
          
            snprintf(ack_config, 200, "{\"messageId\":\"%s\",\"messageType\":\"fingerprint-config-ack\",\"macId\":\"%s\"}", messageId,m_macAddress.toString().c_str());

            if(iuWiFi.isConnected()){
                 iuWiFi.sendMSPCommand(MSPCommand::RECEIVE_DIAGNOSTIC_ACK, ack_config); 
            }
            else if (!iuEthernet.isEthernetConnected && StreamingMode::ETHERNET)    // Ethernet is connected
            {       debugPrint("Sending Fingerprints ACK over Ethernet");
                    snprintf(ack_config, 200, "{\"deviceId\":\"%s\",\"transport\":%d,\"messageType\":%d,\"payload\": \"{\\\"macId\\\":\\\"%s\\\",\\\"messageId\\\":\\\"%s\\\"}\"}",
                      m_macAddress.toString().c_str(),0, 2, m_macAddress.toString().c_str(),messageId);
                   
                    debugPrint(ack_config,true);
                    iuEthernet.write(ack_config); 
                    iuEthernet.write("\n");
                    //iuEthernet.write(ack_config);
            } 
            
        }
        
      }
    // http endpoint configuration
    subConfig = root["httpConfig"];
    if (subConfig.success()) {
        //configureAllFeatures(subConfig);
        bool dataWritten = false;
        bool validConfiguration = iuFlash.validateConfig(IUFlash::CFG_HTTP, root, validationResultString, (char*) m_macAddress.toString().c_str(), getDatetime(), messageId);
        if(loopDebugMode) { 
            debugPrint("Validation: ", false);
            debugPrint(validationResultString); 
            debugPrint("HTTP configuration validation result: ", false); 
            debugPrint(validConfiguration);
        }
        if(validConfiguration){
            // iuFlash.writeInternalFlash(1,CONFIG_HTTP_FLASH_ADDRESS,jsonChar.length(),(const uint8_t*)jsonChar.c_str());
            if (saveToFlash) {
                //DOSFS.begin();
                File httpFile = DOSFS.open("httpConfig.conf", "w");
                if (httpFile)
                {
                    
                    if (loopDebugMode) {
                    debugPrint(F("Writting into file: "), true);
                    }
                    httpFile.print(jsonChar);
                    httpFile.close();
                    dataWritten = true;
                }
                else if (loopDebugMode) {
                    debugPrint("Failed to write into file: httpConfig.conf ");
                    
                }  
            
            }
            if(dataWritten == true){
                iuFlash.writeInternalFlash(1,CONFIG_HTTP_FLASH_ADDRESS,jsonChar.length(),(const uint8_t*)jsonChar.c_str());
            //configureBoardFromFlash("httpConfig.conf",dataWritten);
            JsonObject& config = configureJsonFromFlash("httpConfig.conf",1);

            const char* messageId = config["messageId"];
            const char*  host = config["httpConfig"]["host"].as<char*>();
            int port = config["httpConfig"]["port"].as<int>();
            const char* httpPath = config["httpConfig"]["path"].as<char*>();
            
            bool oemConfig = false;
            bool oemSameConfig = true;
            if(config.containsKey("httpOem")){
                const char*  oem_host = config["httpOem"]["host"].as<char*>();
                int oem_port = config["httpOem"]["port"].as<int>();
                const char* oem_httpPath = config["httpOem"]["path"].as<char*>();
                if(strcmp( oem_host, m_httpHost_oem) != 0  || oem_port != m_httpPort_oem || strcmp(oem_httpPath, m_httpPath_oem) != 0){
                    oemSameConfig = false;
                }
                oemConfig = true;
            }

            // debugPrint("Active httpConfigs : ");
            // debugPrint("Host :",false);debugPrint(m_httpHost);
            // debugPrint("Port :",false);debugPrint(m_httpPort);
            // debugPrint("Path :",false);debugPrint(m_httpPath);

            //Serial.print("File Content :");Serial.println(jsonChar);
            //Serial.print("http details :");Serial.print(m_httpHost);Serial.print(",");Serial.print(m_httpPort);Serial.print(",");Serial.print(m_httpPath);Serial.println("/***********/");
            //iuWiFi.sendMSPCommand(MSPCommand::SET_RAW_DATA_ENDPOINT_HOST,m_httpHost); 
            //iuWiFi.sendMSPCommand(MSPCommand::SET_RAW_DATA_ENDPOINT_PORT,String(m_httpPort).c_str()); 
            //iuWiFi.sendMSPCommand(MSPCommand::SET_RAW_DATA_ENDPOINT_ROUTE,m_httpPath); 

            // char httpConfig_ack[150];
            // snprintf(httpConfig_ack, 150, "{\"messageId\":\"%s\",\"macId\":\"%s\"}", messageId,m_macAddress.toString().c_str());
                
            // debugPrint(F("httpConfig ACK :"));debugPrint(httpConfig_ack);
            if (iuWiFi.isConnected() )
            {   
                iuWiFi.sendMSPCommand(MSPCommand::RECEIVE_HTTP_CONFIG_ACK, validationResultString);
            
            }else if (!iuEthernet.isEthernetConnected && StreamingMode::ETHERNET)    // Ethernet is connected
            {     
                    snprintf(ack_config, 200, "{\"deviceId\":\"%s\",\"transport\":%d,\"messageType\":%d,\"payload\": \"{\\\"macId\\\":\\\"%s\\\",\\\"messageId\\\":\\\"%s\\\"}\"}",
                        m_macAddress.toString().c_str(),0, 2, m_macAddress.toString().c_str(),messageId);
                    
                        debugPrint(ack_config,true);
                        iuEthernet.write(ack_config); 
                        iuEthernet.write("\n");
                    //iuEthernet.write(httpConfig_ack);
            }
            
            
            
            //stm reset
            delay(10);
            if(!httpOtaValidation){
                if((strcmp( host, m_httpHost) != 0  || port != m_httpPort || strcmp(httpPath, m_httpPath) != 0) || oemSameConfig == false ){
                        DOSFS.end();
                        delay(10);
                        //STM32.reset();
                        iuWiFi.hardReset();
                }   
            }
            
            }
        }else{
            if (iuWiFi.isConnected() )
            {   
                iuWiFi.sendMSPCommand(MSPCommand::RECEIVE_HTTP_CONFIG_ACK, validationResultString);
            
            }
        }
        
    } 
    // SET Sensor Configuration and its OFFSET value
    subConfig = root["sensorConfig"];
    if (subConfig.success()) {
        //configureAllFeatures(subConfig);
        bool dataWritten = false;
        
        if (saveToFlash) {
            //DOSFS.begin();
            File sensorConfigFile = DOSFS.open("sensorConfig.conf", "w");
            if (sensorConfigFile)
            {
                
                if (loopDebugMode) {
                 debugPrint(F("Writting into file: "), true);
                }
                sensorConfigFile.print(jsonChar);
                sensorConfigFile.close();
                dataWritten = true;
            }
            else if (loopDebugMode) {
                 debugPrint("Failed to write into file: sensorConfig.conf ");
                
            }  
        
        }
        if(dataWritten == true){
          
          JsonObject& config = configureJsonFromFlash("sensorConfig.conf",1);      // get the accountID
          JsonVariant variant = config;
          
          m_temperatureOffset = config["sensorConfig"]["TMP_OFFSET"];
          m_audioOffset = config["sensorConfig"]["SND_OFFSET"];
        
          if(loopDebugMode){

            debugPrint("Temperature Offset is: ",false);debugPrint(m_temperatureOffset);
            debugPrint("Audio Offset is:",false); debugPrint(m_audioOffset);
            //debugPrint("File content");  
           // variant.prettyPrintTo(Serial);
          }
          
        }
        
    }
    // SET the relayAgentConfig on Ethernet
    subConfig = root["relayAgentConfig"];
    if (subConfig.success()) {
        bool dataWritten = false;
        
        if (saveToFlash) {
            //DOSFS.begin();
            File relayAgentConfigFile = DOSFS.open("relayAgentConfig.conf", "w");
            if (relayAgentConfigFile)
            {
                
                if (loopDebugMode) {
                 debugPrint(F("Writting into file: "), true);
                }
                relayAgentConfigFile.print(jsonChar);
                relayAgentConfigFile.close();
                dataWritten = true;
            }
            else if (loopDebugMode) {
                 debugPrint("Failed to write into file: relayAgentConfig.conf ");
                
            }  
        
        }else
        {       ledManager.overrideColor(RGB_CYAN);
                delay(2000);
                ledManager.stopColorOverride();
                //iuUSB.port->println("Failed to write into file, try again");
        }
        if(dataWritten == true){
          
          JsonObject& config = configureJsonFromFlash("relayAgentConfig.conf",1);      // get the accountID
          JsonVariant variant = config;
          bool heartbeatFlag = false;

          iuEthernet.m_workMode = config["relayAgentConfig"]["workMode"];
          iuEthernet.m_remoteIP = config["relayAgentConfig"]["remoteAddr"];
          iuEthernet.m_remotePort = config["relayAgentConfig"]["remotePort"];
          iuEthernet.m_enableHeartbeat = config["relayAgentConfig"]["enableHeartbeat"];
          iuEthernet.m_heartbeatInterval = config["relayAgentConfig"]["heartbeatInterval"];
          iuEthernet.m_heartbeatDir = config["relayAgentConfig"]["heartbeatDir"];
          iuEthernet.m_heartbeatMsg = config["relayAgentConfig"]["heartbeatMsg"];
        
           while(iuEthernet.SetAT() != 0);
           iuEthernet.SocketConfig(iuEthernet.m_workMode,iuEthernet.m_remoteIP,iuEthernet.m_remotePort);//setupHardware();
           
          if (iuEthernet.m_enableHeartbeat != NULL)
          { 
            // Set Heartbeat configuration  
            heartbeatFlag = true;
            iuEthernet.EnableHeart(iuEthernet.m_enableHeartbeat);
            iuEthernet.HeartDirection(iuEthernet.m_heartbeatDir);              
            iuEthernet.HeartTime(iuEthernet.m_heartbeatInterval);    
            iuEthernet.HeartData(iuEthernet.m_heartbeatMsg);
            
          }  
          bool currentState = false;  
          debugPrint("Previous Ethernet state : ",false);
          debugPrint(iuEthernet.isEthernetConnected,true);

           for (size_t i = 0; i < 10; i++)
           {
               /* code */
              currentState =  iuEthernet.TCPStatus(); 
              debugPrint("Current State : ",false);
              debugPrint(currentState,true);

            if (currentState == iuEthernet.isEthernetConnected)
            {                                                   
                if(m_streamingMode == StreamingMode::ETHERNET){
                    debugPrint("StreamingMode is Ethernet",true);
                    iuEthernet.isEthernetConnected = currentState;
                    break;
                
                }   
                // else{
                //     debugPrint("StreamingMode is Non Ethernet",true);
                //     updateStreamingMode();
                //     if(debugMode){
                //         debugPrint("Update the StreamingMode with Ethernet",true);
                //     }          
                // }
            }
                debugPrint("loop count :",false);
                debugPrint(i,true);
                delay(1000);
           }
           
           debugPrint(" Final TCP Status:",false);
           debugPrint(iuEthernet.isEthernetConnected);
           //iuEthernet.isEthernetConnected = currentState;
          
          //iuEthernet.ExitAT();
          iuEthernet.Restart();
          delay(1000);
          ledManager.overrideColor(RGB_PURPLE);
          delay(2000);
          ledManager.stopColorOverride();
         if(loopDebugMode){
            debugPrint("RunTime workMode: ",false);
            debugPrint(iuEthernet.m_workMode);
            debugPrint("RunTime remoteAddr:",false);
            debugPrint(iuEthernet.m_remoteIP);
            debugPrint("RunTime remotePort:",false);
            debugPrint(iuEthernet.m_remotePort,true);
            if (heartbeatFlag == true)
            {
                debugPrint("Heartbeat:",false);
                debugPrint(iuEthernet.m_enableHeartbeat);
                debugPrint("Heartbeat Interval:",false);
                debugPrint(iuEthernet.m_heartbeatInterval);
                debugPrint("Hearbeat Direction:",false);
                debugPrint(iuEthernet.m_heartbeatDir);
                debugPrint("Heartbeat Message:",false);
                debugPrint(iuEthernet.m_heartbeatMsg);
            }
            
        }    
        heartbeatFlag = false;
        }
    }
    // FFT configuration
    // Message is always saved to file, after which STM resets
    subConfig = root["fft"];
    if (subConfig.success()) {
        // Validate if the received parameters are correct
        if(loopDebugMode) {
            debugPrint("FFT configuration received: ", false);
            subConfig.printTo(Serial); debugPrint("");
        }
        bool validConfiguration = iuFlash.validateConfig(IUFlash::CFG_FFT, subConfig, validationResultString, (char*) m_macAddress.toString().c_str(), getDatetime(), messageId);
        if(loopDebugMode) { 
            debugPrint("Validation: ", false);
            debugPrint(validationResultString); 
            debugPrint("FFT configuration validation result: ", false); 
            debugPrint(validConfiguration);
        }

        if(validConfiguration) {
            if(loopDebugMode) debugPrint("Received valid FFT configuration");
        
            // Save the valid configuration to file 
            if(saveToFlash) { 
                // Check if the config is new, then save to file and reset
                iuFlash.saveConfigJson(IUFlash::CFG_FFT, subConfig);
                if(loopDebugMode) debugPrint("Saved FFT configuration to file");
                
                // Acknowledge that configuration has been saved successfully on /ide_plus/command_response/ topic
                // If streaming mode is BLE, send an acknowledgement on BLE as well
                // NOTE: MSPCommand CONFIG_ACK added to Arduino/libraries/IUSerial/src/MSPCommands.h
                iuWiFi.sendMSPCommand(MSPCommand::CONFIG_ACK, validationResultString);
                if(StreamingMode::BLE && isBLEConnected()) { iuBluetooth.write("FFT_CFG_SUCCESS;"); delay(100); }

                // Restart STM, setFFTParams will configure FFT parameters in setup()
                delay(3000);  // wait for MQTT message to be published
                DOSFS.end();
                delay(10);
                STM32.reset();
            }
        } else {
            if(loopDebugMode) debugPrint("Received invalid FFT configuration");

            // Acknowledge incorrect configuration, send the errors on /ide_plus/command_response topic
            // If streaming mode is BLE, send an acknowledgement on BLE as well
            iuWiFi.sendMSPCommand(MSPCommand::CONFIG_ACK, validationResultString);
            //if(StreamingMode::BLE && isBLEConnected()) { iuBluetooth.write("FFT_CFG_FAILURE;"); delay(100); }
        }
    } // If json is incorrect, it will result in parsing error in jsonBuffer.parseObject(json) which will cause the processConfiguration call to return
    

    subConfig = root["messageType"];
    if (subConfig.success()) {
        String msgType = root["messageType"];
        strcpy(m_otaMsgType,msgType.c_str());
        if(!(strcmp((const char *)m_otaMsgType,(const char *)"initiateota")))
        {
            otaInitTimeoutFlag = false;
            if(loopDebugMode) {
                debugPrint(F("OTA InitReq Wait Timeout cleared.."));
                debugPrint(F("OTA configuration received: "), false);
                subConfig.printTo(Serial); debugPrint("");
            }
            if(doOnceFWValid == true)
            { // Don't accept new OTA request during Validation of Last OTA
                if(loopDebugMode) {
                    debugPrint(F("Last OTA in Progress.. Unable to process OTA Request"));
                }
                return true;
            }
            memset(m_type1,'\0',sizeof(m_type1));
            memset(m_type2,'\0',sizeof(m_type2));
            memset(m_otaStmUri,'\0',sizeof(m_otaStmUri));
            memset(m_otaEspUri,'\0',sizeof(m_otaEspUri));
            memset(stmHash,'\0',sizeof(stmHash));
            memset(espHash,'\0',sizeof(espHash));
            memset(m_otaMsgId,'\0',sizeof(m_otaMsgId));
            memset(m_otaFwVer,'\0',sizeof(m_otaFwVer));
            memset(m_deviceType,'\0',sizeof(m_deviceType));
            strcpy(m_otaMsgId,(const char*)root["messageId"]);
            strcpy(m_deviceType,(const char*)root["supportedDeviceTypes"][0]);
    //     String test1 = root["otaConfig"]["supportedDeviceTypes"];
            if(loopDebugMode) {
                debugPrint(F("OTA Message ID: "), false);
                debugPrint(m_otaMsgId);
                debugPrint(F("OTA FW Version: "), false);
                debugPrint(m_otaFwVer);
            }
            if(strncmp(m_deviceType,"vEdge 1.6",9)!=0) //Change the device name according to device type
            {
                if(loopDebugMode) {
                    debugPrint(F("Sending OTA_FDW_ABORT, Invalid Firmware"));
                }
                sendOtaStatusMsg(MSPCommand::OTA_FDW_ABORT,OTA_DOWNLOAD_ERR, String(iuOta.getOtaRca(OTA_INVALID_FIRMWARE)).c_str());
                if (loopDebugMode) { debugPrint(F("Switching Device mode:OTA -> OPERATION")); }
                iuWiFi.m_setLastConfirmedPublication();
                changeUsageMode(UsageMode::OPERATION);
                delay(10);
                return false;
            }
            subConfig = root["fwBinaries"][0];
            String fwType = subConfig["type"];
            if(!(strcmp((const char *)fwType.c_str(),(const char *)"STM32")))
            {
                strcpy(m_type1,(const char*)fwType.c_str());
                strcpy(m_otaStmUri,(const char*)subConfig["url"]);
                strcpy(stmHash,(const char*)subConfig["hash"]);
                //static const char* stmHash = subConfig["hash"];
                if(loopDebugMode) {
                    debugPrint(F("OTA Type: "), false);
                    debugPrint(m_type1);
                    debugPrint(F("OTA Main FW URI: "), false);
                    debugPrint(m_otaStmUri);
                    debugPrint(F("OTA Main FW Bin Hash: "), false);
                    debugPrint(stmHash);
                }
            }
            else if(!(strcmp((const char *)fwType.c_str(),(const char *)"ESP32")))
            {
                strcpy(m_type2,(const char*)fwType.c_str());
                strcpy(m_otaEspUri,(const char*)subConfig["url"]);
                strcpy(espHash,(const char*)subConfig["hash"]);
                //static const char* espHash = subConfig["hash"];
                if(loopDebugMode) {
                    debugPrint(F("OTA Type: "), false);
                    debugPrint(m_type2);
                    debugPrint(F("OTA WiFi FW URI: "), false);
                    debugPrint(m_otaEspUri);
                    debugPrint(F("OTA WiFi FW Bin Hash: "), false);
                    debugPrint(espHash);
                }
            }

            subConfig = root["fwBinaries"][1];
            String fwType1 = subConfig["type"];
            if(!(strcmp((const char *)fwType1.c_str(),(const char *)"STM32")))
            {
                strcpy(m_type1,(const char*)fwType1.c_str());
                strcpy(m_otaStmUri,(const char*)subConfig["url"]);
                strcpy(stmHash,(const char*)subConfig["hash"]);
                //static const char* stmHash = subConfig["hash"];
                if(loopDebugMode) {
                    debugPrint(F("OTA Type: "), false);
                    debugPrint(m_type1);
                    debugPrint(F("OTA Main FW URI: "), false);
                    debugPrint(m_otaStmUri);
                    debugPrint(F("OTA Main FW Bin Hash: "), false);
                    debugPrint(stmHash);
                }
            }
            else if(!(strcmp((const char *)fwType1.c_str(),(const char *)"ESP32")))
            {
                strcpy(m_type2,(const char*)fwType1.c_str());
                strcpy(m_otaEspUri,(const char*)subConfig["url"]);
                strcpy(espHash,(const char*)subConfig["hash"]);
              //  static const char* espHash = subConfig["hash"];
                if(loopDebugMode) {
                    debugPrint(F("OTA Type: "), false);
                    debugPrint(m_type2);
                    debugPrint(F("OTA WiFi FW URI: "), false);
                    debugPrint(m_otaEspUri);
                    debugPrint(F("OTA WiFi FW Bin Hash: "), false);
                    debugPrint(espHash);
                }
            }

            if((!(strcmp(m_type1,"STM32"))) && (!(strcmp(m_type2,"ESP32"))))
            {
                if(saveToFlash) {
                    subConfig = root;
                    // Check if the config is new, then save to file and reset
                    iuFlash.saveConfigJson(IUFlash::CFG_OTA, subConfig);
                    if(loopDebugMode) 
                        debugPrint("Saved OTA configuration to file");
                }
                //if (loopDebugMode) { debugPrint(F("Switching Device mode:OPERATION -> OTA")); }
                //changeUsageMode(UsageMode::OTA);
                if(loopDebugMode) {
                    debugPrint(F("Changed Device mode: OTA"));
                }
                delay(1);
                iuWiFi.sendLongMSPCommand(MSPCommand::SET_OTA_STM_URI,300000,m_otaStmUri,512);
                delay(1);
                iuWiFi.sendLongMSPCommand(MSPCommand::SET_OTA_ESP_URI,300000,m_otaEspUri,512); 
                delay(1);
                if(loopDebugMode) {
                    debugPrint(F("Sending FDW_INI_ACK"));
                }
                sendOtaStatusMsg(MSPCommand::OTA_INIT_ACK,OTA_REQ_ACK,OTA_RESPONE_OK);
                delay(1);
                strcpy(fwBinFileName, vEdge_Main_FW_BIN);
                iuOta.otaFileRemove(iuFlash.IUFWTMPIMG_SUBDIR,vEdge_Main_FW_BIN);
                iuOta.otaFileRemove(iuFlash.IUFWTMPIMG_SUBDIR,vEdge_Main_FW_MD5);
                iuOta.otaMD5Write(iuFlash.IUFWTMPIMG_SUBDIR,vEdge_Main_FW_MD5,stmHash);
                
                sendOtaStatusMsg(MSPCommand::OTA_FDW_START,OTA_DOWNLOAD_START,OTA_RESPONE_OK);
                delay(1);
                if(loopDebugMode) {
                    debugPrint(F("Sending OTA_FDW_START"));
                }
                otaFwdnldTmout = millis();
                waitingDnldStrart = true;
                ledManager.stopColorOverride();
                ledManager.showStatus(&STATUS_OTA_DOWNLOAD);
            }
            else
            {
                if(loopDebugMode) {
                    debugPrint(F("Sending OTA_FDW_ABORT"));
                }
                sendOtaStatusMsg(MSPCommand::OTA_FDW_ABORT,OTA_DOWNLOAD_ERR, String(iuOta.getOtaRca(OTA_INVALID_MQTT)).c_str());
                if (loopDebugMode) { debugPrint(F("Switching Device mode:OTA -> OPERATION")); }
                iuWiFi.m_setLastConfirmedPublication();
                changeUsageMode(UsageMode::OPERATION);
                delay(10);
            }
        }
        if(!(strcmp((const char *)m_otaMsgType,(const char *)"ota-rollback")))
        {
            if(loopDebugMode) {
                debugPrint(F("OTA Forced Rollback Request received: "), false);
                subConfig.printTo(Serial); debugPrint("");
            }
            if (loopDebugMode) { debugPrint(F("Switching Device mode:OTA -> OPERATION")); }
            iuWiFi.m_setLastConfirmedPublication();
            changeUsageMode(UsageMode::OPERATION);
            otaInitTimeoutFlag = false; 
            if(doOnceFWValid == true)
            { // Don't accept new OTA request during Validation of Last OTA
                if(loopDebugMode) {
                    debugPrint(F("Last OTA in Progress.. Unable to process OTA Request"));
                }
                return true;
            }
            strcpy(m_rlbkMsgId,(const char*)root["messageId"]);
            debugPrint("MessageId:",false);
            debugPrint(m_rlbkMsgId);
            subConfig = root["ota-rollback"];
            if(subConfig.success()) {
                strcpy(m_rlbkFwVer,(const char*)subConfig["fwVersion"]); 
                //String devId = subConfig["deviceId"];
                m_rlbkDevId.fromString((const char*)subConfig["deviceId"]);            
                m_rlbkDowngrade = subConfig["downgrade"];
                debugPrint("Fw Version:",false);
                debugPrint(m_rlbkFwVer);
                debugPrint("Device ID:",false);
                debugPrint(m_rlbkDevId.toString().c_str());
                debugPrint("Downgrade:",false);
                debugPrint(m_rlbkDowngrade);
                if(conductor.getUsageMode() != UsageMode::OTA) {
                    if(DOSFS.exists(OTA_MAIN_FW3) && DOSFS.exists(OTA_MAIN_CHK3) && 
                       DOSFS.exists(OTA_WIFI_FW3) && DOSFS.exists(OTA_WIFI_CHK3))
                    {
                        if(m_rlbkDevId == m_macAddress && m_rlbkDowngrade == true)
                        {
                            if(saveToFlash) {
                                subConfig = root;
                                // Check if the config is new, then save to file and reset
                                iuFlash.saveConfigJson(IUFlash::CFG_FORCE_OTA, subConfig);
                                if(loopDebugMode) 
                                    debugPrint("Saved OTA configuration to file");
                            }                        
                            if(loopDebugMode) debugPrint("Updating flag for Forced Rollback = 0x05");
                            iuOta.updateOtaFlag(OTA_STATUS_FLAG_LOC,OTA_FW_FORCED_ROLLBACK);
                            iuOta.readOtaFlag();
                            if(loopDebugMode) { 
                                debugPrint("OTA Status Flag:",false);
                                debugPrint(iuOta.getOtaFlagValue(OTA_STATUS_FLAG_LOC));
                                debugPrint("Rebooting Device.....");
                            }
                            delay(2000);
                            DOSFS.end();
                            delay(10);
                            STM32.reset();
                        }
                        else
                        {
                            if(loopDebugMode) 
                                debugPrint("Invalid Device ID for Forced Rollback !");
                        }
                    }
                    else
                    {
                        if(loopDebugMode) 
                            debugPrint("Missing Backup FW file(s) Forced rollback failed.");
                    }
                }
            }
        }
    }

    // Modbus Configuration 
    // modbusSlaveConfig configuration
    // Message is always saved to file, after which STM resets
    subConfig = root["modbusSlaveConfig"];
    if (subConfig.success()) {
        // Validate if the received parameters are correct
        if(loopDebugMode) {
            debugPrint("modbusSlave configuration received: ", false);
            subConfig.printTo(Serial); debugPrint("");
        }
        bool validConfiguration = iuFlash.validateConfig(IUFlash::CFG_MODBUS_SLAVE, subConfig, validationResultString, (char*) m_macAddress.toString().c_str(), getDatetime(), messageId);
        if(loopDebugMode) { 
            debugPrint("Validation: ", false);
            debugPrint(validationResultString); 
            debugPrint("modbusSlave configuration validation result: ", false); 
            debugPrint(validConfiguration);
        }
            
        if(validConfiguration) {
            if(loopDebugMode) debugPrint("Received valid modbusSlave configuration");
        
            // Save the valid configuration to file 
            if(saveToFlash) { 
                // Check if the config is new, then save to file and reset
                iuFlash.saveConfigJson(IUFlash::CFG_MODBUS_SLAVE, subConfig);
                iuFlash.writeInternalFlash(1,CONFIG_MODBUS_SLAVE_CONFIG_FLASH_ADDRESS,jsonChar.length(),(const uint8_t*)jsonChar.c_str());

                if(loopDebugMode) debugPrint("Saved modbusSlave configuration to file");
                
                // Apply the latest modbus Configuration 
                if(loopDebugMode) debugPrint("Apply the current modbusSlave configuration from file");
                iuModbusSlave.setupModbusDevice(subConfig);

                // Acknowledge that configuration has been saved successfully on /ide_plus/command_response/ topic
                // If streaming mode is BLE, send an acknowledgement on BLE as well
                // NOTE: MSPCommand CONFIG_ACK added to Arduino/libraries/IUSerial/src/MSPCommands.h
                iuWiFi.sendMSPCommand(MSPCommand::CONFIG_ACK, validationResultString);
                if(StreamingMode::BLE && isBLEConnected()) { iuBluetooth.write("RECEIVED-MODBUS-SLAVE-CONFIGS;"); delay(100); }

                // Restart STM, setFFTParams will configure FFT parameters in setup()
                delay(3000);  // wait for MQTT message to be published
                DOSFS.end();
                delay(10);
                
            }
        } else {
            if(loopDebugMode) debugPrint("Received invalid modbusSlave configuration");
            // Appy default configurations
            checkforModbusSlaveConfigurations();
            // Acknowledge incorrect configuration, send the errors on /ide_plus/command_response topic
            // If streaming mode is BLE, send an acknowledgement on BLE as well
            iuWiFi.sendMSPCommand(MSPCommand::CONFIG_ACK, validationResultString);
            if(StreamingMode::BLE && isBLEConnected()) { debugPrint("FAILED MODBUS CONFIGS"); iuBluetooth.write("FAILED-MODBUS-SLAVE-CONFIGS;"); delay(100); }
        }
        
    } // If json is incorrect, it will result in parsing error in jsonBuffer.parseObject(json) which will cause the processConfiguration call to return
    
    //Certificates download URL Configuration
    subConfig = root["certUrl"];
    if (subConfig.success()) {
        if (loopDebugMode){  debugPrint("Certificate Download Url:",false);
         subConfig.printTo(Serial); debugPrint("");
         }
        const char* url = root["certUrl"]["host"];
        int port = root["certUrl"]["port"];
        const char* path = root["certUrl"]["path"]; 
        const char* messageId = root["messageId"];
        strcpy(m_otaMsgId,messageId);
        // Send URL to ESP32
        iuWiFi.sendMSPCommand(MSPCommand::SET_CERT_CONFIG_URL,jsonChar.c_str());
        char ack_config[70];
        snprintf(ack_config, 70, "{\"messageId\":\"%s\",\"macId\":\"%s\"}", messageId,m_macAddress.toString().c_str());
            
        if(iuWiFi.isConnected() )
        {  
             if(loopDebugMode){debugPrint("Response : ",false);debugPrint(ack_config);}
             iuWiFi.sendMSPCommand(MSPCommand::CONFIG_ACK,ack_config );
        }
        if(StreamingMode::BLE && isBLEConnected()){// Send ACK to BLE
            if(loopDebugMode){ debugPrint("Common URL Config SUCCESS");}
            iuBluetooth.write("CERT-URL-SUCCESS;");
        }
        debugPrint("Common Endpoint Config Success");
    }
    // Configure Diagnostic URL in ESP32
    subConfig = root["diagnosticUrl"];
    if (subConfig.success()) {
        if (loopDebugMode){  debugPrint("Diagnostic Url:",false);
         subConfig.printTo(Serial); debugPrint("");
         }
        const char* messageId = root["messageId"]  ;
        // Send URL to ESP32
        iuWiFi.sendMSPCommand(MSPCommand::SET_DIAGNOSTIC_URL,jsonChar.c_str());
        char ack_config[70];
        snprintf(ack_config, 70, "{\"messageId\":\"%s\",\"macId\":\"%s\"}", messageId,m_macAddress.toString().c_str());
            
        if(iuWiFi.isConnected() )
        {  
             if(loopDebugMode){debugPrint("Response : ",false);debugPrint(ack_config);}
             iuWiFi.sendMSPCommand(MSPCommand::CONFIG_ACK,ack_config );
        }
        if(StreamingMode::BLE && isBLEConnected()){// Send ACK to BLE
            if(loopDebugMode){ debugPrint("Diagnostic URL Config SUCCESS");}
            iuBluetooth.write("DIAGNOSTIC-URL-SUCCESS;");
        }
        debugPrint("Diagnostic Endpoint Config Success"); 
     }

    subConfig = root["auth_type"];
    if (subConfig.success()) {
        //configureAllFeatures(subConfig);
        bool validConfiguration = iuFlash.validateConfig(IUFlash::CFG_WIFI0, variant, validationResultString, (char*) m_macAddress.toString().c_str(), getDatetime(), messageId);
        if(loopDebugMode) { 
            debugPrint("Validation: ", false);
            debugPrint(validationResultString); 
            debugPrint("WiFi configuration validation result: ", false); 
            debugPrint(validConfiguration);
        }
        bool dataWritten = false;
        if(validConfiguration){
            if (saveToFlash) {
                iuFlash.saveConfigJson(IUFlash::CFG_WIFI0, variant);
                iuFlash.writeInternalFlash(1,CONFIG_WIFI_CONFIG_FLASH_ADDRESS,jsonChar.length(),(const uint8_t*)jsonChar.c_str());
                debugPrint(F("Writing into wifi0 file"));
                dataWritten = true;
                iuWiFi.sendMSPCommand(MSPCommand::CONFIG_ACK, validationResultString);
            }
            if(dataWritten == true){
                iuWiFi.sendMSPCommand(MSPCommand::SEND_WIFI_CONFIG,jsonChar.c_str());
                if(iuWiFi.getPowerMode() != PowerMode::REGULAR){
                    iuWiFi.setPowerMode(PowerMode::REGULAR);
                }
                iuWiFi.configure(variant);
            }
        }else {
            if(loopDebugMode) debugPrint("Received invalid WiFi configuration");
            iuWiFi.sendMSPCommand(MSPCommand::CONFIG_ACK, validationResultString);
        }

    }

    subConfig = root["CONFIG_HASH"];
    if (subConfig.success())
    {
        const char *messageId = root["messageId"];
        snprintf(ack_config, 200, "{\"messageId\":\"%s\",\"macId\":\"%s\",\"configType\":\"hash_ack\"}", messageId, m_macAddress.toString().c_str());
        bool dataWritten = false;
        iuFlash.saveConfigJson(IUFlash::CFG_HASH, variant);
        debugPrint(F("Writing into configHash file"));
        if (iuWiFi.isConnected())
        {
            if (loopDebugMode)
            {
                debugPrint("Response : ", false);
                debugPrint(ack_config);
            }
            iuWiFi.sendMSPCommand(MSPCommand::CONFIG_ACK, ack_config);
        }
    }
    // store the DIG configurations 
    subConfig = root["CONFIG"]["DIG"];
    if(subConfig.success()){
        if(loopDebugMode) {
            debugPrint("Triggers configuration received: ", false);
            subConfig.printTo(Serial); debugPrint("");
        }
        const char* messageId = root["messageId"]  ;
        const char* msgType = root["MSGTYPE"];
        snprintf(ack_config, 200, "{\"messageId\":\"%s\",\"macId\":\"%s\",\"configType\":\"dig_ack\"}", messageId,m_macAddress.toString().c_str());
        iuFlash.saveConfigJson(IUFlash::CFG_DIG, variant);
        if(loopDebugMode) {
            debugPrint("configs saved successfully ");
        }
        clearDiagResultArray();
        configureAlertPolicy();
        
        if(iuWiFi.isConnected() )
        {  
         if(loopDebugMode){debugPrint("Response : ",false);debugPrint(ack_config);}
         iuWiFi.sendMSPCommand(MSPCommand::CONFIG_ACK,ack_config );
        }
        if(StreamingMode::BLE && isBLEConnected()){// Send ACK to BLE
            if(loopDebugMode){ debugPrint("Diagnostic Config SUCCESS");}
            iuBluetooth.write("DIG-CFG-SUCCESS;");
        }
    }

    subConfig = root["CONFIG"]["PHASE"];
    if(subConfig.success()){
        if(loopDebugMode) {
            debugPrint("Phase configuration received: ", false);
            subConfig.printTo(Serial); debugPrint("");
        }
        const char* messageId = root["messageId"];
        snprintf(ack_config, 200, "{\"messageId\":\"%s\",\"macId\":\"%s\",\"configType\":\"phase_ack\"}", messageId,m_macAddress.toString().c_str());
                   
        iuFlash.saveConfigJson(IUFlash::CFG_PHASE, variant);
        if(loopDebugMode) {
            debugPrint("configs saved successfully ");
        }
        if(iuWiFi.isConnected())
        {  
         if(loopDebugMode){debugPrint("Response : ",false);debugPrint(ack_config);}
         iuWiFi.sendMSPCommand(MSPCommand::CONFIG_ACK,ack_config );
        }
        if(StreamingMode::BLE && isBLEConnected()){// Send ACK to BLE
            if(loopDebugMode){ debugPrint("Phase Config SUCCESS");}
            iuBluetooth.write("PHASE-CFG-SUCCESS;");
        }
        checkPhaseConfig();
        
    }
    // TEMP : store the feature output JOSN  
    subConfig = root["FRES"];
    if (subConfig.success()) {
        const char* msgType = root["MSGTYPE"];

        if(strcmp(msgType,"FRES") == 0 ){
            if (loopDebugMode){  debugPrint("Update Feature outputs :",false);
            }
            //availableDiagnosticConfig = variant;
            //variant.printTo(Serial); debugPrint("");
            bool res = iuFlash.saveConfigJson(IUFlash::CFG_FOUT, variant);
            debugPrint("Write status : ",false); debugPrint(res);
            //debugPrint("Read Config : ", false);
            //iuFlash.readConfig(IUFlash::CFG_DIG,)
        }
    }

    // RPM Configs
    subConfig = root["CONFIG"]["RPM"];
    if (subConfig.success()) {
        if (loopDebugMode){  debugPrint("RPM Configs Received :",false);
         subConfig.printTo(Serial); debugPrint("");
         }
        bool validConfiguration = iuFlash.validateConfig(IUFlash::CFG_RPM, variant, validationResultString, (char*) m_macAddress.toString().c_str(), getDatetime(), messageId);
        if (validConfiguration) {
            iuFlash.saveConfigJson(IUFlash::CFG_RPM,subConfig);
            if(loopDebugMode){ debugPrint("Saved RPM Configs Successfully");}  
            // Apply RPM Configs
            int LOW_RPM  = root["CONFIG"]["RPM"]["LOW_RPM"];
            int HIGH_RPM = root["CONFIG"]["RPM"]["HIGH_RPM"]; 
            float RPM_TRH = root["CONFIG"]["RPM"]["RPM_TRH"];
            const char* messageId = root["messageId"];
            if(LOW_RPM < FFTConfiguration::currentLowCutOffFrequency || LOW_RPM > FFTConfiguration::currentHighCutOffFrequency ){
                LOW_RPM = FFTConfiguration::currentLowCutOffFrequency;
            }
            if (HIGH_RPM > FFTConfiguration::currentHighCutOffFrequency || HIGH_RPM < FFTConfiguration::currentLowCutOffFrequency){ 
                 HIGH_RPM = FFTConfiguration::currentHighCutOffFrequency;
            }
            FFTConfiguration::currentLowRPMFrequency = LOW_RPM;
            FFTConfiguration::currentHighRPMFrequency = HIGH_RPM;
            if(RPM_TRH < 0 || RPM_TRH == 0){
                FFTConfiguration::currentRPMThreshold = FFTConfiguration::DEFAULT_RPM_THRESHOLD;
            }else{
                FFTConfiguration::currentRPMThreshold = RPM_TRH;
            }
            if(loopDebugMode){
                debugPrint("LOW_RPM : ",false);debugPrint(FFTConfiguration::currentLowRPMFrequency);
                debugPrint("HIGH_RPM :",false);debugPrint(FFTConfiguration::currentHighRPMFrequency);
                debugPrint("RPM_TRH : ",false);debugPrint(FFTConfiguration::currentRPMThreshold);
                debugPrint("MSGID : ",false);debugPrint(messageId);
            }
            if(iuWiFi.isConnected() )
            {  
                if(loopDebugMode){debugPrint("RPM ACK : ",false);debugPrint(validationResultString);}
                iuWiFi.sendMSPCommand(MSPCommand::CONFIG_ACK,validationResultString );
            }
            if(StreamingMode::BLE && isBLEConnected()){// Send ACK to BLE
                if(loopDebugMode){ debugPrint("RPM Config SUCCESS");}
                iuBluetooth.write("RPM-SUCCESS;");
            }
            DOSFS.end();
            delay(10);
        }else {
            if(loopDebugMode) debugPrint("Received invalid RPM configuration");
            iuWiFi.sendMSPCommand(MSPCommand::CONFIG_ACK, validationResultString);
        }
    }

    
    return true;
}


JsonObject& Conductor::createFeatureGroupjson(){
   
    // DynamicJsonBuffer outputJSONbuffer;
    StaticJsonBuffer<2500> outputJSONbuffer;
    JsonObject& root = outputJSONbuffer.createObject();
    JsonObject& fres = root.createNestedObject("FRES");
    JsonObject& spectralFeatures = outputJSONbuffer.parseObject(fingerprintData);
    // JsonVariant variant = root;
    fres["A93"] = featureDestinations::buff[featureDestinations::basicfeatures::accelRMS512Total];
    fres["VAX"] = featureDestinations::buff[featureDestinations::basicfeatures::velRMS512X];
    fres["VAY"] = featureDestinations::buff[featureDestinations::basicfeatures::velRMS512Y];
    fres["VAZ"] = featureDestinations::buff[featureDestinations::basicfeatures::velRMS512Z];
    fres["A9X"] = featureDestinations::buff[featureDestinations::basicfeatures::accelRMS512X];
    fres["A9Y"] = featureDestinations::buff[featureDestinations::basicfeatures::accelRMS512Y];
    fres["A9Z"] = featureDestinations::buff[featureDestinations::basicfeatures::accelRMS512Z];
    fres["DAX"] = featureDestinations::buff[featureDestinations::basicfeatures::dispRMS512X];
    fres["DAY"] = featureDestinations::buff[featureDestinations::basicfeatures::dispRMS512Y];
    fres["DAZ"] = featureDestinations::buff[featureDestinations::basicfeatures::dispRMS512Z];
    fres["TMP"] = featureDestinations::buff[featureDestinations::basicfeatures::temperature];
    fres["S12"] = featureDestinations::buff[featureDestinations::basicfeatures::audio];
    fres["RPM"] = featureDestinations::buff[featureDestinations::basicfeatures::rpm];
    addAdvanceFeature(fres, phaseAngleComputer.totalPhaseIds, m_phase_ids,phaseAngleComputer.phase_output);
    mergeJson(fres,spectralFeatures);
    fres["NULL"] = 0;
    return root;
}

void Conductor::checkPhaseConfig(){

    JsonObject &Phaseconfig = conductor.configureJsonFromFlash("/iuconfig/phase.conf",1);
    //debugPrint("Phase config read from flash ");
    if(Phaseconfig.success()){
    totalIDs = Phaseconfig["CONFIG"]["PHASE"]["IDS"].size();
    phaseAngleComputer.totalPhaseIds = totalIDs;
    for(size_t i = 0; i < totalIDs; i++){
        m_phase_ids[i] = Phaseconfig["CONFIG"]["PHASE"]["IDS"][i].asString();
        strcpy(&m_ax1[i],(char*)Phaseconfig["CONFIG"]["PHASE"]["AX1"][i].asString());
        strcpy(&m_ax2[i],(char*)Phaseconfig["CONFIG"]["PHASE"]["AX2"][i].asString());
        m_trh[i] = Phaseconfig["CONFIG"]["PHASE"]["TRH"][i];
    }
    if (setupDebugMode) {
    for(size_t i = 0; i < totalIDs; i++){
        debugPrint("IDS : ", false);debugPrint(m_phase_ids[i]);
        debugPrint("AX1 : ", false);debugPrint(m_ax1[i]);
        debugPrint("AX2 : ", false);debugPrint(m_ax2[i]);
        debugPrint("TRH : ", false);debugPrint(m_trh[i]);
    }
  }
    }
    else { 
        if (setupDebugMode){debugPrint("Failed to read phase.conf file ");}
    }
}

void Conductor::computeAdvanceFeature(){
    
    for(size_t i=0; i < totalIDs; i++){
        phaseAngleComputer.phase_output[i] = phaseAngleComputer.computePhaseDiff(m_ax1[i],m_ax2[i]);
       // debugPrint("Phase difference : ",false);
        //debugPrint(m_phase_ids[i]);
        // debugPrint(" : ",false);
        //debugPrint(phaseAngleComputer.phase_output[i]);
    }
}

/*
 * Read the OTA Configutation details
 * 
 */
void Conductor::readOtaConfig()
{
    JsonObject& config = conductor.configureJsonFromFlash("/iuconfig/ota.conf",1);
    strcpy(m_otaMsgId,(const char *)config["messageId"]);
    if(loopDebugMode) {
        debugPrint("OTA m_otaMsgId: ",false);
        debugPrint(m_otaMsgId);
    }
    strcpy(m_otaFwVer,(const char*)config["fwVersion"]);

    JsonVariant subConfig = config["fwBinaries"][0];
    String fwType = subConfig["type"];
    if(!(strcmp((const char *)fwType.c_str(),(const char *)"STM32")))
    {
        strcpy(m_type1,(const char*)fwType.c_str());
        strcpy(m_otaStmUri,(const char*)subConfig["url"]);
        strcpy(stmHash,(const char*)subConfig["hash"]);
        //static const char* stmHash = subConfig["hash"];
        if(loopDebugMode) {
            debugPrint(F("OTA Type: "), false);
            debugPrint(m_type1);
            debugPrint(F("OTA Main FW URI: "), false);
            debugPrint(m_otaStmUri);
            debugPrint(F("OTA Main FW Bin Hash: "), false);
            debugPrint(stmHash);
        }
    }
    else if(!(strcmp((const char *)fwType.c_str(),(const char *)"ESP32")))
    {
        strcpy(m_type2,(const char*)fwType.c_str());
        strcpy(m_otaEspUri,(const char*)subConfig["url"]);
        strcpy(espHash,(const char*)subConfig["hash"]);
        //static const char* espHash = subConfig["hash"];
        if(loopDebugMode) {
            debugPrint(F("OTA Type: "), false);
            debugPrint(m_type2);
            debugPrint(F("OTA WiFi FW URI: "), false);
            debugPrint(m_otaEspUri);
            debugPrint(F("OTA WiFi FW Bin Hash: "), false);
            debugPrint(espHash);
        }
    }

    subConfig = config["fwBinaries"][1];
    String fwType1 = subConfig["type"];
    if(!(strcmp((const char *)fwType1.c_str(),(const char *)"STM32")))
    {
        strcpy(m_type1,(const char*)fwType1.c_str());
        strcpy(m_otaStmUri,(const char*)subConfig["url"]);
        strcpy(stmHash,(const char*)subConfig["hash"]);
        //static const char* stmHash = subConfig["hash"];
        if(loopDebugMode) {
            debugPrint(F("OTA Type: "), false);
            debugPrint(m_type1);
            debugPrint(F("OTA Main FW URI: "), false);
            debugPrint(m_otaStmUri);
            debugPrint(F("OTA Main FW Bin Hash: "), false);
            debugPrint(stmHash);
        }
    }
    else if(!(strcmp((const char *)fwType1.c_str(),(const char *)"ESP32")))
    {
        strcpy(m_type2,(const char*)fwType1.c_str());
        strcpy(m_otaEspUri,(const char*)subConfig["url"]);
        strcpy(espHash,(const char*)subConfig["hash"]);
        //  static const char* espHash = subConfig["hash"];
        if(loopDebugMode) {
            debugPrint(F("OTA Type: "), false);
            debugPrint(m_type2);
            debugPrint(F("OTA WiFi FW URI: "), false);
            debugPrint(m_otaEspUri);
            debugPrint(F("OTA WiFi FW Bin Hash: "), false);
            debugPrint(espHash);
        }
    }
}


/*
 * Read the OTA Configutation details
 * 
 */
void Conductor::readForceOtaConfig()
{
    JsonObject& config = conductor.configureJsonFromFlash("/iuconfig/force_ota.conf",1);
    strcpy(m_rlbkMsgId,(const char*)config["messageId"]);
    debugPrint("MessageId:",false);
    debugPrint(m_rlbkMsgId);
    JsonVariant subConfig = config["ota-rollback"];
    if(subConfig.success()) {
        strcpy(m_rlbkFwVer,(const char*)subConfig["fwVersion"]); 
        //String devId = subConfig["deviceId"];
        m_rlbkDevId.fromString((const char*)subConfig["deviceId"]);            
        m_rlbkDowngrade = subConfig["downgrade"];
        if(loopDebugMode) {
            debugPrint("Fw Version:",false);
            debugPrint(m_rlbkFwVer);
            debugPrint("Device ID:",false);
            debugPrint(m_rlbkDevId.toString().c_str());
            debugPrint("Downgrade:",false);
            debugPrint(m_rlbkDowngrade);
        }
    }
}

/*
 * Read the MQTT Configutation details
 * 
 */

 void Conductor::configureMQTTServer(String filename){

  // Open the configuration file
  File myFile = DOSFS.open(filename,"r");
  
  const size_t bufferSize = JSON_OBJECT_SIZE(1) + JSON_OBJECT_SIZE(4) + 208;
  StaticJsonBuffer<bufferSize> jsonBuffer;

  // Parse the root object
  JsonObject &root = jsonBuffer.parseObject(myFile);

  if (!root.success() && !iuFlash.checkConfig(CONFIG_MQTT_FLASH_ADDRESS)){
    debugPrint(F("Failed to read MQTT.conf file, using default configuration"));
    setDefaultMQTT();
    // flashStatusFlag = true;
    //m_accountid = "XXXAdmin";
  }
  else if(iuFlash.checkConfig(CONFIG_MQTT_FLASH_ADDRESS) && !root.success()){
        String mqttConfig = iuFlash.readInternalFlash(CONFIG_MQTT_FLASH_ADDRESS);
        debugPrint(mqttConfig);
        JsonObject &config = jsonBuffer.parseObject(mqttConfig);
        if(config.success())
        {
            debugPrint("Mqtt Config Found");
            m_mqttServerIp = config["mqtt"]["mqttServerIP"];

            int mqttport = config["mqtt"]["port"];
            //debugPrint("INside MQTT.conf .......");
            // m_mqttServerIp.fromString(mqttServerIP);//mqttServerIP;
            m_mqttServerPort = mqttport;
            m_mqttUserName = config["mqtt"]["username"]; //MQTT_DEFAULT_USERNAME;
            m_mqttPassword = config["mqtt"]["password"]; //MQTT_DEFAULT_ASSWORD;
            // m_tls_enabled = config["mqtt"]["tls"];
            m_accountId = config["accountid"];
            File mqttFile = DOSFS.open("MQTT.conf","w");
            if(mqttFile)
            {
                mqttFile.print(mqttConfig.c_str());
                mqttFile.flush();
                debugPrint("MQTT.conf File write Success");
                flashStatusFlag = true;
                mqttFile.close();
            }

            iuWiFi.hardReset();
            if (debugMode) {
                debugPrint(F("MQTT ServerIP :"),false);
                debugPrint(m_mqttServerIp);
                debugPrint(F("Mqtt Port :"),false);
                debugPrint(m_mqttServerPort);
                debugPrint(F("Mqtt UserName :"),false);
                debugPrint(m_mqttUserName);
                debugPrint(F("Mqtt Password :"),false);
                debugPrint(m_mqttPassword);
                // debugPrint(F("TLS ENABLE:"),false);
                // debugPrint(m_tls_enabled);
                debugPrint(F("Account ID :"));
                debugPrint(m_accountId);
            }   
        }else{
            setDefaultMQTT();
        }
  }
 else {
  
  m_mqttServerIp = root["mqtt"]["mqttServerIP"];
  int mqttport = root["mqtt"]["port"];
   //debugPrint("INside MQTT.conf .......");
//   m_mqttServerIp.fromString(mqttServerIP);//mqttServerIP;
  m_mqttServerPort = mqttport;
  m_mqttUserName = root["mqtt"]["username"]; //MQTT_DEFAULT_USERNAME;
  m_mqttPassword = root["mqtt"]["password"]; //MQTT_DEFAULT_ASSWORD;
//   m_tls_enabled = root["mqtt"]["tls"];
  m_accountId = root["accountid"];
  
  iuWiFi.hardReset();
  if (debugMode) {
        debugPrint(F("MQTT ServerIP :"),false);
        debugPrint(m_mqttServerIp);
        debugPrint(F("Mqtt Port :"),false);
        debugPrint(m_mqttServerPort);
        debugPrint(F("Mqtt UserName :"),false);
        debugPrint(m_mqttUserName);
        debugPrint(F("Mqtt Password :"),false);
        debugPrint(m_mqttPassword);
        // debugPrint(F("TLS ENABLE:"),false);
        // debugPrint(m_tls_enabled);
        debugPrint(F("Account ID :"));
        debugPrint(m_accountId);
  }   
 
 }
  myFile.close();
  
}
/*
 * swap credentails
 */
// void Conductor::fastSwap (const char **i, const char **d)
// {
//     const char *t = *d;
//     *d = *i;
//     *i = t;
// }

/*
 * get the Board Configuration data
 * 
 * Used to configure http endpoint configurations
 */


bool Conductor::configureBoardFromFlash(String filename,bool isSet){
  
  if(isSet != true){

    return false;
  }
  
  // Open the configuration file
 
  File myFile = DOSFS.open(filename,"r");
  const size_t bufferSize = JSON_OBJECT_SIZE(1) + JSON_OBJECT_SIZE(6) + 510;
  StaticJsonBuffer<bufferSize> jsonBuffer;

  // Parse the root object
  JsonObject &root = jsonBuffer.parseObject(myFile);
  if (!root.success() && !iuFlash.checkConfig(CONFIG_HTTP_FLASH_ADDRESS)){
    debugPrint(F("Failed to read httpConf.conf file, using default configuration"));
    setDefaultHTTP();

  }else if(iuFlash.checkConfig(CONFIG_HTTP_FLASH_ADDRESS) && !root.success()){
      String httpConfig = iuFlash.readInternalFlash(CONFIG_HTTP_FLASH_ADDRESS);
        debugPrint(httpConfig);
        JsonObject &config = jsonBuffer.parseObject(httpConfig);
        if(config.success())
        {
            debugPrint("Http Config Found");

            m_httpHost = config["httpConfig"]["host"];
            m_httpPort = config["httpConfig"]["port"];
            m_httpPath = config["httpConfig"]["path"];
            m_httpUsername = config["httpConfig"]["username"];
            m_httpPassword = config["httpConfig"]["password"];
            m_httpOauth = config["httpConfig"]["oauth"];

            if(config.containsKey("httpOem")){
                m_httpHost_oem = config["httpOem"]["host"];
                m_httpPort_oem = config["httpOem"]["port"];
                m_httpPath_oem = config["httpOem"]["path"];
                m_httpUsername_oem = config["httpOem"]["username"];
                m_httpPassword_oem = config["httpOem"]["password"];
                m_httpOauth_oem = config["httpOem"]["oauth"];
            }
            File httpFile = DOSFS.open("httpConfig.conf","w");
            if(httpFile)
            {
                httpFile.print(httpConfig.c_str());
                httpFile.flush();
                debugPrint("httpConfig.conf File write Success");
                flashStatusFlag = true;
                httpFile.close();
            }

            if(debugMode){
                debugPrint("FROM configureBoardFromFlash :");

                debugPrint(F("Http Host :"),false);
                debugPrint(m_httpHost);
                //debugPrint(":");
                debugPrint(F("Port:"),false);
                debugPrint(m_httpPort);
                debugPrint(F("Path :"),false);
                debugPrint(m_httpPath);
                debugPrint(F("UserName :"),false);
                debugPrint(m_httpUsername);
                debugPrint(F("Password :"),false);
                debugPrint(m_httpPassword);
                debugPrint(F("Oauth :"),false);
                debugPrint(m_httpOauth);
                }
            }else{
                debugPrint(F("Failed to read httpConf.conf file, using default configuration"));
                setDefaultHTTP();
            }
        }
 else {

        // Read configuration from the file

        m_httpHost = root["httpConfig"]["host"];
        m_httpPort = root["httpConfig"]["port"];
        m_httpPath = root["httpConfig"]["path"];
        m_httpUsername = root["httpConfig"]["username"];
        m_httpPassword = root["httpConfig"]["password"];
        m_httpOauth = root["httpConfig"]["oauth"];

        if(root.containsKey("httpOem")){
            m_httpHost_oem = root["httpOem"]["host"];
            m_httpPort_oem = root["httpOem"]["port"];
            m_httpPath_oem = root["httpOem"]["path"];
            m_httpUsername_oem = root["httpOem"]["username"];
            m_httpPassword_oem = root["httpOem"]["password"];
            m_httpOauth_oem = root["httpOem"]["oauth"];
        }

if(debugMode){
  debugPrint("FROM configureBoardFromFlash :");

  debugPrint(F("Http Host :"),false);
  debugPrint(m_httpHost);
  //debugPrint(":");
  debugPrint(F("Port:"),false);
  debugPrint(m_httpPort);
  debugPrint(F("Path :"),false);
  debugPrint(m_httpPath);
  debugPrint(F("UserName :"),false);
  debugPrint(m_httpUsername);
  debugPrint(F("Password :"),false);
  debugPrint(m_httpPassword);
  debugPrint(F("Oauth :"),false);
  debugPrint(m_httpOauth);
  }
}
 myFile.close();
 
 return true;
}

/*
 * configureJsonFromFlash(char* filename,bool isSet)
 * 
 * 
 */
JsonObject& Conductor:: configureJsonFromFlash(String filename,bool isSet){

  if(isSet != true){

    //return false;
  }
  
 // Open the configuration file
 
  File myFile = DOSFS.open(filename,"r");
  
  //StaticJsonBuffer<1024> jsonBuffer;
  //const size_t bufferSize = JSON_ARRAY_SIZE(8) + JSON_OBJECT_SIZE(300) + 60;        // dynamically allociated memory
  const size_t bufferSize = JSON_OBJECT_SIZE(2) + 2*JSON_OBJECT_SIZE(4) + 11*JSON_OBJECT_SIZE(5) + JSON_OBJECT_SIZE(13) + 1396;
  DynamicJsonBuffer jsonBuffer(bufferSize);
 // Parse the root object 
  JsonObject &root = jsonBuffer.parseObject(myFile);
  //JsonObject& root2 = root["fingerprints"];
  
  if (!root.success()){
    debugPrint(F("Failed to read file, using default configuration"));
   
  }
 else {
  // close file
  myFile.close();

 }
    
 return root;     // JSON Object
}

/**
 * Device level configuration
 *
 * @return True if the configuration is valid, else false.
 */
void Conductor::configureMainOptions(JsonVariant &config)
{
    JsonVariant value = config["GRP"];
    if (value.success()) {        
        deactivateAllGroups();
        bool mainGroupFound = false;
        // activateGroup(&healthCheckGroup);  // Health check is always active
        FeatureGroup *group;
        const char* groupName;
        for (uint8_t i = 0; i < FeatureGroup::instanceCount; ++i) {
            groupName = value[i];
            if (groupName != NULL) {
                group = FeatureGroup::getInstanceByName(groupName);
                if (group) {
                    activateGroup(group);
                    if (!mainGroupFound) {
                        changeMainFeatureGroup(group);
                        mainGroupFound = true;
                    }
                }
            }
        }
    }
    // Raw Data publication period
    value = config["RAW"];
    if (value.success()) {
        m_rawDataPublicationTimer = (uint32_t) (value.as<int>()) * 1000;
    }
    // Sleep management
    bool resetCycleTime = false;
    value = config["POW"];
    if (value.success()) {
        m_sleepMode = (sleepMode) (value.as<int>());
        resetCycleTime = true;
    }
    value = config["TSL"];
    if (value.success()) {
        m_autoSleepDelay = (uint32_t) (value.as<int>()) * 1000;
        resetCycleTime = true;
    }
    value = config["TOFF"];
    if (value.success()) {
        m_sleepDuration = (uint32_t) (value.as<int>()) * 1000;
        resetCycleTime = true;
    }
    value = config["TCY"];
    if (value.success()) {
        m_cycleTime = (uint32_t) (value.as<int>()) * 1000;
        resetCycleTime = true;
    }
    if (resetCycleTime) {
        m_startTime = millis();
    }
    value = config["DSP"];
    if(value.success()){
        m_mainFeatureGroup->setDataSendPeriod(value.as<uint16_t>());
        // NOTE: Older firmware device will not set this parameter even if configJson contains it.
    }
    value = config["DDSP"];
    if(value.success()){
        diagStreamingPeriod = (uint32_t) (value.as<int>()) * 1000;
    }
    value = config["FDSP"];
    if(value.success()){
        fresPublishPeriod = (uint32_t) (value.as<int>()) * 1000;
    }
    value = config["DIG"];
    if(value.success()){
        digStream = (bool) (value.as<bool>());
    }
    value = config["FRES"];
    if(value.success()){
        fresStream = (bool) (value.as<bool>());
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
    for (uint8_t i = 0; i < Feature::instanceCount; ++i) {
        feature = Feature::instances[i];
        myConfig = config[feature->getName()];
        if (myConfig.success()) {
            feature->configure(myConfig);  // Configure the feature
            computer = feature->getComputer();
            if (computer) {
                computer->configure(myConfig);  // Configure the computer
            }
            if (debugMode) {
                debugPrint(F("Configured feature "), false);
                debugPrint(feature->getName());
            }
        }
    }
}

/**
 * Process the commands
 */
void Conductor::processCommand(char *buff)
{
    IPAddress tempAddress;
    size_t buffLen = strlen(buff);
    
    switch(buff[0]) {
        case 'A': // ping device
            if (strcmp(buff, "ALIVE") == 0) {
                // Blink the device to let the user know that is is live
                ledManager.showStatus(&STATUS_IS_ALIVE);
                uint32_t startT = millis();
                uint32_t current = startT;
                while (current - startT < 3000) {
                    ledManager.updateColors();
                    current = millis();
                }
                ledManager.resetStatus();
            }
            break;
        case 'F': {
            // fetch FFT configuration
            if (strncmp(buff, "FFT-CFG-SR", 10) == 0) {
                // TODO: cannot be tested properly with Rigado, as streaming mode won't update properly unless BLE heart-beat is received
                if(m_streamingMode == StreamingMode::BLE || m_streamingMode ==  StreamingMode::WIFI_AND_BLE) {
                    char samplingRateString[8];
                    itoa(FFTConfiguration::currentSamplingRate, samplingRateString, 10);
                    iuBluetooth.write("SR:");
                    iuBluetooth.write(samplingRateString);
                    iuBluetooth.write(";");
                    if (loopDebugMode) { debugPrint("FFT: Sampling Rate sent over BLE: ", false); debugPrint(FFTConfiguration::currentSamplingRate); }
                }
            } else if (strncmp(buff, "FFT-CFG-BS", 10) == 0) {
                if(m_streamingMode == StreamingMode::BLE || m_streamingMode == StreamingMode::WIFI_AND_BLE) {
                    char blockSizeString[8];
                    itoa(FFTConfiguration::currentBlockSize, blockSizeString, 10);
                    iuBluetooth.write("BS:");
                    iuBluetooth.write(blockSizeString);
                    iuBluetooth.write(";");
                    if (loopDebugMode) { debugPrint("FFT: Block Size sent over BLE: ", false); debugPrint(FFTConfiguration::currentBlockSize); }
                }
            }
            break;
        }    
        case 'G': // set feature group
            if (strncmp(buff, "GROUP-", 6) == 0) {
                FeatureGroup *group = FeatureGroup::getInstanceByName(&buff[6]);
                if (group) {
                    deactivateAllGroups();
                    m_mainFeatureGroup = group;
                    activateGroup(m_mainFeatureGroup);
                    if (loopDebugMode) {
                        debugPrint(F("Feature group activated: "), false);
                        debugPrint(m_mainFeatureGroup->getName());
                    }
                }
            }
            else if (strcmp(buff,"GET_MODBUS_CONFIG") == 0)
            {
                 if (isBLEConnected())
                 {
                    char modbusConfig[30];
                    if(iuModbusSlave.parity_byte == 0 ){  iuModbusSlave.m_parity = "NONE";} // TODO : Parity data got currupt after one time configured so added extra conditions
                    if(iuModbusSlave.parity_byte == 1 ){  iuModbusSlave.m_parity = "EVEN";}
                    if(iuModbusSlave.parity_byte == 2 ){  iuModbusSlave.m_parity = "ODD";}

                    snprintf(modbusConfig, sizeof(modbusConfig)/sizeof(char), "MS-%d,%d,%d,%d,%s-SM;",iuModbusSlave.m_id,iuModbusSlave.m_baud,
                    iuModbusSlave.m_databit,iuModbusSlave.m_stopbit,iuModbusSlave.m_parity);
                    iuBluetooth.write(modbusConfig);
                    if(debugMode){
                        debugPrint("MODBUS DEBUG : ",false);
                        debugPrint(modbusConfig);
                    }
                }
            }
            break;
        case 'I': // ping device
            if (strcmp(buff, "IDE-HARDRESET") == 0)
            {
                if (isBLEConnected()) {
                    iuBluetooth.write("WIFI-DISCONNECTED;");
                    delay(2);
                    iuBluetooth.write("MQTT-DISCONNECTED;");
                }
                DOSFS.end();
                delay(10);
                STM32.reset();          
            }
            else if (strcmp(buff, "IDE-GET-VERSION") == 0)
            {
                if (isBLEConnected()) {
                    iuBluetooth.write("IDE-VERSION-");
                    iuBluetooth.write(FIRMWARE_VERSION);
                    iuBluetooth.write(';');
                }
            }
            else if (strcmp(buff, "IUGET_FIRMWARE_VERSION") == 0)
            {
                if (iuWiFi.isConnected()) {
                    char message[128];
                    strcat(message, "{\"device_id\":\"");
                    strcat(message, m_macAddress.toString().c_str());
                    strcat(message, "\",\"wifi_macId\":\"");
                    strcat(message, iuWiFi.getMacAddress().toString().c_str());
                    strcat(message, "\",\"mainFirmware_ver\":");
                    strcat(message, FIRMWARE_VERSION);
                    strcat(message, ",\"wifiFirmware_ver\":");
                    strcat(message, iuWiFi.espFirmwareVersion);
                    strcat(message, "}");
                    iuWiFi.sendMSPCommand(MSPCommand::PUBLISH_FIRMWARE_VER,message);
                }
            }
            else if (strcmp(buff, "IUSET_ERASE_EXT_FLASH") == 0)
                {
                    debugPrint("Formating Flash Please wait");
                    ledManager.overrideColor(RGB_RED);
                    DOSFS.format();
                    ledManager.overrideColor(RGB_WHITE);
                    DOSFS.end();
                    debugPrint("Format Successfully");
                    delay(2000);
                    STM32.reset();
                }
            break;
        case 'P': 
        {
            if (strcmp(buff, "PUBLISH_DEVICE_DETAILS_MQTT") == 0) {
                char deviceDetailsString[250];
                StaticJsonBuffer<250> deviceDetailsBuffer;
                JsonObject& deviceDetails = deviceDetailsBuffer.createObject();
                deviceDetails["mac"] = m_macAddress.toString().c_str();
                deviceDetails["fft_blockSize"] = FFTConfiguration::currentBlockSize;
                deviceDetails["fft_samplingRate"] = FFTConfiguration::currentSamplingRate;
                deviceDetails["host_firmware_version"] = FIRMWARE_VERSION;
                deviceDetails["wifi_firmware_version"] = iuWiFi.espFirmwareVersion;
                deviceDetails["data_send_period"] = m_mainFeatureGroup->getDataSendPeriod(); // milliseconds
                deviceDetails["feature_group"] = m_mainFeatureGroup->getName();
                deviceDetails["raw_data_period"] = m_rawDataPublicationTimer / 1000;  // seconds
                deviceDetails.printTo(deviceDetailsString);
                debugPrint("INFO deviceDetailsString : ", false);
                debugPrint(deviceDetailsString);
                debugPrint("size of device details string : ",false);
                debugPrint(strlen(deviceDetailsString));
                iuWiFi.sendMSPCommand(MSPCommand::PUBLISH_DEVICE_DETAILS_MQTT, deviceDetailsString);
            }
            break;
        }
        case 'S':
            if (strncmp(buff, "SET-MQTT-IP-", 12) == 0) {
                // if (tempAddress.fromString(&buff[12])) {
                    m_mqttServerIp = &buff[12];     // temp adress ?
                    //Serial.print("mqtt ip :");Serial.println(m_mqttServerIp);
                    iuWiFi.hardReset();
                    if (m_streamingMode == StreamingMode::BLE ||
                        m_streamingMode == StreamingMode::WIFI_AND_BLE)
                    {
                        if (loopDebugMode) {
                            debugPrint("Set MQTT server IP: ", false);
                            debugPrint(m_mqttServerIp);
                            
                        }
                        iuBluetooth.write("SET-MQTT-OK;");
                    }
                     //Serial.print("MQTT IP Address :");Serial.println(m_mqttServerIp);
                // }
            }
        case '3':  // Collect acceleration raw data
            iuWiFi.sendHostSamplingRate(FFTConfiguration::calculatedSamplingRate); // updated freq after GET_FFT
            if ((buff[7] == '0' && buff[9] == '0' && buff[11] == '0' &&
                buff[13] == '0' && buff[15] == '0' && buff[17] == '0') && !RawDataState::rawDataTransmissionInProgress)
            {
                if (loopDebugMode) {
                    debugPrint("Record mode");
                }
                delay(500);
                if (m_streamingMode == StreamingMode::WIFI || m_streamingMode == StreamingMode::WIFI_AND_BLE) {
                    rawDataRequest();
                    // startRawDataSendingSession(); // Will be handled in the main loop
                } else if (m_streamingMode == StreamingMode::ETHERNET) {
                    sendAccelRawData(0);  // Axis X
                    sendAccelRawData(1);  // Axis Y
                    sendAccelRawData(2);  // Axis Z
                }
                resetDataAcquisition();
            }
            break;
        case '4':              // Set temperature Offset value  [4000:12 < command-value>]
            if (buff[0] == '4' && buff[3] == '0' && buff[4]==':') {
                
                int id; 
                char temperatureJSON[60];
                sscanf(buff,"%d:%d",&id,&m_temperatureOffset);
                snprintf(temperatureJSON, 60, "{\"sensorConfig\":{\"TMP_OFFSET\":%d,\"SND_OFFSET\":%d } }",m_temperatureOffset,m_audioOffset);
                 
                processConfiguration(temperatureJSON,true);    
                
                }
            break;
        case '5':              // Set Audio Offset value  [5000:12 < command-value>]
            if (buff[0] == '5' && buff[3] == '0' && buff[4]==':') {
                
                int id; 
                char audioOffsetJSON[60];
                sscanf(buff,"%d:%d",&id,&m_audioOffset);
                snprintf(audioOffsetJSON,60,"{\"sensorConfig\":{\"SND_OFFSET\":%d,\"TMP_OFFSET\":%d }} ",m_audioOffset,m_temperatureOffset);
                processConfiguration(audioOffsetJSON,true); 
                
                }
            break; 
        case 'E' :
            if (buff[0]=='E' && buff[1]=='N'&&buff[2]=='A'&&buff[3]=='B'&&buff[4]=='L'&&buff[5]=='E') // ENABLE-ISR
            {
                //Start the Sensor DATA AcquisitionMode
                FeatureStates::isISRActive = true;
                debugPrint(F("ISR-ENABLE from USB !!!"));
            }
            break;
        case 'D' :
            if (buff[0]=='D' && buff[1]=='I'&&buff[2]=='S'&&buff[3]=='A'&&buff[4]=='B'&&buff[5]=='L'&&buff[6]=='E') // DISABLE-ISR
            {
                //Start the Sensor DATA AcquisitionMode
                if ( FFTConfiguration::currentSensor == FFTConfiguration::lsmSensor)
                {
                    detachInterrupt(digitalPinToInterrupt(IULSM6DSM::INT1_PIN));
                }
                else
                {
                    detachInterrupt(digitalPinToInterrupt(IUKX222::INT1_PIN));
                }
                FeatureStates::isISRActive = false;
                FeatureStates::isISRDisabled = true;
                debugPrint(F("ISR-DISABLE form USB !!!"));
            }
            break;  
        case 'C':
            // CERT-UPGRADE-MQTT-SSL - Command to trigger mqtt and ssl cert Upgrade
            if(iuWiFi.isConnected()) {
                if(strcmp(buff,"CERT-UPGRADE") == 0){
                    if (debugMode)
                    {
                        debugPrint("Certificate Upgrade Command :",false);
                        debugPrint(buff);
                    }
                    m_mqttConnected = false;
                    iuWiFi.sendMSPCommand(MSPCommand::CERT_UPGRADE_TRIGGER);
                }
            }
            break;
        default:
            break;
    }
}

/**
 *
 */
void Conductor::processUserCommandForWiFi(char *buff,
                                          IUSerial *feedbackSerial)
{
    if (strcmp(buff, "WIFI-GET-MAC") == 0) {
        if (iuWiFi.getMacAddress()) {
            feedbackSerial->write("WIFI-MAC-");
            feedbackSerial->write(iuWiFi.getMacAddress().toString().c_str());
            feedbackSerial->write(';');
        }
    } else if (strcmp(buff, "WIFI-DISABLE") == 0) {
        iuWiFi.setPowerMode(PowerMode::DEEP_SLEEP);
        updateStreamingMode();
        if (isBLEConnected()) {
            iuBluetooth.write("WIFI-DISCONNECTED;");
            delay(2);
            iuBluetooth.write("MQTT-DISCONNECTED;");
        }
    } else {
        iuWiFi.setPowerMode(PowerMode::REGULAR);
        // We want the WiFi to do something, so need to make sure it's available
        if (!iuWiFi.isAvailable()) {
            // Show the status to the user
            ledManager.showStatus(&STATUS_WIFI_WORKING);
            uint32_t startT = millis();
            uint32_t current = startT;
            // Wait for up to 3sec the WiFi wake up
            while (!iuWiFi.isAvailable() && current - startT < 3000) {
                iuWiFi.readMessages();
                ledManager.updateColors();
                current = millis();
            }
            ledManager.resetStatus();
        }
        // Process message
        iuWiFi.processUserMessage(buff, &iuFlash);
        // Show status
        if (iuWiFi.isAvailable() && iuWiFi.isWorking()) {
            ledManager.showStatus(&STATUS_WIFI_WORKING);
        }
    }
}


/**
 * Process the legacy commands
 */
void Conductor::processLegacyCommand(char *buff)
{
    // TODO Command protocol redefinition required
    switch (buff[0]) {
        case '0': // Set Thresholds
            if (buff[4] == '-' && buff[9] == '-' && buff[14] == '-') {
                int idx(0), th1(0), th2(0), th3(0);
                sscanf(buff, "%d-%d-%d-%d", &idx, &th1, &th2, &th3);
                if (idx == 1 || idx == 2 || idx == 3) {
                    opStateComputer.setThresholds(idx,
                                                  (float) th1 / 100.,
                                                  (float) th2 / 100.,
                                                  (float) th3 / 100.);
                }
                else {
                    opStateComputer.setThresholds(idx, (float) th1, (float) th2,
                                                  (float) th3);
                }
               
            }
            
            break;
        case '1':  // Receive the timestamp data from the bluetooth hub
            if (buff[1] == ':' && buff[12] == '.') {
                setRefDatetime(&buff[2]);
            }
            break;
        case '2':  // Bluetooth parameter setting
            if (buff[1] == ':' && buff[7] == '-' && buff[13] == '-') {
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
                opStateComputer.deleteAllSources();
                for (uint8_t i = 0; i < 12; i++) {
                    feat = m_mainFeatureGroup->getFeature(i);
                    if (feat) {
                        opStateComputer.addSource(feat, 1, fcheck[i] > 0);
                    }
                }
                activateFeature(&opStateFeature);
            }
            break;
        default:
            break;
    }
}


/**
 * Process the USB commands
 */
void Conductor::processUSBMessage(IUSerial *iuSerial)
{
    char *buff = iuSerial->getBuffer();
    processCommand(buff);
    
    // // resetting BLE from USB, while testing
    // if (strcmp(buff, "BLE-RESET-USB")) {
    //     debugPrint("ON BOOT, BLE BUFFER : ", false); debugPrint(buff);
    //     iuBluetooth.softReset();
    //     debugPrint("Resetting BLE on USB trigger");
    // }

    if (buff[0] == '{') {
        processConfiguration(buff, true);
    } else if (strncmp(buff, "WIFI-", 5) == 0) {
        processUserCommandForWiFi(buff, &iuUSB);
    } else if (strncmp(buff, "MCUINFO", 7) == 0) {
        streamMCUUInfo(iuUSB.port);
    } else {
        // Usage mode Mode switching
        char *result = NULL;
        switch (m_usageMode) {
            case UsageMode::CALIBRATION:
                if (strcmp(buff, "IUCAL_END") == 0) {
                    iuUSB.port->println(END_CONFIRM);
                    changeUsageMode(UsageMode::OPERATION);
                }
                break;
            case UsageMode::EXPERIMENT:
                if (strcmp(buff, "IUCMD_END") == 0) {
                    iuUSB.port->println(END_CONFIRM);
                    changeUsageMode(UsageMode::OPERATION);
                    return;
                }
                result = strstr(buff, "Arange");
                    if (result != NULL) {
                    
                    if ( FFTConfiguration::currentSensor == FFTConfiguration::lsmSensor)
                    {
                        switch (result[7] - '0') {
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
                    }
                    else
                    {
                        switch (result[7] - '0') {
                            case 0:
                                iuAccelerometerKX222.setScale(iuAccelerometerKX222.FSR_8G);
                                break;
                            case 1:
                                iuAccelerometerKX222.setScale(iuAccelerometerKX222.FSR_16G);
                                break;
                            case 2:
                                iuAccelerometerKX222.setScale(iuAccelerometerKX222.FSR_32G);
                                break;
                        }
                    }
                    return;
                }
                result = strstr(buff, "rgb");
                if (result != NULL) {
                    ledManager.overrideColor(RGBColor(255 * (result[7] - '0'),
                                                      255 * (result[8] - '0'),
                                                      255 * (result[9] - '0')));
                    return;
                }
                result = strstr(buff, "acosr");
                if (result != NULL) {
                    // Change audio sampling rate
                    int A = result[6] - '0';
                    int B = result[7] - '0';
                    uint16_t samplingRate = (uint16_t) ((A * 10 + B) * 1000);
                    iuI2S.setSamplingRate(samplingRate);
                    return;
                }
                result = strstr(buff, "accsr");
                if (result != NULL) {
                    int A = result[6] - '0';
                    int B = result[7] - '0';
                    int C = result[8] - '0';
                    int D = result[9] - '0';
                    int samplingRate = (A * 1000 + B * 100 + C * 10 + D);
                    if ( FFTConfiguration::currentSensor == FFTConfiguration::lsmSensor)
                    {
                        iuAccelerometer.setSamplingRate(samplingRate);
                    }
                    else
                    {
                        iuAccelerometerKX222.setSamplingRate(samplingRate);
                    }
                }
                break;
            case UsageMode::OPERATION:  
                if (strcmp(buff, "IUCAL_START") == 0) {
                    iuUSB.port->println(START_CONFIRM);
                    changeUsageMode(UsageMode::CALIBRATION);
                }
                if (strcmp(buff, "IUCMD_START") == 0) {
                    iuUSB.port->println(START_CONFIRM);
                    changeUsageMode(UsageMode::EXPERIMENT);
                }
                if (strcmp(buff, "IUGET_DATA") == 0) {
                  iuUSB.port->write(START_CONFIRM);
                  changeUsageMode(UsageMode::CUSTOM);   // switch to CUSTOM usage mode
                 }
                if(strcmp(buff,"IUGET_TCP_CONFIG") == 0) {
                    debugPrint("CMD RECEIVED Successfully");
                    if(DOSFS.exists("relayAgentConfig.conf")){
                        
                        const char* _workMode;
                        const char* _remoteIP;
                        int _remotePort;
                        JsonObject& config = configureJsonFromFlash("relayAgentConfig.conf",1);      // get the accountID

                        _workMode = config["relayAgentConfig"]["workMode"];
                        _remoteIP = config["relayAgentConfig"]["remoteAddr"];
                        _remotePort = config["relayAgentConfig"]["remotePort"];
                        
                        iuUSB.port->println("--------- DEVICE CONFIGURATIONS -----------");
                        iuUSB.port->print("U2E_WORK_MODE:");iuUSB.port->println(_workMode);
                        iuUSB.port->print("REMOTE_IP:");iuUSB.port->println(_remoteIP); 
                        iuUSB.port->print("REMOTE_PORT:");iuUSB.port->println(_remotePort);
                        iuUSB.port->println("----------------------------------------------------------");
                        
                    }else
                    {   
                        debugPrint("File does not exists !!!");
                    }
                }    

                if (strcmp(buff, "IUGET_DEVICEID") == 0)
                {
                    iuUSB.port->print("DEVICE_ID : ");
                    iuUSB.port->println(m_macAddress);
                }
                if(strcmp(buff, "IUGET_WIFI_MACID") ==0)
                {
                    iuUSB.port->print("WIFI_MACID : ");
                    iuUSB.port->println(iuWiFi.getMacAddress());
                }
                if (strcmp(buff, "IUGET_FIRMWARE_VERSION") == 0)
                {
                    iuUSB.port->print("FIRMWARE_VERSION : ");
                    iuUSB.port->println(FIRMWARE_VERSION);
                }
                if (strcmp(buff, "IUGET_DEVICE_TYPE") == 0)
                {
                    iuUSB.port->print("DEVICE_TYPE : ");
                    iuUSB.port->println(DEVICE_TYPE);
                }
                if (strcmp(buff,"REMOVE_ESP_FILES") == 0)
                {
                    debugPrint("Deleting Files from ESP32");
                    iuWiFi.sendMSPCommand(MSPCommand::DELETE_CERT_FILES);
                    DOSFS.remove("iuconfig/wifi0.conf");
                }
                if (strcmp(buff,"IUGET_WIFI_CONFIG") == 0)
                {
                    if (DOSFS.exists("iuconfig/wifi0.conf")){
                    const char* ssid;
                    const char* authType;
                    const char* staticIP;
                    const char* gatewayIP;
                   
                    JsonObject& config = configureJsonFromFlash("iuconfig/wifi0.conf",1);
                    ssid = config["ssid"];
                    authType = config["auth_type"];
                    staticIP = config["static"];
                    gatewayIP = config["gateway"];


                    iuUSB.port->println("*****WIFI_CONFIG*****");
                    iuUSB.port->print("SSID : ");
                    iuUSB.port->println(ssid);
                    iuUSB.port->print("AUTH_TYPE : ");
                    iuUSB.port->println(authType);
                    iuUSB.port->print("STATIC_IP : ");
                    iuUSB.port->println(staticIP);
                    iuUSB.port->print("GATEWAY_IP : ");
                    iuUSB.port->println(gatewayIP);
                    }else{
                    debugPrint(F("iuconfig/wifi0.conf file does not exists"));
                  }
                }
                if (strcmp(buff,"IUSET_ERASE_INT_FLASH") == 0)
                {
                    if(iuFlash.checkConfig(CONFIG_MQTT_FLASH_ADDRESS)){
                        iuFlash.clearInternalFlash(CONFIG_MQTT_FLASH_ADDRESS);
                        debugPrint(F("MQTT config removed form internal Flash"));
                    }
                    if(iuFlash.checkConfig(CONFIG_HTTP_FLASH_ADDRESS)){
                        iuFlash.clearInternalFlash(CONFIG_HTTP_FLASH_ADDRESS);
                        debugPrint(F("HTTP config removed form internal Flash"));
                    }
                    if(iuFlash.checkConfig(CONFIG_MODBUS_SLAVE_CONFIG_FLASH_ADDRESS)){
                        iuFlash.clearInternalFlash(CONFIG_MODBUS_SLAVE_CONFIG_FLASH_ADDRESS);
                        debugPrint(F("MODBUS Slave config removed form internal Flash"));
                    }
                    if(iuFlash.checkConfig(CONFIG_WIFI_CONFIG_FLASH_ADDRESS)){
                        iuFlash.clearInternalFlash(CONFIG_WIFI_CONFIG_FLASH_ADDRESS);
                        debugPrint(F("WIFI config removed form internal Flash"));
                    }

                }
                /********************FOR DEVELOPMENT PURPOSE ONLY*******************/
                // This command mainly use to avoid flash all 3 frimwares if changes required only in ESP32 firmware. 
                // Increase "loopTimeout" from ino to 5min to avoid stm reboot
                // Hit the command from USB, This keeps ESP32 in Download mode. 
                // After Flashing ESP32 firmware reboot STM32.
                if(strcmp(buff, "FLASH_ESP") == 0){
                    digitalWrite(7,LOW); // IDE1.5_PORT_CHANGE
                    digitalWrite(IUESP8285::ESP32_ENABLE_PIN, LOW); // IDE1.5_PORT_CHANGE
                    
                    delay(100);
                    digitalWrite(IUESP8285::ESP32_ENABLE_PIN, HIGH);
                    while(1){
                        while (Serial1.available()) {
                            Serial.write(Serial1.read() );
                        }
                        while (Serial.available()) {
                            Serial1.write(Serial.read() );
                        }
                    }
                }
                if (strcmp(buff, "IUGET_HTTP_CONFIG") == 0)
                {
                    if (DOSFS.exists("httpConfig.conf")){
                        const char* _httpHost;
                        const char* _httpPort;
                        const char* _httpPath;

                        const char* oem_httpHost;
                        const char* oem_httpPort;
                        const char* oem_httpPath;

                        JsonObject& config = configureJsonFromFlash("httpConfig.conf",1);
                        _httpHost = config["httpConfig"]["host"];
                        _httpPort = config["httpConfig"]["port"];
                        _httpPath = config["httpConfig"]["path"];
                        if(config.containsKey("httpOem")){
                            oem_httpHost = config["httpOem"]["host"];
                            oem_httpPort = config["httpOem"]["port"];
                            oem_httpPath = config["httpOem"]["path"];
                        }

                    iuUSB.port->println("*****HTTP_CONFIG*****");
                    iuUSB.port->print("HTTP_HOST : ");
                    iuUSB.port->println(_httpHost);
                    iuUSB.port->print("HTTP_PORT : ");
                    iuUSB.port->println(_httpPort);
                    iuUSB.port->print("HTTP_PATH : ");
                    iuUSB.port->println(_httpPath);
                    if(config.containsKey("httpOem")){
                        iuUSB.port->println("*****HTTP_OEM_CONFIG*****");
                        iuUSB.port->print("HTTP_HOST : ");
                        iuUSB.port->println(oem_httpHost);
                        iuUSB.port->print("HTTP_PORT : ");
                        iuUSB.port->println(oem_httpPort);
                        iuUSB.port->print("HTTP_PATH : ");
                        iuUSB.port->println(oem_httpPath);
                    }
                    }else{
                        debugPrint(F("httpConfig.conf file does not exists"));
                    }
                }
                if (strcmp(buff, "IUGET_MQTT_CONFIG") == 0)
                {
                    if  (DOSFS.exists("MQTT.conf")){
                    const char* _serverIP;
                    const char* _serverPort;
                    const char* _UserName;
                    const char* _Password;
                    // const char* _tls;

                    JsonObject& config = configureJsonFromFlash("MQTT.conf",1);
                    _serverIP = config["mqtt"]["mqttServerIP"];
                    _serverPort = config["mqtt"]["port"];
                    _UserName = config["mqtt"]["username"];
                    _Password = config["mqtt"]["password"];
                    // _tls = config["mqtt"]["tls"];


                    iuUSB.port->println("*****MQTT_CONFIG*****");
                    iuUSB.port->print("MQTT_SERVER_IP : ");
                    iuUSB.port->println(_serverIP);
                    iuUSB.port->print("MQTT_PORT : ");
                    iuUSB.port->println(_serverPort);
                    iuUSB.port->print("MQTT_USERNAME : ");
                    iuUSB.port->println(_UserName);
                    iuUSB.port->print("MQTT_PASSWORD : ");
                    iuUSB.port->println(_Password);
                    // iuUSB.port->print("MQTT_TLS : ");
                    // iuUSB.port->println(_tls);
                  }else{
                    debugPrint(F("MQTT.conf file does not exists"));
                  }
                  
                }  
                if (strcmp(buff, "IUGET_FFT_CONFIG") == 0) {
                    iuUSB.port->print("FFT: Sampling Rate: ");
                    iuUSB.port->println(FFTConfiguration::currentSamplingRate);
                    iuUSB.port->print("FFT: Block Size: ");
                    iuUSB.port->println(FFTConfiguration::currentBlockSize);
                    if(FFTConfiguration::currentSensor == FFTConfiguration::lsmSensor){
                        iuUSB.port->print("FFT: LSMgRange : ");
                        iuUSB.port->println(FFTConfiguration::currentLSMgRange);
                    }
                    if(FFTConfiguration::currentSensor == FFTConfiguration::kionixSensor){
                        iuUSB.port->print("FFT: KNXgRange : ");
                        iuUSB.port->println(FFTConfiguration::currentKNXgRange);
                    }
                }
                if (strcmp(buff,"IUGET_DEVICE_CONF") == 0)
                {
                    // Read the device.conf file
                    if(DOSFS.exists("/iuconfig/device.conf")){
                        JsonObject& config = configureJsonFromFlash("/iuconfig/device.conf",1);
                        String jsonChar;
                        config.printTo(jsonChar);
                        if (loopDebugMode)
                        {
                            debugPrint("Data From : device.conf ",true);
                        }
                        //debugPrint(jsonChar);
                        iuUSB.port->println(jsonChar);
                    }else
                    {
                        debugPrint(F("device.conf file does not exists."));
                    }
                    
                }
                if (strcmp(buff, "IUSET_ERASE_EXT_FLASH") == 0)
                {
                    debugPrint("Formating Flash Please wait");
                    ledManager.overrideColor(RGB_RED);
                    DOSFS.format();
                    ledManager.overrideColor(RGB_WHITE);
                    DOSFS.end();
                    debugPrint("Format Successfully");
                    delay(2000);
                    STM32.reset();
                }
                if (strcmp(buff, "IUGET_OTAFLAG_VALUE") == 0)
                {
                    iuUSB.port->println("OTA FLAG VALUES:");
                    iuOta.readOtaFlag();
                    iuUSB.port->print("[0-OTA_STATUS]: ");
                    iuUSB.port->println(iuOta.getOtaFlagValue(OTA_STATUS_FLAG_LOC));
                    iuUSB.port->print("[1-OTA_RETRY]: ");
                    iuUSB.port->println(iuOta.getOtaFlagValue(OTA_RETRY_FLAG_LOC));
                    iuUSB.port->print("[2-OTA_VALIDITY_RETRY]: ");
                    iuUSB.port->println(iuOta.getOtaFlagValue(OTA_VLDN_RETRY_FLAG_LOC));
                    iuUSB.port->print("[3-OTA_PEND_MSG_STS]: ");
                    iuUSB.port->println(iuOta.getOtaFlagValue(OTA_PEND_STATUS_MSG_LOC));
                }
                if (strcmp(buff, "IUGET_CERT_CONFIG") == 0) {
                    iuWiFi.sendMSPCommand(MSPCommand::GET_CERT_CONFIG);
                }
                if (strcmp(buff, "IUGET_DIG_CONFIG") == 0) {
                    JsonObject &diag =  configureJsonFromFlash("/iuRule/diagnostic.conf",1);
                    if(diag.success()){
                        iuUSB.port->println("*****DIG CONFIG*****");
                        diag.printTo(*iuUSB.port);
                        iuUSB.port->println("");
                    }
                }
                if (strcmp(buff, "IUGET_PHASE_CONFIG") == 0) {
                    JsonObject &phase =  configureJsonFromFlash("/iuconfig/phase.conf",1);
                    if(phase.success()){
                        iuUSB.port->println("*****PHASE CONFIG*****");
                        phase.printTo(*iuUSB.port);
                        iuUSB.port->println("");
                    }
                }
                if (strcmp(buff, "IUGET_RPM_CONFIG") == 0) {
                    JsonObject &rpm =  configureJsonFromFlash("/iuconfig/rpm.conf",1);
                    if(rpm.success()){
                        iuUSB.port->println("*****RPM CONFIG*****");
                        rpm.printTo(*iuUSB.port);
                        iuUSB.port->println("");
                    }
                }
                if (strcmp(buff, "IUSET_OTAFLAG_00") == 0)
                {
                    uint32_t cmdStrTime = millis();
                    uint8_t value = 0xFF;
                    while(iuUSB.port->available() > 0)
                    {
                        value = iuUSB.port->parseInt();
                        iuUSB.port->print("Value:");
                        iuUSB.port->println(value);
                        if((millis() - cmdStrTime) > 7000)
                            value = 0xFF;
                        if(value >= 0)
                            break;
                    }
                    if(value >= 0 && value < 10 )
                        iuOta.updateOtaFlag(OTA_STATUS_FLAG_LOC,value);
                    delay(100);
                }
               if (strcmp(buff, "IUSET_OTAFLAG_01") == 0)
                {
                    uint32_t cmdStrTime = millis();
                    uint8_t value = 0xFF;
                    while(iuUSB.port->available() > 0)
                    {
                        value = iuUSB.port->parseInt();
                        iuUSB.port->print("Value:");
                        iuUSB.port->println(value);
                        if((millis() - cmdStrTime) > 7000)
                            value = 0xFF;
                        if(value >= 0)
                            break;
                    }
                    if(value >= 0 && value < 4 )
                        iuOta.updateOtaFlag(OTA_RETRY_FLAG_LOC,value);
                    delay(100);
                }
               if (strcmp(buff, "IUSET_OTAFLAG_02") == 0)
                {
                    uint32_t cmdStrTime = millis();
                    uint8_t value = 0xFF;
                    while(iuUSB.port->available() > 0)
                    {
                        value = iuUSB.port->parseInt();
                        iuUSB.port->print("Value:");
                        iuUSB.port->println(value);
                        if((millis() - cmdStrTime) > 7000)
                            value = 0xFF;
                        if(value >= 0)
                            break;
                    }
                    if(value >= 0 && value < 4 )
                        iuOta.updateOtaFlag(OTA_VLDN_RETRY_FLAG_LOC,value);
                    delay(100);
                }
                if (strcmp(buff, "IUSET_OTAFLAG_03") == 0)
                {
                    uint32_t cmdStrTime = millis();
                    uint8_t value = 0xFF;
                    while(iuUSB.port->available() > 0)
                    {
                        value = iuUSB.port->parseInt();
                        iuUSB.port->print("Value:");
                        iuUSB.port->println(value);
                        if((millis() - cmdStrTime) > 7000)
                            value = 0xFF;
                        if(value >= 0)
                            break;
                    }
                    if(value >= 0 && value <= 4 )
                        iuOta.updateOtaFlag(OTA_PEND_STATUS_MSG_LOC,value);
                    delay(100);
                }
                if (strcmp(buff, "IUGET_WIFI_TXPWR") == 0) {
                    iuWiFi.sendMSPCommand(MSPCommand::WIFI_GET_TX_POWER);
                    
                }
                // Set the WiFI TX Power
                if(buff[0] == '4' && buff[1]=='0' && buff[2] == '0' && buff[3]== '-')   // 400-[0-9]
                {    //iuWiFi.sendMSPCommand(MSPCommand::WIFI_SET_TX_POWER);
                    uint8_t mode = processWiFiRadioModes(buff);
                    if(loopDebugMode){
                        debugPrint("SET MODE :",false);
                        debugPrint(mode);
                    }
                }
                if (strcmp(buff,"IUGET_MODBUS_CONFIG") == 0 ){
                    if(DOSFS.exists("/iuconfig/modbusSlave.conf")){
                        JsonObject& config = configureJsonFromFlash("/iuconfig/modbusSlave.conf",1);
                        String jsonChar;
                        config.printTo(jsonChar);
                        if (loopDebugMode)
                        {
                            debugPrint("Data From : modbusSlave.conf ",true);
                        }
                        //debugPrint(jsonChar);
                        iuUSB.port->println(jsonChar);
                    }else
                    {
                        debugPrint(F("modbusSlave.conf file does not exists."));
                    }    
                
                }
                    
                break;
                
            case UsageMode::CUSTOM:
                if (strcmp(buff, "IUEND_DATA") == 0) {
                    iuUSB.port->println(END_CONFIRM);
                    changeUsageMode(UsageMode::OPERATION);    //back to Operation Mode
                   return; 
                }  
                result = strstr(buff, "Arange");
                                if (result != NULL) {
                    
                    if ( FFTConfiguration::currentSensor == FFTConfiguration::lsmSensor)
                    {
                        switch (result[7] - '0') {
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
                    }
                    else
                    {
                        switch (result[7] - '0') {
                            case 0:
                                iuAccelerometerKX222.setScale(iuAccelerometerKX222.FSR_8G);
                                break;
                            case 1:
                                iuAccelerometerKX222.setScale(iuAccelerometerKX222.FSR_16G);
                                break;
                            case 2:
                                iuAccelerometerKX222.setScale(iuAccelerometerKX222.FSR_32G);
                                break;
                        }
                    }
                    return;
                }
                result = strstr(buff, "rgb");
                if (result != NULL) {
                    ledManager.overrideColor(RGBColor(255 * (result[7] - '0'),
                                                      255 * (result[8] - '0'),
                                                      255 * (result[9] - '0')));
                    return;
                }
                result = strstr(buff, "acosr");
                if (result != NULL) {
                    // Change audio sampling rate
                    int A = result[6] - '0';
                    int B = result[7] - '0';
                    uint16_t samplingRate = (uint16_t) ((A * 10 + B) * 1000);
                    iuI2S.setSamplingRate(samplingRate);
                    return;
                }
                result = strstr(buff, "accsr");
                if (result != NULL) {
                    int A = result[6] - '0';
                    int B = result[7] - '0';
                    int C = result[8] - '0';
                    int D = result[9] - '0';
                    int samplingRate = (A * 1000 + B * 100 + C * 10 + D);
                    if ( FFTConfiguration::currentSensor == FFTConfiguration::lsmSensor)
                    {
                        iuAccelerometer.setSamplingRate(samplingRate);
                    }
                    else
                    {
                        iuAccelerometerKX222.setSamplingRate(samplingRate);
                    }
                }
                break;
            case UsageMode::OTA:
                if (loopDebugMode) {
                    debugPrint("Usage Mode: OTA");
                }
                break;
            default:
                if (loopDebugMode) {
                    debugPrint(F("Unhandled usage mode: "), false);
                    debugPrint(m_usageMode);
                }
        }
    }
 }


void Conductor::processJSONmessage(const char * buff) 
{
    if (buff[0] == 'm' || buff[0] == 'M') 
    {
        debugPrint("JSON type: ", false); debugPrint(buff[0]);
        int message_id = int(buff[1]);
        debugPrint("JSON message ID: ", false); debugPrint(message_id);
        int segment_index = int(buff[2]);
        debugPrint("JSON segment index / segment count: ", false); debugPrint(segment_index);
        debugPrint("JSON segment data/control : ", false); 
        bool done = false;
        int i = 3;
        while (true) {
            if (done) { break;  }
            debugPrint(char(buff[i]), false);
            if(char(buff[i]) == ';') { done = true; }
            i++;
        }
        debugPrint("", true);
    }
}

/**
 * Process the instructions sent over Bluetooth
 */
void Conductor::processBLEMessage(IUSerial *iuSerial)
{
    m_lastBLEmessage = millis();
    char *buff = iuSerial->getBuffer();
    // debugPrint("DEBUG: BLE BUFFER: ", false); debugPrint(buff, true);

    if (buff[0] == 'm' || buff[0] == 'M')  { // && (int(buff[1]) < MAX_SEGMENTED_MESSAGES)) {
        // int(buff[1]) -> messageID
        char segmentedMessageBuffer[21];
        for (int i=0; i<20; i++) { segmentedMessageBuffer[i] = buff[i]; }
        #ifdef IU_DEBUG_SEGMENTED_MESSAGES
        debugPrint("DEBUG: Conductor::processBLEMessage() calling processSegmentedMessage()");
        #endif
        processSegmentedMessage(segmentedMessageBuffer);
        iuBluetooth.resetBuffer();  // consider the BLE received buffer to be flushed
        #ifdef IU_DEBUG_SEGMENTED_MESSAGES
        debugPrint("DEBUG: Back in Conductor::processBLEMessage() after processSegmentedMessage()");
        #endif
    }
    if (m_streamingMode == StreamingMode::WIRED) {
        return;  // Do not listen to BLE when wired
    }
    if (buff[0] == '{') {
        processConfiguration(buff, true);
    } else if (strncmp(buff, "WIFI-", 5) == 0) {
        processUserCommandForWiFi(buff, &iuBluetooth);
    } else if (strncmp(buff, "BLE-RESET", 9) == 0) {
        debugPrint("RECEIVED BLE-RESET");
        //  this is the first condition that will be tested to check if bluetooth is connected in updateStreamingMode() -> isBLEConnected(); 
        // we make this 0 here to indicate that bluetooth is disconnected
        m_lastBLEmessage = 0;
        // debugPrint("BLE RESET");
        iuBluetooth.softReset();     
    } else {
        processCommand(buff);
        processLegacyCommand(buff);
    }
    updateStreamingMode();
}

/**
 * Process the instructions from the WiFi chip
 * This is used in case of ETHERNET StreamingMode
 */
void Conductor::processWiFiMessage(IUSerial *iuSerial)
{
    char *buff = iuSerial->getBuffer();
    uint16_t packetLen =  iuSerial->mspGetDataSize();
    if(getUsageMode() != UsageMode::OTA) {
        if (buff[0] == '{')
        {
            processConfiguration(buff,true);    //save the configuration into the file
            return;       // this functon should process only one command in one call, all if conditions should be mutually exclusive
        }
        if (iuWiFi.processChipMessage()) {
            if (iuWiFi.isWorking()) {
                ledManager.showStatus(&STATUS_WIFI_WORKING);
            } else {
                ledManager.resetStatus();
            }
            updateStreamingMode();
        }
        uint32_t currentTime = millis();
        if(buff[0] == 'i' && buff[3] == '_' && buff[8] =='/' && buff[18] == '-'){ // ide_plus/time_sync-timestamp
        
        if(iuEthernet.isEthernetConnected == 1) { // check if ethernet is not connected
            iuEthernet.isEthernetConnected = 0;  // set to connected, when timesync message is received
            ledManager.showStatus(&STATUS_WIFI_CONNECTED);  
        }
            setRefDatetime(&buff[19]);
            lastTimeSync = currentTime;
        }
        if((buff[0] == '3' && buff[7] == '0' && buff[9] == '0' && buff[11] == '0' &&
                    buff[13] == '0' && buff[15] == '0' && buff[17] == '0') ){
            processCommand(buff);

        }else
        {
            processLegacyCommand(buff);
        }
    }
    uint8_t idx = 0;
 //   char otaResponse[256];
 //   double otaInitTimeStamp;
    switch (iuWiFi.getMspCommand()) {
        case MSPCommand::ESP_DEBUG_TO_STM_HOST:
            if (loopDebugMode) {
                debugPrint(buff);
            }
            //  Serial.print("Debug from ESP : ");
            // Serial.write(buff);
            // Serial.println();
            break;
        case MSPCommand::OTA_INIT_REQUEST:
            if(doOnceFWValid == true)
            { // Don't accept new OTA request during Validation of Last OTA
                if(loopDebugMode) {
                    debugPrint(F("Last OTA in Progress.. Unable to process OTA Request"));
                }
            }
            else
            {
                if (loopDebugMode) {
                    debugPrint(F("OTA Init Request Received"));
                    debugPrint(F("Switching Device mode:OPERATION -> OTA"));
                }   
                changeUsageMode(UsageMode::OTA);
                delay(1);
                /* OTA Get MQTT message request timer. In case of timeout switch back to OPERATION MODE */
                otaInitWaitTimeout = millis();
                otaInitTimeoutFlag = true;
            }          
            delay(1);            
            break;
        case MSPCommand::OTA_STM_DNLD_STATUS:
            if (loopDebugMode) { debugPrint(F("STM FW Download Completed !")); }
            strcpy(fwBinFileName, vEdge_Wifi_FW_BIN);
            iuOta.otaFileRemove(iuFlash.IUFWTMPIMG_SUBDIR,vEdge_Wifi_FW_BIN);
            iuOta.otaFileRemove(iuFlash.IUFWTMPIMG_SUBDIR,vEdge_Wifi_FW_MD5);
            iuOta.otaMD5Write(iuFlash.IUFWTMPIMG_SUBDIR,vEdge_Wifi_FW_MD5,espHash);
            iuWiFi.sendMSPCommand(MSPCommand::OTA_STM_DNLD_OK);
            otaFwdnldTmout = millis();
            delay(1);
  //          waitingDnldStrart = false; 
            break;
        case MSPCommand::OTA_DNLD_FAIL:
            if (loopDebugMode) {
                debugPrint(F("OTA FW Download Failed !"));
                debugPrint(buff);
            } 
            delay(1);
            otaFwdnldTmout = millis();
            waitingDnldStrart = false;
            sendOtaStatusMsg(MSPCommand::OTA_FDW_ABORT,OTA_DOWNLOAD_ERR,buff);
            for(int i = 0 ; i < 15; i++) {
                ledManager.overrideColor(RGB_RED);
                delay(200);
                ledManager.stopColorOverride();
                delay(200);
            }
            if (loopDebugMode) { debugPrint(F("Switching Device mode:OTA -> OPERATION")); }
            iuWiFi.m_setLastConfirmedPublication();
            changeUsageMode(UsageMode::OPERATION);
            delay(100); 
            break;
        case MSPCommand::OTA_ESP_DNLD_STATUS:
            if (loopDebugMode) { debugPrint(F("ESP FW Download Completed !")); }
            iuWiFi.sendMSPCommand(MSPCommand::OTA_ESP_DNLD_OK);
            otaFwdnldTmout = millis();
            waitingDnldStrart = false;
            delay(1);
            if (loopDebugMode) { debugPrint(F("STM,ESP FW Download Completed")); }
            char hashMD5[34];
            bool hashCheck;
            hashCheck = false;
            memset(hashMD5,'\0', 34);
            iuOta.otaGetMD5(iuFlash.IUFWTMPIMG_SUBDIR,vEdge_Main_FW_BIN,hashMD5);
            if (loopDebugMode) {
                debugPrint(F("Main FW hash received:"),false);
                debugPrint(stmHash);
                debugPrint(F("Main FW hash computed:"),false);
                debugPrint(hashMD5);
            }
            if(!(strcmp(hashMD5,stmHash)))
            {
                memset(hashMD5,'\0', 34);
                iuOta.otaGetMD5(iuFlash.IUFWTMPIMG_SUBDIR,vEdge_Wifi_FW_BIN,hashMD5);
                if (loopDebugMode) {                    
                    debugPrint(F("WiFi FW hash received:"),false);
                    debugPrint(espHash);
                    debugPrint(F("WiFi FW hash computed:"),false);
                    debugPrint(hashMD5);
                }
                if(!(strcmp(hashMD5,espHash)))
                {
                    if (loopDebugMode) { debugPrint(F("Main FW,WiFi FW Hash match Ok")); }
                    hashCheck = true;
                }
                else
                {
                    if (loopDebugMode) { debugPrint(F("WiFi FW Hash match Fail !")); }
                    hashCheck = false;
                }
            }
            else
            {
                if (loopDebugMode) { debugPrint(F("Main FW Hash match Fail !")); }
                hashCheck = false;
            } 
            if(hashCheck == true) {
                if (loopDebugMode) { debugPrint(F("OTA File Write Success, Sending OTA-FDW-SUCCESS")); }
                ledManager.overrideColor(RGB_PURPLE);
                sendOtaStatusMsg(MSPCommand::OTA_FDW_SUCCESS,OTA_DOWNLOAD_OK,OTA_RESPONE_OK);
                doOnceFWValid = false;
                FW_Valid_State = 0;
                iuOta.updateOtaFlag(OTA_STATUS_FLAG_LOC,OTA_FW_DOWNLOAD_SUCCESS);
                iuOta.updateOtaFlag(OTA_VLDN_RETRY_FLAG_LOC,0);
                delay(1000);
                if (loopDebugMode) { debugPrint(F("OTA FW hash Success, Sending OTA-FUG-START")); }
                sendOtaStatusMsg(MSPCommand::OTA_FUG_START,OTA_UPGRADE_START,OTA_RESPONE_OK);
                delay(1000);
                if (loopDebugMode) { debugPrint(F("Rebooting device for FW Upgrade......")); }
                delay(500);
                DOSFS.end();
                delay(10);
                STM32.reset();
            }
            else
            {
                if (loopDebugMode) { debugPrint(F("OTA File Write Failed, Sending OTA-ERR-FDW-ABORT")); }
                waitingDnldStrart = false;
                certDownloadInProgress = false;
                sendOtaStatusMsg(MSPCommand::OTA_FDW_ABORT,OTA_DOWNLOAD_ERR, String(iuOta.getOtaRca(OTA_CHECKSUM_FAIL)).c_str());
                for(int i = 0 ; i < 15; i++) {
                    ledManager.overrideColor(RGB_RED);
                    delay(200);
                    ledManager.stopColorOverride();
                    delay(200);
                }
            }
            delay(1000);
            if (loopDebugMode) { debugPrint(F("Switching Device mode:OTA -> OPERATION")); }
            iuWiFi.m_setLastConfirmedPublication();
            changeUsageMode(UsageMode::OPERATION);
            delay(100);
            break;
        case MSPCommand::OTA_PACKET_DATA:
            if (loopDebugMode) {
            //    debugPrint(F("OTA_PACKET_DATA Len:"),false);
            //    debugPrint(packetLen);
            }
//            waitingDnldStrart = false;
            otaFwdnldTmout = millis();
            if(iuOta.otaFwBinWrite(iuFlash.IUFWTMPIMG_SUBDIR,fwBinFileName, buff, packetLen)) {
                if (loopDebugMode) { debugPrint(F("Sending OTA_PACKET_ACK")); }
                iuWiFi.sendMSPCommand(MSPCommand::OTA_PACKET_ACK);
            }
            else
            { // Fw File write failed... Abort OTA
                if (loopDebugMode) {
                    debugPrint(F("OTA File Write Failed, Sending OTA-ERR-FDW-ABORT"));
                }
                waitingDnldStrart = false;
                sendOtaStatusMsg(MSPCommand::OTA_FDW_ABORT,OTA_DOWNLOAD_ERR, String(iuOta.getOtaRca(OTA_FLASH_RDWR_FAIL)).c_str());
                for(int i = 0 ; i < 15; i++) {
                    ledManager.overrideColor(RGB_RED);
                    delay(200);
                    ledManager.stopColorOverride();
                    delay(200);
                }
                if (loopDebugMode) { debugPrint(F("Switching Device mode:OTA -> OPERATION")); }
                iuWiFi.m_setLastConfirmedPublication();
                changeUsageMode(UsageMode::OPERATION);
                delay(100);         
            }
            break;
        //Certificate management MSP Command
        
        case MSPCommand::GET_CERT_COMMOM_URL:
             iuWiFi.sendMSPCommand(MSPCommand::SET_CERT_CONFIG_URL);
            break;
        case MSPCommand::ESP32_FLASH_FILE_WRITE_FAILED:
            if (loopDebugMode)
            {
                debugPrint("ESP32 File Write Failed");
            }
            break;
        case MSPCommand::ESP32_FLASH_FILE_READ_FAILED:
            if (loopDebugMode)
            {
                debugPrint("ESP32 File Read Failed");
            }
            break;
        case MSPCommand::CERT_DOWNLOAD_INIT:
            certDownloadMode = true;
            if(certDownloadInProgress == true)
            { // Don't accept new OTA request during Validation of Last OTA
                if(loopDebugMode) {
                    debugPrint(F("Last Cert Download In-Progress.. Unable to process Certificates Download Init Request"));
                }
            }
            else
            {
                if (loopDebugMode) {
                    debugPrint(F("Certificate Download Init Request Received"));
                    debugPrint(F("Switching Device mode:OPERATION -> OTA/Certificate Download"));
                    debugPrint(buff);
                }
                certDownloadInProgress = true; 
                m_getDownloadConfig = false;
                m_certDownloadStarted = false;
                strcpy(m_otaMsgId,buff);
                certDownloadInitWaitTimeout = millis();  
                changeUsageMode(UsageMode::OTA);
                delay(1);
                /* OTA Get MQTT message request timer. In case of timeout switch back to OPERATION MODE */
                //otaInitWaitTimeout = millis();
                //otaInitTimeoutFlag = true;
                //iuWiFi.sendMSPCommand(MSPCommand::CERT_DOWNLOAD_INIT_ACK); // send init Ack
                sendOtaStatusMsg(MSPCommand::CERT_DOWNLOAD_INIT_ACK,CERT_DOWNLOAD_ACK, String(iuOta.getOtaRca(CERT_DOWNLOAD_INIT_REQ_ACK)).c_str());
                
            }          
            break;
        case MSPCommand::GET_CERT_DOWNLOAD_CONFIG:
            if (debugMode)
            {
                debugPrint("Received the Command to get the cert config json ");
            }
            strcpy(m_otaMsgId,buff);
            m_getDownloadConfig = true;
            m_certDownloadStarted = false;
            certDownloadConfigTimeout = millis();
            iuWiFi.sendMSPCommand(MSPCommand::GET_CERT_DOWNLOAD_CONFIG);
            break;    
        case MSPCommand::DOWNLOAD_TLS_SSL_START:
            if (debugMode)
            {
                debugPrint("Downloading of TLS SSL EAP cert Initiated");
            }
            m_certDownloadStarted = true;
            otaFwdnldTmout = millis();
            waitingDnldStrart = false;
            otaInitTimeoutFlag = false;
            certDownloadInProgress = false;
            m_getDownloadConfig = false;
            sendOtaStatusMsg(MSPCommand::DOWNLOAD_TLS_SSL_START,CERT_DOWNLOAD_STARTED, String(iuOta.getOtaRca(CERT_DOWNLOAD_START)).c_str());
            delay(10);
            // Certificates Download start Visuals
            ledManager.stopColorOverride();
            ledManager.showStatus(&STATUS_OTA_DOWNLOAD); 
            break;
        case MSPCommand::CERT_DOWNLOAD_ABORT:
             // TODO : add Visuals
            
            if (loopDebugMode) {
                debugPrint(F("Certificate File Download Aborted"));
                debugPrint(buff);
            } 
            delay(1);
            otaFwdnldTmout = millis();
            waitingDnldStrart = false;
            certDownloadInProgress = false;
            m_certDownloadStarted = false;
            m_getDownloadConfig = false;
            m_downloadSuccess = false;
            m_upgradeSuccess = false;
            sendCertInitAck = false;
            ledManager.overrideColor(RGB_ORANGE);
            sendOtaStatusMsg(MSPCommand::CERT_DOWNLOAD_ABORT,CERT_DOWNLOAD_ERR,buff);
            delay(1000);
            for(int i = 0 ; i < 20; i++) {
                ledManager.overrideColor(RGB_RED);
                delay(200);
                ledManager.stopColorOverride();
                delay(200);
            }
            if (loopDebugMode) { debugPrint(F("Switching Device mode:CERT DOWNLOAD Mode -> OPERATION")); }
            iuWiFi.m_setLastConfirmedPublication();
            changeUsageMode(UsageMode::OPERATION);
            delay(100); 
            
            break;
        case MSPCommand::CERT_DOWNLOAD_SUCCESS:
             if(loopDebugMode){
                debugPrint("Certificate Download Success");
                debugPrint(buff);
            }
            otaInitTimeoutFlag = false;
            waitingDnldStrart = false;
            certDownloadInProgress = false;
            m_certDownloadStarted = false;
            m_getDownloadConfig = false;
            m_downloadSuccess = true;
            m_upgradeSuccess = false;
            m_downloadSuccessStartTime = millis();
            ledManager.overrideColor(RGB_BLUE); 
            delay(1000);
            //  TODO : Add Visuals
            for(int i = 0 ; i < 10; i++) {
                ledManager.overrideColor(RGB_GREEN);
                delay(100);
                ledManager.stopColorOverride();
                delay(100);
            }            
            sendOtaStatusMsg(MSPCommand::CERT_DOWNLOAD_SUCCESS,CERT_DOWNLOAD_COMPLETE,buff);
            // if (loopDebugMode) { debugPrint(F("Switching Device mode:CERT DOWNLOAD Mode -> OPERATION")); }
            //  iuWiFi.m_setLastConfirmedPublication();
            //  changeUsageMode(UsageMode::OPERATION);
            
            break;
        case MSPCommand::CERT_UPGRADE_INIT:
             if(loopDebugMode){
                 debugPrint("Certificates Upgrade Init");
             }
             sendOtaStatusMsg(MSPCommand::CERT_DOWNLOAD_INIT,CERT_UPGRADE_STARTED,buff);
            break;
        case MSPCommand::CERT_UPGRADE_ABORT:
             sendOtaStatusMsg(MSPCommand::CERT_UPGRADE_ABORT,CERT_UPGRADE_ERR,buff);
            if (loopDebugMode) { 
                debugPrint("Certificates Upgrade Failed");
                debugPrint(F("Switching Device mode:CERT DOWNLOAD -> OPERATION")); }
                iuWiFi.m_setLastConfirmedPublication();
                changeUsageMode(UsageMode::OPERATION);
                delay(100);
                certDownloadInProgress = false;
                m_certDownloadStarted = false;
                m_getDownloadConfig = false;
                m_downloadSuccess = false;
                m_upgradeSuccess = false;
            ledManager.overrideColor(RGB_ORANGE);
            delay(1000);
            for(int i = 0 ; i < 20; i++) {
                ledManager.overrideColor(RGB_RED);
                delay(200);
                ledManager.stopColorOverride();
                delay(200);
            }
            iuWiFi.hardReset();
            break;
        case MSPCommand::CERT_UPGRADE_SUCCESS:
            if (loopDebugMode)
             {
                 debugPrint("Certificates Upgrade Success");
             }

            ledManager.overrideColor(RGB_PURPLE);
             sendOtaStatusMsg(MSPCommand::CERT_UPGRADE_SUCCESS,CERT_UPGRADE_COMPLETE,buff);
             certDownloadInProgress = false;
             m_certDownloadStarted = false;
             m_getDownloadConfig = false;
             m_downloadSuccess = false;
             m_upgradeSuccess = true;
             delay(1000);
            // TODO : Add Visuals
             // Show Visuals min 15 sec
            for(int i = 0 ; i < 20; i++) {
                ledManager.overrideColor(RGB_GREEN);
                delay(200);
                ledManager.stopColorOverride();
                delay(200);
            }
            if (loopDebugMode) { debugPrint(F("Switching Device mode:CERT DOWNLOAD Mode -> OPERATION")); }
             iuWiFi.m_setLastConfirmedPublication();
             changeUsageMode(UsageMode::OPERATION);
             delay(100);
             iuWiFi.hardReset();
             break;
        case MSPCommand::CERT_INVALID_STATIC_URL:
            if (loopDebugMode)
            {
                debugPrint("Received Invalid Common HTTP Endpoint");
            }
            break;
        case MSPCommand::DELETE_CERT_FILES:
            if (debugMode)
            {
                debugPrint("message:",false);
                debugPrint(buff);
            }
            break;
        case MSPCommand::SEND_CERT_DWL_CFG:
            iuUSB.port->println(buff);
            break;
        case MSPCommand::SEND_CERT_DIG_CFG:
            iuUSB.port->println(buff);
            break;
        case MSPCommand::CERT_NO_DOWNLOAD_AVAILABLE:
            if (debugMode)
            {
                debugPrint("New Certificate Download Request not available");
            }
                       
            sendOtaStatusMsg(MSPCommand::CERT_UPGRADE_ABORT,CERT_UPGRADE_ERR,buff);
            if (loopDebugMode) { debugPrint(F("Switching Device mode:CERT DOWNLOAD Mode -> OPERATION")); }
            iuWiFi.m_setLastConfirmedPublication();
            changeUsageMode(UsageMode::OPERATION);
            delay(100);
            break;
        case MSPCommand::ALL_MQTT_CONNECT_ATTEMPT_FAILED:
            certDownloadInProgress = false;
            m_certDownloadStarted = false;
            m_getDownloadConfig = false;
            m_downloadSuccess = false;
            m_upgradeSuccess = false;
            certDownloadInitWaitTimeout = 0;
            certDownloadConfigTimeout = 0;
            if(debugMode){
                debugPrint("All Secure MQTT Connection Attempt Failed,Can't connect to broker, re-configure the mqtt details ");
            }
            sendOtaStatusMsg(MSPCommand::CERT_UPGRADE_ABORT,CERT_UPGRADE_ERR,buff);
            // Show Visuals min 15 sec
            for(int i = 0 ; i < 40; i++) {
                ledManager.overrideColor(RGB_CYAN);
                delay(200);
                ledManager.stopColorOverride();
                delay(200);
            }
            if (loopDebugMode) { debugPrint(F("Switching Device mode:CERT DOWNLOAD Mode -> OPERATION")); }
            iuWiFi.m_setLastConfirmedPublication();
            changeUsageMode(UsageMode::OPERATION);
            
            break;
        case MSPCommand::SET_CERT_DOWNLOAD_MSGID:
            if(loopDebugMode){ debugPrint("Received Certificate Download MessageId");}
            strcpy(m_otaMsgId,buff);
            debugPrint("CERT MESG ID : ",false);
            debugPrint(m_otaMsgId);
            break;
        // MSP Status messages
        case MSPCommand::MSP_INVALID_CHECKSUM:
            if (loopDebugMode) { debugPrint(F("MSP_INVALID_CHECKSUM")); }
            break;
        case MSPCommand::MSP_TOO_LONG:
            if (loopDebugMode) { debugPrint(F("MSP_TOO_LONG")); }
            break;
        case MSPCommand::RECEIVE_WIFI_FV:{
            if (loopDebugMode) { debugPrint(F("RECEIVE_WIFI_FV")); }
            strncpy(iuWiFi.espFirmwareVersion, buff, 6);
            byte firstDigit = atoi(&buff[0]); 
            byte secondDigit = atoi(&buff[2]);
            byte thirdDigit = atoi(&buff[4]);
            iuModbusSlave.WIFI_FIRMWARE_VERSION = (firstDigit*100) + (secondDigit*10) + thirdDigit; 
            iuWiFi.espFirmwareVersionReceived = true;
            debugPrint("WIFI VERSION : ",false);debugPrint(iuModbusSlave.WIFI_FIRMWARE_VERSION );
        
            }
        break;
        case MSPCommand::ASK_BLE_MAC:
            if (loopDebugMode) { debugPrint(F("ASK_BLE_MAC")); }
            iuWiFi.sendBleMacAddress(m_macAddress);
            break;
	      case MSPCommand::ASK_HOST_FIRMWARE_VERSION:
            if (loopDebugMode){ debugPrint(F("ASK_HOST_FIRMWARE_VERSION")); }
            iuWiFi.sendHostFirmwareVersion(FIRMWARE_VERSION);
            break;
    	case MSPCommand::GET_DEVICE_CONFIG:
            {
            if (loopDebugMode){ debugPrint(F("GET_DEVICE_CONFIG")); }
            char deviceInfo[64];
            //sprintf(deviceInfo,"%s-%d-%d",FIRMWARE_VERSION,FFTConfiguration::currentSamplingRate,FFTConfiguration::currentBlockSize);
            sprintf(deviceInfo,"%s-%d-%d-%d",FIRMWARE_VERSION,FFTConfiguration::calculatedSamplingRate,FFTConfiguration::currentBlockSize,httpOEMConfigPresent);
            iuWiFi.sendMSPCommand(MSPCommand::GET_DEVICE_CONFIG,deviceInfo);
            iuWiFi.sendMSPCommand(MSPCommand::RECEIVE_HOST_FIRMWARE_VERSION,FIRMWARE_VERSION);
            
            byte firstDigit = atoi(&FIRMWARE_VERSION[0]); 
            byte secondDigit = atoi(&FIRMWARE_VERSION[2]);
            byte thirdDigit = atoi(&FIRMWARE_VERSION[4]);
            iuModbusSlave.STM_FIRMWARE_VERSION = (firstDigit*100) + (secondDigit*10) + thirdDigit;
            debugPrint("STM VERSION :",false);debugPrint(iuModbusSlave.STM_FIRMWARE_VERSION);               
            }
            break;
        case MSPCommand::ASK_HOST_SAMPLING_RATE:        
            if (loopDebugMode){ debugPrint(F("ASK_HOST_SAMPLING_RATE")); }
            iuWiFi.sendHostSamplingRate(FFTConfiguration::currentSamplingRate);    
            break;
        case MSPCommand::ASK_HOST_BLOCK_SIZE:
            if (loopDebugMode){ debugPrint(F("ASK_HOST_BLOCK_SIZE")); }
            iuWiFi.sendHostBlockSize(FFTConfiguration::currentBlockSize);
            break;
        case MSPCommand::HTTPS_ACK: {
            if (loopDebugMode){ debugPrint(F("HTTPS_ACK")); }
            if (buff[0] == 'X') {
                httpsStatusCodeX = atoi(&buff[1]);
            } else if (buff[0] == 'Y') {
                httpsStatusCodeY = atoi(&buff[1]);
            } else if (buff[0] == 'Z') {
                httpsStatusCodeZ = atoi(&buff[1]);
            }
            if (loopDebugMode) {
                debugPrint("HTTPS status codes X | Y | Z ",false);
                debugPrint(httpsStatusCodeX, false);debugPrint(" | ", false);
                debugPrint(httpsStatusCodeY, false);debugPrint(" | ", false);
                debugPrint(httpsStatusCodeZ, true);
            }
#if 1
            if(httpsStatusCodeX != 200 || (httpsStatusCodeX == 200 && httpsStatusCodeY != 200 && httpsStatusCodeY != 0) || (httpsStatusCodeX == 200 && httpsStatusCodeY == 200 && httpsStatusCodeZ != 200 && httpsStatusCodeZ != 0))
            {
                debugPrint("HTTPS status codes X | Y | Z ",false);
                debugPrint(httpsStatusCodeX, false);debugPrint(" | ", false);
                debugPrint(httpsStatusCodeY, false);debugPrint(" | ", false);
                debugPrint(httpsStatusCodeZ, true);
                // Abort transmission on failure  // IDE1.5_PORT_CHANGE Change
                // RawDataState::rawDataTransmissionInProgress = false;
                // RawDataState::startRawDataCollection = false;             
            }
#endif
            break;
        }
        case MSPCommand::HTTPS_OEM_ACK: {
            if (loopDebugMode){ debugPrint(F("HTTPS_OEM_ACK")); }
            if (buff[0] == 'X') {
                httpsOEMStatusCodeX = atoi(&buff[1]);
            } else if (buff[0] == 'Y') {
                httpsOEMStatusCodeY = atoi(&buff[1]);
            } else if (buff[0] == 'Z') {
                httpsOEMStatusCodeZ = atoi(&buff[1]);
            }
            if (loopDebugMode) {
                debugPrint("HTTPS OEM status codes X | Y | Z ",false);
                debugPrint(httpsOEMStatusCodeX, false);debugPrint(" | ", false);
                debugPrint(httpsOEMStatusCodeY, false);debugPrint(" | ", false);
                debugPrint(httpsOEMStatusCodeZ, true);
            }
#if 1
            if(httpsOEMStatusCodeX != 200 || (httpsOEMStatusCodeX == 200 && httpsOEMStatusCodeY != 200 && httpsOEMStatusCodeY != 0) || (httpsOEMStatusCodeX == 200 && httpsOEMStatusCodeY == 200 && httpsOEMStatusCodeZ != 200 && httpsOEMStatusCodeZ != 0))
            {
                debugPrint("HTTPS OEM status codes X | Y | Z ",false);
                debugPrint(httpsOEMStatusCodeX, false);debugPrint(" | ", false);
                debugPrint(httpsOEMStatusCodeY, false);debugPrint(" | ", false);
                debugPrint(httpsOEMStatusCodeZ, true);
                // // Abort transmission on failure  // IDE1.5_PORT_CHANGE Change
                // RawDataState::rawDataTransmissionInProgress = false;
                // RawDataState::startRawDataCollection = false;             
            }
#endif
            break;
        }
        case MSPCommand::SEND_RAW_DATA_NEXT_PKT: {
                sendNextAxis = bool(strtol(buff, NULL, 0));
                debugPrint("Send Next Axis : ", false);debugPrint(sendNextAxis);
            }
            break;
        case MSPCommand::WIFI_GET_TX_POWER:
            if (loopDebugMode) {
                debugPrint("TX power from ESP32  :",false);
                debugPrint(buff);
            }
            break;
        case MSPCommand::WIFI_ALERT_CONNECTED:
            if (loopDebugMode) { debugPrint(F("WIFI-CONNECTED;")); }
            if(getDatetime() < 1590000000.00){iuWiFi.sendMSPCommand(MSPCommand::GOOGLE_TIME_QUERY);}
            if (isBLEConnected()) {
                iuBluetooth.write("WIFI-CONNECTED;");
            }
            // if(conductor.getUsageMode() == UsageMode::OTA) {
            //     if (loopDebugMode) { debugPrint(F("OTA DNLD MODE")); }
            //     ledManager.stopColorOverride();
            //     ledManager.showStatus(&STATUS_OTA_DOWNLOAD);
            // }
            break;
        case MSPCommand::WIFI_ALERT_DISCONNECTED:
            certDownloadInProgress = false;
            if (isBLEConnected()) {
                iuBluetooth.write("WIFI-DISCONNECTED;");
                delay(2);
                iuBluetooth.write("MQTT-DISCONNECTED;");
            }
            if(conductor.getUsageMode() == UsageMode::OTA) {
                if (loopDebugMode && !certDownloadMode) { 
                    debugPrint(F("OTA In Progress - WIFI-DISCONNECTED"));
                    debugPrint(F("Sending OTA-ERR-FDW-ABORT"));
                }
                for(int i = 0 ; i < 15; i++) {
                    ledManager.overrideColor(RGB_RED);
                    delay(200);
                    ledManager.stopColorOverride();
                    delay(200);
                }
                if(!certDownloadMode){
                     // In case WiFi Disconnect/ESP Reset durig OTA, switch to OPERTATION Mode ??
                    sendOtaStatusMsg(MSPCommand::OTA_FDW_ABORT,OTA_DOWNLOAD_ERR, String(iuOta.getOtaRca(OTA_WIFI_DISCONNECT)).c_str());
                }
                if (loopDebugMode) { debugPrint(F("Switching Device mode:OTA -> OPERATION")); }
                certDownloadMode = false;
                iuWiFi.m_setLastConfirmedPublication();
                changeUsageMode(UsageMode::OPERATION);
                delay(100);
            }
            break;
        case MSPCommand::MQTT_ALERT_CONNECTED:
            if (loopDebugMode) { debugPrint(F("MQTT-CONNECTED;")); }
            if (isBLEConnected()) {
                iuBluetooth.write("MQTT-CONNECTED;");
            }
            m_mqttConnected = true;
            if(conductor.getUsageMode() == UsageMode::OTA) {         // NOTE : Need to analyze the Impact on OTA 
                if (loopDebugMode) { debugPrint(F("OTA DNLD MODE or Certificates Upgrade Mode")); }
                ledManager.stopColorOverride();
                ledManager.showStatus(&STATUS_OTA_DOWNLOAD);
            }
            break;
        case MSPCommand::MQTT_ALERT_DISCONNECTED:
            if (isBLEConnected()) {
                iuBluetooth.write("MQTT-DISCONNECTED;");
            }
            if(conductor.getUsageMode() == UsageMode::OTA) {
                if (loopDebugMode && !certDownloadMode) { 
                    debugPrint(F("OTA In Progress - MQTT-DISCONNECTED"));
                    debugPrint(F("Sending OTA-ERR-FDW-ABORT"));
                }

            m_mqttConnected = false;
            #if 0
                for(int i = 0 ; i < 15; i++) {
                    ledManager.overrideColor(RGB_RED);
                    delay(200);
                    ledManager.stopColorOverride();
                    delay(200);
                }                     //// NOTE : Not required during ota, already handled in wifi alert disconnection
                // In case connection with MQTT broker Disconnected/ESP Reset durig OTA, switch to OPERTATION Mode ??
                sendOtaStatusMsg(MSPCommand::OTA_FDW_ABORT,OTA_DOWNLOAD_ERR, String(iuOta.getOtaRca(OTA_MQTT_DISCONNECT)).c_str());
                if (loopDebugMode) { debugPrint(F("Switching Device mode:OTA -> OPERATION")); }
                iuWiFi.m_setLastConfirmedPublication();
                changeUsageMode(UsageMode::OPERATION);
                delay(100);
            #endif
            }
            break;
        case MSPCommand::ASK_WIFI_CONFIG:
            if (DOSFS.exists("/iuconfig/wifi0.conf"))
            {
                JsonObject& config = configureJsonFromFlash("/iuconfig/wifi0.conf",1);
                String jsonChar;
                config.printTo(jsonChar);
                iuWiFi.sendMSPCommand(MSPCommand::SEND_WIFI_CONFIG,jsonChar.c_str());
            }
            break;
        case MSPCommand::WIFI_ALERT_NO_SAVED_CREDENTIALS:
            if (loopDebugMode) {
                debugPrint(F("WIFI_ALERT_NO_SAVED_CREDENTIALS"));
            }
            if (isBLEConnected()) {
                iuBluetooth.write("WIFI-NOSAVEDCRED;");
            }
            break;
        case MSPCommand::SET_DATETIME:
            if (loopDebugMode) { debugPrint(F("SET_DATETIME")); debugPrint("Google_Query_time : "); debugPrint(buff); }
            setRefDatetime(buff);
            break;
        case MSPCommand::CONFIG_FORWARD_CONFIG:
            if (loopDebugMode) { debugPrint(F("CONFIG_FORWARD_CONFIG")); }
            processConfiguration(buff, true);
            break;
        case MSPCommand::CONFIG_FORWARD_CMD:
            if (loopDebugMode) { debugPrint(F("CONFIG_FORWARD_CMD")); }
            processCommand(buff);
            break;
        case MSPCommand::CONFIG_FORWARD_LEGACY_CMD:
            if (loopDebugMode) { debugPrint(F("CONFIG_FORWARD_LEGACY_CMD")); }
            processCommand(buff);
            processLegacyCommand(buff);
            break;
        case MSPCommand::WIFI_CONFIRM_ACTION:
            if (loopDebugMode) {
                debugPrint(F("WIFI_CONFIRM_ACTION: "), false);
                debugPrint(buff);
            }
            break;
        case MSPCommand::WIFI_CONFIRM_PUBLICATION:
            idx = (uint8_t) buff[0];
            if (loopDebugMode) {
                debugPrint("WIFI_CONFIRM_PUBLICATION: ", false);
                debugPrint(idx);
            }
            if (strlen(buff) > 0 && idx < sendingQueue.maxCount) {
                sendingQueue.confirmSuccessfullSend(idx);
            }
            break;
        case MSPCommand::GET_ESP_RSSI:
            iuWiFi.current_rssi = atof(&buff[0]);
            if (loopDebugMode) {
                debugPrint(F("RECEIVED WIFI RSSI : "), false);
                debugPrint(iuWiFi.current_rssi);
            }
            //Append current RSSI
            modbusFeaturesDestinations[7] = iuWiFi.current_rssi;
            break;
        case MSPCommand::GET_RAW_DATA_ENDPOINT_INFO:
            // TODO: Implement
            { 
              if(DOSFS.exists("httpConfig.conf")) {  
                JsonObject& config = configureJsonFromFlash("httpConfig.conf",1);
              
                m_httpHost = config["httpConfig"]["host"];
                m_httpPath = config["httpConfig"]["path"];
                m_httpPort = config["httpConfig"]["port"];
                m_httpUsername = config["httpConfig"]["username"];
                m_httpPassword = config["httpConfig"]["password"];
                m_httpOauth = config["httpConfig"]["oauth"];

                if(config.containsKey("httpOem")){
                    m_httpHost_oem = config["httpOem"]["host"];
                    m_httpPort_oem = config["httpOem"]["port"];
                    m_httpPath_oem = config["httpOem"]["path"];
                    m_httpUsername_oem = config["httpOem"]["username"];
                    m_httpPassword_oem = config["httpOem"]["password"];
                    m_httpOauth_oem = config["httpOem"]["oauth"];
                    httpOEMConfigPresent = true;
                }else{
                    httpOEMConfigPresent = false;
                }

              }else if(iuFlash.checkConfig(CONFIG_HTTP_FLASH_ADDRESS)){
                    String httpConfig = iuFlash.readInternalFlash(CONFIG_HTTP_FLASH_ADDRESS);
                    debugPrint(httpConfig);
                    const size_t bufferSize = JSON_OBJECT_SIZE(1) + JSON_OBJECT_SIZE(6) + 510;
                    StaticJsonBuffer<bufferSize> jsonBuffer;
                    JsonObject &config = jsonBuffer.parseObject(httpConfig);
                    if(config.success())
                    {
                        debugPrint("Http Config Found");
                        m_httpHost = config["httpConfig"]["host"];
                        m_httpPort = config["httpConfig"]["port"];
                        m_httpPath = config["httpConfig"]["path"];
                        m_httpUsername = config["httpConfig"]["username"];
                        m_httpPassword = config["httpConfig"]["password"];
                        m_httpOauth = config["httpConfig"]["oauth"];

                        if(config.containsKey("httpOem")){
                            m_httpHost_oem = config["httpOem"]["host"];
                            m_httpPort_oem = config["httpOem"]["port"];
                            m_httpPath_oem = config["httpOem"]["path"];
                            m_httpUsername_oem = config["httpOem"]["username"];
                            m_httpPassword_oem = config["httpOem"]["password"];
                            m_httpOauth_oem = config["httpOem"]["oauth"];
                            httpOEMConfigPresent = true;
                        }else{
                            httpOEMConfigPresent = false;
                        }

                        File httpFile = DOSFS.open("httpConfig.conf","w");
                        if(httpFile)
                        {
                            httpFile.print(httpConfig.c_str());
                            httpFile.flush();
                            debugPrint("httpConfig.conf File write Success");
                            flashStatusFlag = true;
                            httpFile.close();
                        }
                    }else{
                        debugPrint(F("Failed to read httpConf.conf file, using default configuration"));
                        setDefaultHTTP();
                    }
              }  
              else{
                    setDefaultHTTP();
                }
                iuWiFi.sendMSPCommand(MSPCommand::SET_RAW_DATA_ENDPOINT_HOST,m_httpHost); 
                iuWiFi.sendMSPCommand(MSPCommand::SET_RAW_DATA_ENDPOINT_PORT,String(m_httpPort).c_str()); 
                iuWiFi.sendMSPCommand(MSPCommand::SET_RAW_DATA_ENDPOINT_ROUTE,m_httpPath);


                iuWiFi.sendMSPCommand(MSPCommand::SET_RAW_DATA_ENDPOINT_HOST_OEM,m_httpHost_oem); 
                iuWiFi.sendMSPCommand(MSPCommand::SET_RAW_DATA_ENDPOINT_PORT_OEM,String(m_httpPort_oem).c_str()); 
                iuWiFi.sendMSPCommand(MSPCommand::SET_RAW_DATA_ENDPOINT_ROUTE_OEM,m_httpPath_oem);
                            
                break;
           }
            
        case MSPCommand::GET_MQTT_CONNECTION_INFO:
            if (loopDebugMode) { debugPrint(F("GET_MQTT_CONNECTION_INFO")); }
            {
            JsonObject& config = configureJsonFromFlash("MQTT.conf",1);
            if(config.success()){
                m_mqttServerIp = config["mqtt"]["mqttServerIP"];
                m_mqttServerPort = config["mqtt"]["port"];
                m_mqttUserName = config["mqtt"]["username"];
                m_mqttPassword = config["mqtt"]["password"];
                // m_tls_enabled = config["mqtt"]["tls"];
                m_accountId = config["accountid"];
            }
            if(m_mqttUserName == NULL || m_mqttPassword == NULL || m_mqttServerPort == NULL){
              // load default configurations
                String mqttConfig = iuFlash.readInternalFlash(CONFIG_MQTT_FLASH_ADDRESS);
                debugPrint(mqttConfig);
                const size_t bufferSize = JSON_OBJECT_SIZE(1) + JSON_OBJECT_SIZE(4) + 108;
                StaticJsonBuffer<bufferSize> jsonBuffer;
                JsonObject &config = jsonBuffer.parseObject(mqttConfig);
                if(config.success())
                {
                debugPrint("Mqtt Config Found");
                m_mqttServerIp = config["mqtt"]["mqttServerIP"];

                int mqttport = config["mqtt"]["port"];
                //debugPrint("INside MQTT.conf .......");
                // m_mqttServerIp.fromString(mqttServerIP);//mqttServerIP;
                m_mqttServerPort = mqttport;
                m_mqttUserName = config["mqtt"]["username"]; //MQTT_DEFAULT_USERNAME;
                m_mqttPassword = config["mqtt"]["password"]; //MQTT_DEFAULT_ASSWORD;
                // m_tls_enabled = config["mqtt"]["tls"];
                m_accountId = config["accountid"];
                }
                else{
                    setDefaultMQTT();
                }
            }
            delay(100);
            //Serial.print("UserName :");Serial.println(m_mqttUserName);
            //Serial.print("Password 1 :");Serial.println(m_mqttPassword);
            iuWiFi.sendMSPCommand(MSPCommand::SET_MQTT_SERVER_IP,
                                    m_mqttServerIp);
            iuWiFi.sendMSPCommand(MSPCommand::SET_MQTT_SERVER_PORT,
                                  String(m_mqttServerPort).c_str());
            iuWiFi.sendMSPCommand(MSPCommand::SET_MQTT_USERNAME,
                                  m_mqttUserName);// MQTT_DEFAULT_USERNAME);
            iuWiFi.sendMSPCommand(MSPCommand::SET_MQTT_PASSWORD,
                                  m_mqttPassword); //MQTT_DEFAULT_ASSWORD);
            // iuWiFi.sendMSPCommand(MSPCommand::SET_MQTT_TLS_FLAG,
            //                      String(m_tls_enabled).c_str()); 
                                  
           break;
          }
         //case MSPCommand::SEND_FINGERPRINT_ACK:
         //     if (loopDebugMode) { debugPrint(F("Send Diagnostic message Acknowledge to wifi")); }
         //     iuWiFi.sendMSPCommand(MSPCommand::SEND_DIAGNOSTIC_ACK,
         //                           diagnosticACK);
        case MSPCommand::PUBLISH_RAW_DATA:
            //Serial.println("RECEIVED ACK FROM PUBLISH_RAW_DATA COMMAND...");
            //Serial.print("Buffer:");Serial.println(buff);
            break;
        //case MSPCommand::RECEIVE_RAW_DATA_ACK:
        //    Serial.println("RECEIVED ACK FROM SEND_RAW_DATA COMMAND...");
        //    Serial.print("Buffer:");Serial.println(buff);
            
        //     break;
        case MSPCommand::SET_PENDING_HTTP_CONFIG:
            {
             //Serial.print("HTTP Pending Response ..............................................:");
             //Serial.println(buff);
             // create the JSON objects 
             DynamicJsonBuffer jsonBuffer;
             JsonObject& pendingConfigObject = jsonBuffer.parseObject(buff); 
             size_t msgLen = strlen(buff);

             char fingerprintAlarm[1500];
             char featuresThreshold[1500]; 
             char fingerprintFeatures[1500];
             char httpConfig[1500];
             
             JsonVariant fingerprintAlarmConfig  = pendingConfigObject["result"]["fingerprintAlarm"];
             JsonVariant featuresThresholdConfig = pendingConfigObject["result"]["alarm"];
             JsonVariant fingerprintFeaturesConfig = pendingConfigObject["result"]["fingerprint"];
             JsonVariant httpServerConfig = pendingConfigObject["result"]["httpConfig"];
             
             //Serial.println(fingerprintFeaturesConfig.size());
             
             fingerprintAlarmConfig.prettyPrintTo(fingerprintAlarm);
             featuresThresholdConfig.prettyPrintTo(featuresThreshold);
             fingerprintFeaturesConfig.prettyPrintTo(fingerprintFeatures);
             httpServerConfig.prettyPrintTo(httpConfig);

             

             processConfiguration(fingerprintAlarm,true);        // apply fingerprints thresholds 
             processConfiguration(featuresThreshold ,true);       // apply features thresholds 
             processConfiguration(fingerprintFeatures ,true);     // apply fingerprintsFeatures configurations 
             processConfiguration(httpConfig ,true);              // apply httpServer configurations 
             debugPrint("DEBUG INGO : Executed Pending HTTP Configs "); 
             
             break;
            }      
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
    feature->setRequired(true);
    FeatureComputer* computer = feature->getComputer();
    if (computer != NULL) {
        // Activate the feature's computer
        computer->activate();
        // Activate the computer's sources
        for (uint8_t i = 0; i < computer->getSourceCount(); ++i) {
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
    if (feature->isRequired()) {
        return false;
    }
    FeatureComputer* computer;
    for (uint8_t i = 0; i < feature->getReceiverCount(); ++i) {
        computer = feature->getReceiver(i);
        if (computer != NULL && !computer->isActive()) {
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
    feature->setRequired(false);
    FeatureComputer* computer = feature->getComputer();
    if (computer != NULL) {
        // If none of the computer's destinations is required, the computer
        // can be deactivated too.
        bool deactivatable = true;
        for (uint8_t i = 0; i < computer->getDestinationCount(); ++i) {
            deactivatable &= (!computer->getDestination(i)->isRequired());
        }
        if (deactivatable) {
            computer->deactivate();
            Feature* antecedent;
            for (uint8_t i = 0; i < computer->getSourceCount(); ++i) {
                antecedent = computer->getSource(i);
                if (!isFeatureDeactivatable(antecedent)) {
                    deactivateFeature(antecedent);
                }
            }
        }
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
    }
    activateFeature(&opStateFeature);
}

/**
 *
 */
void Conductor::deactivateGroup(FeatureGroup *group)
{
    group->deactivate();
    for (uint8_t i = 0; i < group->getFeatureCount(); ++i) {
        deactivateFeature(group->getFeature(i));
    }
}

/**
 *
 */
void Conductor::deactivateAllGroups()
{
    for (uint8_t i = 0; i < FeatureGroup::instanceCount; ++i) {
        deactivateGroup(FeatureGroup::instances[i]);
    }
}

/**
 *
 */
void Conductor::configureGroupsForOperation()
{
    if (!configureFromFlash(IUFlash::CFG_DEVICE)) {
        // Config not found, default to mainFeatureGroup
        deactivateAllGroups();
        resetDataAcquisition();
        activateGroup(m_mainFeatureGroup);
    }
    changeMainFeatureGroup(m_mainFeatureGroup);
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
    resetDataAcquisition();
    activateGroup(&calibrationGroup);
    // TODO: The following should be written in flash or sent from cloud
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

void Conductor::changeMainFeatureGroup(FeatureGroup *group)
{
    m_mainFeatureGroup = group;
    if (!configureFromFlash(IUFlash::CFG_OP_STATE)) {
        // Config not found, default to DEFAULT_ACCEL_ENERGY_THRESHOLDS
        opStateComputer.deleteAllSources();
        Feature *feat;
        for (uint8_t i = 0; i < m_mainFeatureGroup->getFeatureCount(); i++) {
            feat = m_mainFeatureGroup->getFeature(i);
            if (feat == &accelRMS512Total) {
                opStateComputer.addOpStateFeature(feat,
                                          DEFAULT_ACCEL_ENERGY_NORMAL_TH,
                                          DEFAULT_ACCEL_ENERGY_WARNING_TH,
                                          DEFAULT_ACCEL_ENERGY_HIGH_TH,
                                          1, true);
            } else {
                opStateComputer.addOpStateFeature(feat,
                                                  10000, 10000, 10000,
                                                  1, false); 
            }
        }
        activateFeature(&opStateFeature);
        setThresholdsFromFile();
    }
}


/* =============================================================================
    Time management
============================================================================= */

/**
 * Set the refDatetime and update the lastSynchrotime to now.
 */
void Conductor::setRefDatetime(double refDatetime)
{
    if (refDatetime >  0) {
        m_refDatetime = refDatetime;
        m_lastSynchroTime = millis();
        if (loopDebugMode) {
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
    uint32_t now = millis();
    char temp[50];
    // debugPrint("CHECKING BLE CONNECTION STATUS: ", false); debugPrint(itoa(m_lastBLEmessage, temp, 10), true);
    // debugPrint("TIME DIFF: ", false); debugPrint(itoa(now-m_lastBLEmessage, temp, 10), true);

    // m_lastBLEmessage == 0 indicates that BLE has started (either on boot or after reset) after disconnection
    return m_lastBLEmessage > 0 && now - m_lastBLEmessage < BLEconnectionTimeout;
}

void Conductor::resetBLEonTimeout() {
    // reset m_lastBLEmessage to zero if bluetooth connection is lost i.e. time difference between now and last message is greater than BLEconnectionTimeout
    // since m_lastBLEmessage > 0 is the first condition to check if bluetooth is connected, 
    // this will ensure isBLEConnected will return false and the mode will be switched
    uint32_t now = millis();
    if (m_lastBLEmessage > 0 && now - m_lastBLEmessage > BLEconnectionTimeout) { 
        m_lastBLEmessage = 0; 
        // debugPrint("BLE RESET");
        iuBluetooth.softReset(); 
    } 
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
            newMode = StreamingMode::WIRED;   //+++
            break;
        case UsageMode::EXPERIMENT:
            newMode = StreamingMode::WIRED;
            break;
        case UsageMode::CUSTOM:                   // CUSTOM Mode
            newMode = StreamingMode::WIRED;
            break;
        case UsageMode::OPERATION:
        case UsageMode::OPERATION_BIS:
            resetBLEonTimeout(); // reset BLE if it BLE has timed out
            if (isBLEConnected()) {
                if (iuWiFi.isConnected()) {
                    newMode = StreamingMode::WIFI_AND_BLE;
                } else {
                    newMode = StreamingMode::BLE;
                }
            } else if (iuWiFi.isConnected()) {
                newMode = StreamingMode::WIFI;
            }
             else if(!iuEthernet.isEthernetConnected){
                 newMode = StreamingMode::ETHERNET;  //Wifi is not connected but found ethernet MAC ID
             }
            break;
        case UsageMode::OTA: // During OTA, no streaming
            newMode = StreamingMode::NONE;
            break;
    }
    if (m_streamingMode == newMode) {
        // char streaming_mode_string[10];
        // debugPrint("Streaming mode not updated, streaming mode is : ", false); debugPrint(itoa(m_streamingMode, streaming_mode_string, 10), true);
        return; // Nothing to do
    }
    m_streamingMode = newMode;
    if (loopDebugMode) {
        debugPrint(F("\nStreaming mode is: "), false);
        switch (m_streamingMode) {
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
            case StreamingMode::ETHERNET:
                debugPrint(F("ETHERNET"));
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
    if (m_acquisitionMode == mode) {
        return; // Nothing to do
    }
    m_acquisitionMode = mode;
    switch (m_acquisitionMode) {
        case AcquisitionMode::RAWDATA:
            resetDataAcquisition();
            break;
        case AcquisitionMode::FEATURE:
            resetDataAcquisition();
            break;
        case AcquisitionMode::NONE:
            endDataAcquisition();
            break;
        default:
            if (loopDebugMode) { debugPrint(F("Invalid acquisition Mode")); }
            break;
    }
}

/**
 * Switch to a new Usage Mode
 */
void Conductor::changeUsageMode(UsageMode::option usage)
{
    if (m_usageMode == usage) {
        return; // Nothing to do
    }
    if(m_usageMode == UsageMode::OTA && usage != UsageMode::OPERATION)
    {
        if (loopDebugMode) {
            debugPrint(F("Except OPERATION, No other mode is allowed during OTA"));
        }
        return; // Nothing to do
    }
    m_usageMode = usage;
    String msg;
    switch (m_usageMode) {
        case UsageMode::CALIBRATION:
            configureGroupsForCalibration();
            ledManager.overrideColor(RGB_CYAN);
            msg = "calibration";
            break;
        case UsageMode::EXPERIMENT:
            ledManager.overrideColor(RGB_PURPLE);
            msg = "experiment";
            break;
        case UsageMode::OPERATION:
        case UsageMode::OPERATION_BIS:
            ledManager.stopColorOverride();
            configureGroupsForOperation();
            if ( FFTConfiguration::currentSensor == FFTConfiguration::lsmSensor)
            {
                iuAccelerometer.setGrange(FFTConfiguration::currentLSMgRange);
            }
            else
            {
                iuAccelerometerKX222.setGrange(FFTConfiguration::currentKNXgRange);
            }
            msg = "operation";
            break;
        case UsageMode::CUSTOM:
            ledManager.overrideColor(RGB_CYAN);
            //configureGroupsForOperation();
            if ( FFTConfiguration::currentSensor == FFTConfiguration::lsmSensor)
            {
                iuAccelerometer.setGrange(FFTConfiguration::currentLSMgRange);
            }
            else
            {
                iuAccelerometerKX222.setGrange(FFTConfiguration::currentKNXgRange);
            }
            msg = "custom";
            break;        
        case UsageMode::OTA:
            ledManager.overrideColor(RGB_CYAN);
            msg = "ota";
            break;
        default:
            if (loopDebugMode) {
                debugPrint(F("Invalid usage mode preset"));
            }
            return;
    }
    changeAcquisitionMode(UsageMode::acquisitionModeDetails[m_usageMode]);
    updateStreamingMode();
    if (loopDebugMode) { debugPrint("\nSet up for " + msg + "\n"); }
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
    if (m_inDataAcquistion) {
        return true; // Already in data acquisition
    }
    // m_inDataAcquistion should be set to true BEFORE triggering the data
    // acquisition, otherwise I2S buffer won't be emptied in time
    m_inDataAcquistion = true;
    if (!iuI2S.triggerDataAcquisition(m_callback)) {
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
    if (!m_inDataAcquistion) {
        return; // nothing to do
    }
    iuI2S.endDataAcquisition();
    m_inDataAcquistion = false;
    for (uint8_t i = 0; i < Feature::instanceCount; ++i) {
        Feature::instances[i]->reset();
    }
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
    bool force = false;
    if (FFTConfiguration::currentSensor == FFTConfiguration::lsmSensor)
    {
        if (!m_inDataAcquistion || m_acquisitionMode == AcquisitionMode::NONE) {
            return;
        }
        if (iuI2C.isError()) {
            if (debugMode) {
                debugPrint(F("I2C error"));
            }
            iuI2C.resetErrorFlag();  // Reset I2C error
            iuI2S.readData();         // Empty I2S buffer to continue
            return;
        }
        // If EXPERIMENT mode, send last data batch before collecting the new data
        if (m_usageMode == UsageMode::EXPERIMENT) {
            if (loopDebugMode && (m_acquisitionMode != AcquisitionMode::RAWDATA ||
                                m_streamingMode != StreamingMode::WIRED)) {
                debugPrint(F("EXPERIMENT should be RAW DATA + USB mode."));
            }
        if (inCallback) {
                iuI2S.sendData(iuUSB.port);             // raw audio data 
                iuAccelerometer.sendData(iuUSB.port);

        }
                
            force = true;
        }

        // CUSTOM Mode

    if (m_usageMode == UsageMode::CUSTOM ) {
            if (loopDebugMode && (m_acquisitionMode != AcquisitionMode::RAWDATA ||
                                m_streamingMode != StreamingMode::WIRED)) {
                debugPrint(F("CUSTOM Mode should be RAW DATA + USB mode."));
            }
        if (inCallback) {

            float *acceleration;
            float aucostic;
            char rawData[50]; 
            aucostic = iuI2S.getData();                               // raw audio data 
            acceleration = iuAccelerometer.getData(iuUSB.port);       // raw accel data

            snprintf(rawData,50,"%04.3f,%04.3f,%04.3f,%.3f",acceleration[0],acceleration[1],acceleration[2],aucostic);
            
            String payload = "";
            payload = "$";
            payload += m_macAddress.toString().c_str();
            payload += ",";
            payload += rawData;
            payload += "#";
            
            iuUSB.port->write(payload.c_str());
            
        }
                
            force = true;
        }
        // Collect the new data
        for (uint8_t i = 0; i < Sensor::instanceCount; ++i) {
        if (strcmp("ACC", Sensor::instances[i]->getName())==0)
            {
                Sensor::instances[i]->acquireData(inCallback, force);
            }
        }
    }
    else if ( FFTConfiguration::currentSensor == FFTConfiguration::kionixSensor)
    {
        for (uint8_t i = 0; i < Sensor::instanceCount; ++i) {
        if (strcmp("ACX", Sensor::instances[i]->getName())==0)
            {
                Sensor::instances[i]->acquireData(inCallback, force);
            }
        }
    }
}

/**
 * @brief capture only Audio sensor data
 * 
 * @param inCallback 
 */
void Conductor::acquireAudioData(bool inCallback){
  for (uint8_t i = 0; i < Sensor::instanceCount; ++i) {
        if ( strcmp("MIC", Sensor::instances[i]->getName())==0 )
        {
            Sensor::instances[i]->acquireData(inCallback,true);
        }
    }  
}
/**
 * Data acquisition function
 *
 * Method formerly benchmarked for (Temperature).
 */

void Conductor::acquireTemperatureData()
{
    for (uint8_t i = 0; i < Sensor::instanceCount; ++i) {
        if (strcmp("T10", Sensor::instances[i]->getName())==0 || strcmp("BAT", Sensor::instances[i]->getName())==0)
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
    if (m_acquisitionMode != AcquisitionMode::FEATURE) {
        return;
    }
    // Run feature computers
    for (uint8_t i = 0; i < FeatureComputer::instanceCount; ++i) {
        FeatureComputer::instances[i]->compute();
    }
}

/**
 * Send feature data through a Serial port, depending on StreamingMode
 *
 * NB: If the AcquisitionMode is not FEATURE, does nothing.
 */
void Conductor::streamFeatures()
{
    if (m_acquisitionMode != AcquisitionMode::FEATURE) {
        return;
    }
    IUSerial *ser1 = NULL;
    bool sendFeatureGroupName1 = false;
    IUSerial *ser2 = NULL;
    bool sendFeatureGroupName2 = false;

    switch (m_streamingMode) {
        case StreamingMode::NONE:
            //ser1 = &iuWiFi;    
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
        case StreamingMode::ETHERNET:
            ser1 = &iuWiFi;    
            break;
        default:
            if (loopDebugMode) {
                debugPrint(F("StreamingMode not handled: "), false);
                debugPrint(m_streamingMode);
            }
    }
    double timestamp = getDatetime();
    float batteryLoad = iuBattery.getBatteryLoad();
    for (uint8_t i = 0; i < FeatureGroup::instanceCount; ++i) {                       // instanceCount =  [0:7]
        // TODO Switch to new streaming format once the backend is ready
        if (ser1) {
            if (m_streamingMode == StreamingMode::WIFI ||
                m_streamingMode == StreamingMode::WIFI_AND_BLE || m_streamingMode == StreamingMode::ETHERNET)
            { 
                if(RawDataState::rawDataTransmissionInProgress == false)
                {   
                    FeatureGroup::instances[i]->bufferAndStream(
                    ser1, IUSerial::MS_PROTOCOL, m_macAddress,
                    ledManager.getOperationState(), batteryLoad, timestamp,
                    sendFeatureGroupName1);
                }
                // FeatureGroup::instances[i]->bufferAndQueue(
                //     &sendingQueue, IUSerial::MS_PROTOCOL, m_macAddress,
                //     ledManager.getOperationState(), batteryLoad, timestamp,
                //     sendFeatureGroupName1);
            } else {
                FeatureGroup::instances[i]->legacyStream(ser1, m_macAddress,
                    ledManager.getOperationState(), batteryLoad, timestamp,
                    sendFeatureGroupName1);
            }
        }
        if (ser2) {                                                                 // Streaming only over BLE
            FeatureGroup::instances[i]->legacyStream(ser2, m_macAddress,
                ledManager.getOperationState(), batteryLoad, timestamp,
                sendFeatureGroupName2, 1);
        }
//        FeatureGroup::instances[i]->bufferAndQueue(
//            &sendingQueue, IUSerial::MS_PROTOCOL, m_macAddress,
//            ledManager.getOperationState(), batteryLoad, timestamp,
//            true);
        if (FeatureStates::isISRActive != true && FeatureStates::isISRDisabled && computationDone == true){   
                FeatureStates::isFeatureStreamComplete = true;   // publication completed
                FeatureStates::isISRActive = true;
                //debugPrint("Published to WiFi Complete !!!");
                // createFeatureGroupjson();
            }
    }
   #if 0
   CharBufferNode *nodeToSend = sendingQueue.getNextBufferToSend();
    //Serial.print("nodeToSend : ");Serial.println(nodeToSend->buffer);
    if (nodeToSend ) {
           uint16_t msgLen = strlen(nodeToSend->buffer);
           if (m_streamingMode == StreamingMode::WIFI || m_streamingMode == StreamingMode::WIFI_AND_BLE)
           {
                if(RawDataState::rawDataTransmissionInProgress == false)
                {     
                    iuWiFi.startLiveMSPCommand(MSPCommand::PUBLISH_FEATURE_WITH_CONFIRMATION, msgLen + 2);
                    iuWiFi.streamLiveMSPMessage((char) nodeToSend->idx);
                    iuWiFi.streamLiveMSPMessage(':');
                    iuWiFi.streamLiveMSPMessage(nodeToSend->buffer, msgLen);
                    iuWiFi.endLiveMSPCommand();
                    sendingQueue.attemptingToSend(nodeToSend->idx);
                }
           }
           if(m_streamingMode == StreamingMode::ETHERNET){
                uint16_t msgLen = strlen(nodeToSend->buffer);
                //char streamingHeader[732];
                //char streamingData[732];
                char feturesStreaming[800];
                //snprintf(streamingData,732,"XXXAdmin;;;%s;;;%s", m_macAddress.toString().c_str(),nodeToSend->buffer);
                //Serial.print("Streamign Header : ");Serial.println(streamingHeader);
                //if(rbase64.encode(streamingData) ==  RBASE64_STATUS_OK) {
                       // send the streaming JSON
                    snprintf(feturesStreaming,800,"{\"deviceId\":\"%s\",\"transport\":%d,\"messageType\":%d,\"payload\":\"XXXAdmin;;;%s;;;%s\" }",
                    m_macAddress.toString().c_str(),0,0,/*rbase64.result()*/m_macAddress.toString().c_str(),nodeToSend->buffer );   
                    iuWiFi.write(feturesStreaming);      
                    iuWiFi.write("\n");
                //}
                if(loopDebugMode){
                    debugPrint(feturesStreaming);
                }
                sendingQueue.attemptingToSend(nodeToSend->idx);
            }
            //sendingQueue.attemptingToSend(nodeToSend->idx);
             
     }
     
    sendingQueue.maintain();
    
    if (FeatureStates::isISRActive != true && FeatureStates::isISRDisabled){   
                //Serial.print("Streaming Data : ");
               // Serial.println(nodeToSend->buffer);
                FeatureStates::isFeatureStreamComplete = true;   // publication completed
                FeatureStates::isISRActive = true;
                //Serial.println("Published to WiFi Complete !!!");
            }

    #endif 
}

/**
 * @brief Compute all the diagnostic triggers
 * 
 */
void Conductor::computeTriggers(){
   if(m_streamingMode == StreamingMode::WIFI || m_streamingMode == StreamingMode::WIFI_AND_BLE){
    // read the dig config 
    iuTrigger.m_specializedCompute();
    if(iuTrigger.DIG_LIST[0] != NULL && iuTrigger.DIG_COUNT != 0) {
      computeDiagnoticState(iuTrigger.DIG_LIST,iuTrigger.DIG_COUNT );
    }
   }
    // iuTrigger.DIG_COUNT = 0;
    // reportableIndexCounter = 0;
    // clearDiagResultArray();
    
}

void Conductor::streamDiagnostics(){
   if((m_streamingMode == StreamingMode::WIFI || m_streamingMode == StreamingMode::WIFI_AND_BLE) && validTimeStamp()){
        const char* dId;
        bool publishDiag = false;
        bool publishAlert = false;
        bool publishFres = false;
        DynamicJsonBuffer reportableJsonBUffer;
        JsonObject& reportableJson = reportableJsonBUffer.createObject();
        int publishSelect = publish::ALERT_POLICY;
        uint32_t nowT = millis();
        if (nowT - digLastPublish >= diagStreamingPeriod)
        {
            digLastPublish = nowT;
            publishSelect = publish::STREAM;
            publishDiag = true;
            if (diagAlertResults[0] != NULL && reportableDIGLength > 0)
            {
                publishSelect = publish::ALERT_POLICY;
            }
        }

        if(nowT - fresLastPublish >= fresPublishPeriod)
        {
            fresLastPublish = nowT;
            publishSelect = publish::STREAM;
            publishFres = true;
            if (diagAlertResults[0] != NULL && reportableDIGLength > 0)
            {
                publishSelect = publish::ALERT_POLICY;
            }
        }
        switch (publishSelect)
        {
        case publish::ALERT_POLICY: //Alert Policies
            if (diagAlertResults[0] != NULL && reportableDIGLength > 0)
            {
                for (size_t i = 0; i < reportableDIGLength; i++)
                {
                    dId = diagAlertResults[i]; //.c_str();
                    if (dId != NULL)
                    {
                        // construct the RDIG JSON
                        JsonObject &diagnostic = reportableJson.createNestedObject(dId);
                        JsonArray &ftr = diagnostic.createNestedArray("FTR");
                        // Add Firing Triggers list
                        addFTR(dId, ftr, i);
                        diagnostic["ALRREP"] = alert_repeat_state[i];
                    }
                }
                publishAlert = true;
            }
            if((diagAlertResults[0] != NULL && publishAlert == true)){
                //reportableJson.printTo(Serial); debugPrint("");
                reportableJson.printTo(m_diagnosticResult,DIG_PUBLISHED_BUFFER_SIZE);
                snprintf(m_diagnosticPublishedBuffer,DIG_PUBLISHED_BUFFER_SIZE,"{\"DEVICEID\":\"%s\",\"TIMESTAMP\":%.2f,\"DIGRES\":%s}",m_macAddress.toString().c_str(),getDatetime(),m_diagnosticResult);
                // Published to MQTT 
                iuWiFi.sendMSPCommand(MSPCommand::PUBLISH_IU_RDIG,m_diagnosticPublishedBuffer);
                // if(loopDebugMode){
                //     debugPrint("O/P Buffer : ",false);
                //     debugPrint(m_diagnosticPublishedBuffer);
                //     debugPrint("BUFF LEN :",false);
                //     debugPrint(strlen(m_diagnosticPublishedBuffer));
                // }
            }
            break;
        case publish::STREAM: //Streaming Diagnostics
            if(publishDiag && digStream){
                if (iuTrigger.DIG_COUNT > 0)
                {
                    for (int index = 0; index < iuTrigger.DIG_COUNT; index++)
                    {
                        dId = (char *)iuTrigger.DIG_LIST[index].c_str(); //.c_str();
                        if (dId != NULL)
                        {
                            // construct the RDIG JSON
                            JsonObject &diagnostic = reportableJson.createNestedObject(dId);
                            JsonArray &ftr = diagnostic.createNestedArray("FTR");
                            // Add Firing Triggers list
                            addFTR(ftr, index);
                            if (iuTrigger.DIG_STATE[index])
                            {
                                diagnostic["DSTATE"] = true;
                            }
                            else if (!iuTrigger.DIG_STATE[index])
                            {
                                diagnostic["DSTATE"] = false;
                            }
                        }else{
                            publishDiag = false;
                        }
                    }
                    publishDiag = true;
                }
                else{
                    publishDiag = false;
                }
                if((iuTrigger.DIG_LIST[0] != NULL && publishDiag == true) ){
                    //reportableJson.printTo(Serial); debugPrint("");
                    reportableJson.printTo(m_diagnosticResult,DIG_PUBLISHED_BUFFER_SIZE);
                    snprintf(m_diagnosticPublishedBuffer,DIG_PUBLISHED_BUFFER_SIZE,"{\"DEVICEID\":\"%s\",\"TIMESTAMP\":%.2f,\"DIGRES\":%s}",m_macAddress.toString().c_str(),getDatetime(),m_diagnosticResult);
                    // Published to MQTT 
                    iuWiFi.sendMSPCommand(MSPCommand::PUBLISH_IU_DIAGNOSTIC,m_diagnosticPublishedBuffer);
                    // if(loopDebugMode){
                    //     debugPrint("O/P Buffer : ",false);
                    //     debugPrint(m_diagnosticPublishedBuffer);
                    //     debugPrint("BUFF LEN :",false);
                    //     debugPrint(strlen(m_diagnosticPublishedBuffer));
                    // }
                }
            }
            if(publishFres && fresStream){
                JsonObject& fres = createFeatureGroupjson()["FRES"];
                //reportableJson.printTo(Serial); debugPrint("");
                fres.printTo(m_diagnosticResult,DIG_PUBLISHED_BUFFER_SIZE);
                snprintf(m_diagnosticPublishedBuffer,DIG_PUBLISHED_BUFFER_SIZE,"{\"DEVICEID\":\"%s\",\"TIMESTAMP\":%.2f,\"FRES\":%s}",m_macAddress.toString().c_str(),getDatetime(),m_diagnosticResult);
                // Published to MQTT 
                iuWiFi.sendMSPCommand(MSPCommand::PUBLISH_IU_FRES,m_diagnosticPublishedBuffer);
                // if(loopDebugMode){
                //     debugPrint("O/P Buffer : ",false);
                //     debugPrint(m_diagnosticPublishedBuffer);
                //     debugPrint("BUFF LEN :",false);
                //     debugPrint(strlen(m_diagnosticPublishedBuffer));
                // }
            }
            break;
        default:
            break;
        }
        
            // if((diagAlertResults[0] != NULL && publishDiag == true)|| (iuTrigger.DIG_LIST[0] != NULL && publishAlert == true) ){
            //     //reportableJson.printTo(Serial); debugPrint("");
            //     reportableJson.printTo(m_diagnosticResult,DIG_PUBLISHED_BUFFER_SIZE);
            //     snprintf(m_diagnosticPublishedBuffer,DIG_PUBLISHED_BUFFER_SIZE,"{\"DEVICEID\":\"%s\",\"TIMESTAMP\":%.2f,\"DIGRES\":%s}",m_macAddress.toString().c_str(),getDatetime(),m_diagnosticResult);
            //     // Published to MQTT 
            //     iuWiFi.sendMSPCommand(MSPCommand::PUBLISH_IU_DIAGNOSTIC,m_diagnosticPublishedBuffer);
            //     if(loopDebugMode){
            //         debugPrint("O/P Buffer : ",false);
            //         debugPrint(m_diagnosticPublishedBuffer);
            //         debugPrint("BUFF LEN :",false);
            //         debugPrint(strlen(m_diagnosticPublishedBuffer));
            //     }
            // }
          
        }
    iuTrigger.DIG_COUNT = 0;
    reportableIndexCounter = 0;
    clearDiagResultArray();
}

void Conductor::constructPayload(const char* dId,JsonObject& desc ){
    // Not in Used
    JsonObject& descJson = configureJsonFromFlash("iuconfig/desc.conf",1);
    desc["FA"] =   1;//descJson["DESC"][dId]["FA"].as< const char* >();
    desc["RR"] =   1;//descJson["DESC"][dId]["RR"].as< const char* >();
    desc["CMSG"] = 1;//descJson["DESC"][dId]["CMSG"].as< const char* >();
                  
} 

// Append the Firing Triggers of Reportable Diagnostic
void Conductor::addFTR(const char* dId ,JsonArray& FTR,uint8_t id ){
    // get the list of firing triggers 
    uint8_t dig_Index = reportableDIGID[id];
    uint8_t ftr_Count = iuTrigger.ACTIVE_TRGCOUNT[dig_Index];   // get DIG Index from DIG NAME 
    for (size_t tId = 0; tId < ftr_Count; tId++)
    {
        char* TRG_ID =  iuTrigger.activeTRG[dig_Index][tId];
        FTR.add(TRG_ID);
    }
}

// Append the Firing Triggers of Reportable Diagnostic
void Conductor::addFTR(JsonArray& FTR,uint8_t id ){
    // get the list of firing triggers 
    uint8_t dig_Index = id;
    uint8_t ftr_Count = iuTrigger.ACTIVE_TRGCOUNT[dig_Index];   // get DIG Index from DIG NAME 
    for (size_t tId = 0; tId < ftr_Count; tId++)
    {
        char* TRG_ID =  iuTrigger.activeTRG[dig_Index][tId];
        FTR.add(TRG_ID);
    }
}
/**
 * Send the acceleration raw data.
 *
 * @param axis 0, 1 or 3, corresponding to axis X, Y or Z
 */
void Conductor::sendAccelRawData(uint8_t axisIdx)
{
     
    if (axisIdx > 2) {
        return;
    }
    char accelEnergyName[4] = "A00";
    char axis[4] = "XYZ";
    accelEnergyName[2] = axis[axisIdx];
    Feature *accelEnergy = Feature::getInstanceByName(accelEnergyName);
    if (accelEnergy == NULL) {
        return;
    }
    // Streaming raw data through bluetooth for blocksize of 4096 is not possible due to BLE buffer limitations. 
    //The following function will be deprecated on the app from Firmware v1.1.3, and should not be used.
    if (m_streamingMode == StreamingMode::BLE)
    {
        iuBluetooth.write("REC,");
        iuBluetooth.write(m_macAddress.toString().c_str());                  
        iuBluetooth.write(',');
        iuBluetooth.write(axis[axisIdx]);
        accelEnergy->stream(&iuBluetooth, 4);
        iuBluetooth.write(';');
        delay(10);
    }
    else if (m_streamingMode == StreamingMode::WIFI ||
             m_streamingMode == StreamingMode::WIFI_AND_BLE ) {
       
        int idx = 0;                        // Tracks number of elements filled in txBuffer
        for (int i =0; i < IUMessageFormat::maxBlockSize; i++) rawData.txRawValues[i] = 0;
        
        rawData.timestamp = getDatetime();
        rawData.axis = axis[axisIdx];
        //TODO check if 32 sections are actually ready. If not ready, notify ESP with a different MSP command?
        idx = accelEnergy->sendToBuffer(rawData.txRawValues, 0, FFTConfiguration::currentBlockSize / 128);  

        // Although IUMessageFormat::maxBlockSize raw data bytes will be sent to the ESP, the ESP will only HTTP POST currentBlockSize elements
        iuWiFi.sendLongMSPCommand(MSPCommand::SEND_RAW_DATA, 3000000,
                                  (char*) &rawData, sizeof rawData);               
        delay(2800);

     }else if(m_streamingMode == StreamingMode::ETHERNET){      // Ethernet Mode
        uint16_t maxLen = 16400;   //3500
        char txBuffer[maxLen];
        for (uint16_t i =0; i < maxLen; i++) {
            txBuffer[i] = 0;
        }
        txBuffer[0] = axis[axisIdx];    
        uint16_t idx = 1;
        idx += accelEnergy->sendToBuffer(txBuffer, idx, 4);   //4
        txBuffer[idx] = 0; // Terminate string (idx incremented in sendToBuffer)

        //construct the FFT JSON
        char rawAcceleration[maxLen];
        char* accelX;char* accelY; char* accelZ;
        if(txBuffer[0] == 'X' && axisIdx == 0){
            
            memmove(RawDataState::rawAccelerationX,txBuffer + 2, strlen(txBuffer) - 1); // sizeof(txBuffer)/sizeof(txBuffer[0]) -2);
            debugPrint("RawAcel X: ",false);debugPrint(RawDataState::rawAccelerationX);
        }else if(txBuffer[0]== 'Y' && axisIdx == 1)
        {
            memmove(RawDataState::rawAccelerationY,txBuffer + 2, strlen(txBuffer) - 1 ); //sizeof(txBuffer)/sizeof(txBuffer[0]) -2);
            debugPrint("RawAcel Y: ",false);debugPrint(RawDataState::rawAccelerationY);
        }else if(txBuffer[0] == 'Z' && axisIdx == 2){
            
            memmove(RawDataState::rawAccelerationZ,txBuffer + 2, strlen(txBuffer) - 1) ; //sizeof(txBuffer)/sizeof(txBuffer[0]) -2);    
            debugPrint("RawAcel Z: ",false);debugPrint(RawDataState::rawAccelerationZ);
            snprintf(rawAcceleration,maxLen,"{\"deviceId\":\"%s\",\"transport\":%d,\"messageType\":%d,\"payload\":\"{\\\"deviceId\\\":\\\"%s\\\",\\\"firmwareVersion\\\":\\\"%s\\\",\\\"samplingRate\\\":%d,\\\"blockSize\\\":%d,\\\"X\\\":\\\"%s\\\",\\\"Y\\\":\\\"%s\\\",\\\"Z\\\":\\\"%s\\\"}\"}",m_macAddress.toString().c_str(),1,0,m_macAddress.toString().c_str(),FIRMWARE_VERSION,IULSM6DSM::defaultSamplingRate,512,RawDataState::rawAccelerationX,RawDataState::rawAccelerationY,RawDataState::rawAccelerationZ);
            
            //iuWifi in this case is UART pointing to ethernet controller
            iuWiFi.write(rawAcceleration);           // send the rawAcceleration over UART 
            iuWiFi.write("\n");            
            if(loopDebugMode){
               debugPrint("Raw Acceleration:",true);
               debugPrint(rawAcceleration);     
            }
            //FREE ALLOCATED MEMORY 
            memset(RawDataState::rawAccelerationX,0,sizeof(RawDataState::rawAccelerationX));
            memset(RawDataState::rawAccelerationY,0,sizeof(RawDataState::rawAccelerationY));
            memset(RawDataState::rawAccelerationZ,0,sizeof(RawDataState::rawAccelerationZ));
        }
     }
}

// Copies raw data at current time instant to buffers. These buffers should 
// be changed later in the optimization pass after v1.1.3
void Conductor::rawDataRequest() {  
    
    memset(RawDataState::rawAccelerationX, 0, IUMessageFormat::maxBlockSize * 2);
    memset(RawDataState::rawAccelerationY, 0, IUMessageFormat::maxBlockSize * 2);
    memset(RawDataState::rawAccelerationZ, 0, IUMessageFormat::maxBlockSize * 2);
    
    // Feature *accelEnergyX = Feature::getInstanceByName("A0X");
    // Feature *accelEnergyY = Feature::getInstanceByName("A0Y");
    // Feature *accelEnergyZ = Feature::getInstanceByName("A0Z");
    
    rawDataRecordedAt = getDatetime();

    // accelEnergyX->sendToBuffer((q15_t*) rawAccelerationX, 0, FFTConfiguration::currentBlockSize / 128);
    // accelEnergyY->sendToBuffer((q15_t*) rawAccelerationY, 0, FFTConfiguration::currentBlockSize / 128);
    // accelEnergyZ->sendToBuffer((q15_t*) rawAccelerationZ, 0, FFTConfiguration::currentBlockSize / 128);   

    // Indicate the raw data is to be collected
    RawDataState::startRawDataCollection = true;
    RawDataState::XCollected = false;
    RawDataState::YCollected = false;
    RawDataState::ZCollected = false;
    RawDataState::startRawDataTransmission = false;
    RawDataState::rawDataTransmissionInProgress = false;
    if(loopDebugMode) {
        debugPrint("Raw data request: starting raw data collection at: ",false); debugPrint(rawDataRecordedAt);
    }
}
/**
 * Should be called every loop iteration. If session is in progress, sends stored axis data to the ESP then waits untill
 * HTTP 200 is received for that axis. Does not proceed to next axis if HTTP 200 is not received.
 * Implemented a retry mechanism. If Any axis not received the response it will retry after 5s.
 * Retry count = 20000 / 5000; 
 */
void Conductor::manageRawDataSending() {
    // Start raw data transmission session
    if(RawDataState::startRawDataCollection &&
       RawDataState::XCollected && RawDataState::YCollected && RawDataState::ZCollected &&
       !RawDataState::rawDataTransmissionInProgress)
    {
        if(loopDebugMode) {
            debugPrint("Raw data request: collected raw data, starting transmission");
        }
        RawDataState::rawDataTransmissionInProgress = true;
        httpsStatusCodeX = httpsStatusCodeY = httpsStatusCodeZ = 0;
        httpsOEMStatusCodeX = httpsOEMStatusCodeY = httpsOEMStatusCodeZ = 0;
        sendNextAxis = false;
        XSentToWifi = YsentToWifi = ZsentToWifi = false;    
    }

    if (RawDataState::rawDataTransmissionInProgress) {
        // double timeSinceLastSentToESP = millis() - lastPacketSentToESP; // use later for retry mechanism
        if (!XSentToWifi) {
            prepareRawDataPacketAndSend('X');
            XSentToWifi = true; 
            // RawDataTimeout = millis(); // IDE1.5_PORT_CHANGE
            RawDataTotalTimeout = millis();
            iuWiFi.m_setLastConfirmedPublication();
            if(loopDebugMode) {
                debugPrint("Raw data request: X sent to wifi");
            }
            // lastPacketSentToESP = millis();
        } else if ((httpsStatusCodeX == 200 || httpsOEMStatusCodeX == 200) && (!YsentToWifi && sendNextAxis)) { 
            prepareRawDataPacketAndSend('Y');
            YsentToWifi = true;
            sendNextAxis = false;
            // RawDataTimeout = millis(); // IDE1.5_PORT_CHANGE
            RawDataTotalTimeout = millis();
            iuWiFi.m_setLastConfirmedPublication();
            if(loopDebugMode) {
                debugPrint("Raw data request: X delivered, Y sent to wifi");
            }
            // lastPacketSentToESP = millis();
        } else if ((httpsStatusCodeY == 200 || httpsOEMStatusCodeY == 200) && (!ZsentToWifi && sendNextAxis)) {
            prepareRawDataPacketAndSend('Z');
            ZsentToWifi = true;
            sendNextAxis = false;
            // RawDataTimeout = millis(); // IDE1.5_PORT_CHANGE
            RawDataTotalTimeout = millis();
            iuWiFi.m_setLastConfirmedPublication();
            if(loopDebugMode) {
                debugPrint("Raw data request: Y delivered, Z sent to wifi");
            }
            // lastPacketSentToESP = millis();
        } 
        // else if((millis() - RawDataTimeout) > 8000 && XSentToWifi && httpsStatusCodeX != 200){
        //     prepareRawDataPacketAndSend('X');
        //     XSentToWifi = true;
        //     RawDataTimeout = millis(); // IDE1.5_PORT_CHANGE
        //     if(loopDebugMode) {
        //         debugPrint("Raw data request: Resending X, X sent to wifi");
        //     }
        // } else if((millis() - RawDataTimeout) > 8000 && XSentToWifi && httpsStatusCodeX == 200 && httpsStatusCodeY != 200 ){
        //     prepareRawDataPacketAndSend('Y');
        //     YsentToWifi = true;
        //     RawDataTimeout = millis(); // IDE1.5_PORT_CHANGE
        //     if(loopDebugMode) {
        //         debugPrint("Raw data request: Resending Y, Y sent to wifi");
        //     }
        // } else if((millis() - RawDataTimeout) > 8000 && XSentToWifi && YsentToWifi && httpsStatusCodeX == 200 && httpsStatusCodeY == 200 && httpsStatusCodeZ != 200 ){
        //     prepareRawDataPacketAndSend('Z');
        //     ZsentToWifi = true;
        //     RawDataTimeout = millis(); // IDE1.5_PORT_CHANGE
        //     if(loopDebugMode) {
        //         debugPrint("Raw data request: Resending Z, Z sent to wifi");
        //     }
        // }
        if(httpOEMConfigPresent){
            if ((httpsStatusCodeX == 200 && httpsStatusCodeY == 200 && httpsStatusCodeZ == 200) && (httpsOEMStatusCodeX == 200 && httpsOEMStatusCodeY == 200 && httpsOEMStatusCodeZ == 200) ) {
            // End the transmission session, reset RawDataState::startRawDataCollection and RawDataState::rawDataTransmissionInProgress
            // Rest of the tracking variables are reset when rawDataRequest() is called
                if(loopDebugMode) {
                    debugPrint("Raw data request: Z delivered, ending transmission session");
                }
            RawDataState::startRawDataCollection = false;
            RawDataState::rawDataTransmissionInProgress = false;    
            }
        }else{
            if (httpsStatusCodeX == 200 && httpsStatusCodeY == 200 && httpsStatusCodeZ == 200){
                if(loopDebugMode) {
                    debugPrint("Raw data request: Z delivered, ending transmission session");
                }
                RawDataState::startRawDataCollection = false;
                RawDataState::rawDataTransmissionInProgress = false;
            }
        }
        
        if((millis() - RawDataTotalTimeout) > 30000)
        { // IDE1.5_PORT_CHANGE -- On timeout of 4 Sec. if no response OK/FAIL then abort transmission
            RawDataState::startRawDataCollection = false;
            RawDataState::rawDataTransmissionInProgress = false;              
            if(loopDebugMode){
                debugPrint("RawDataTimeout Exceeded,transmission Aborted");
            }
        }
    }
}

void Conductor::prepareRawDataPacketAndSend(char axis) {
    rawData.axis = axis;
    rawData.timestamp = rawDataRecordedAt;
    switch(axis) {
        case 'X':
            memcpy(rawData.txRawValues, RawDataState::rawAccelerationX, IUMessageFormat::maxBlockSize * 2);
            break;
        case 'Y':
            memcpy(rawData.txRawValues, RawDataState::rawAccelerationY, IUMessageFormat::maxBlockSize * 2);
            break;
        case 'Z':
            memcpy(rawData.txRawValues, RawDataState::rawAccelerationZ, IUMessageFormat::maxBlockSize * 2);
            break;
    }
    iuWiFi.sendLongMSPCommand(MSPCommand::SEND_RAW_DATA, 10000000,
                                        (char*) &rawData, sizeof rawData);
    if (loopDebugMode) {
        debugPrint("Sent ", false);debugPrint(axis,false);debugPrint(" data which was recorded at ",false);
        debugPrint(rawDataRecordedAt);
    }
}

// void Conductor::startRawDataSendingSession() {
//     RawDataState::rawDataTransmissionInProgress = true;
//     httpStatusCodeX = httpStatusCodeY = httpStatusCodeZ = 0;
//     XSentToWifi = YsentToWifi = ZsentToWifi = false;    
// }

/**
 * Handle periodical publication of accel raw data.
 */
void Conductor::periodicSendAccelRawData()
{
    uint32_t now = millis();
    if ((now - m_rawDataPublicationStart > m_rawDataPublicationTimer) && !RawDataState::rawDataTransmissionInProgress) {
        iuWiFi.sendHostSamplingRate(FFTConfiguration::calculatedSamplingRate);
        if (loopDebugMode)
            debugPrint(F("***  Sending Raw Data ***"));
        delay(500);
        if (m_streamingMode == StreamingMode::WIFI || m_streamingMode == StreamingMode::WIFI_AND_BLE) {
            rawDataRequest();
            // startRawDataSendingSession();
        } else if (m_streamingMode == StreamingMode::ETHERNET) {
            sendAccelRawData(0);
            sendAccelRawData(1);
            sendAccelRawData(2);
        }
        m_rawDataPublicationStart = now;
        resetDataAcquisition();
    }
}


/* =============================================================================
    Send Diagnostic Fingerprint data
============================================================================= */

// bool Conductor::sendDiagnosticFingerPrints(){
//   //static int count = 0;
//   //debugPrint("SENDING ...........");

//   int messageLength = strlen(fingerprintData);
 
//   char FingerPrintResult[150 + messageLength];
//   snprintf(FingerPrintResult, 150 + messageLength, "{\"macID\":\"%s\",\"timestamp\": %lf,\"state\":\"%d\",\"accountId\":\"%s\",\"fingerprints\": %s }", m_macAddress.toString().c_str(),getDatetime(),ledManager.getOperationState(),"XXXAdmin",fingerprintData);

//     uint32_t lock_delay_start = millis();
//     while(sync_fingerprint_lock == false && first_compute == true)
//     {
//     //   Serial.println("Waiting for lock to be released before publishing fingerprints");
//       // wait till the lock is released
//     }
//     uint32_t lock_delay_end = millis();
//     char lock_delay[10];
//     itoa((lock_delay_end - lock_delay_start), lock_delay, 10);
//     Serial.print("LOCK DELAY: ");
//     Serial.println(lock_delay);
    
// if(sync_fingerprint_lock == true){
//     first_compute = true;

//  //if( isFingerprintConfigured == NULL) {
//   debugPrint("Published Fingerprints"); 
//   //Serial.print("Message Length :");Serial.println(messageLength);
//   iuWiFi.sendMSPCommand(MSPCommand::SEND_DIAGNOSTIC_RESULTS,FingerPrintResult );  
//  }
//  else {
//   //debugPrint("FingerprintConfigured is not configured !!!!");
//  } 
// }

void Conductor::sendDiagnosticFingerPrints() {  

    double fingerprint_timestamp = getDatetime();

    if (strlen(fingerprintData) > 5) {//handle empty fingerprint configuration (fingerprintData will be "{}" with escape characters)
            bool ready_to_publish = false;

            if (computed_first_fingerprint_timestamp == false) {
                computed_first_fingerprint_timestamp = true;
                last_fingerprint_timestamp = fingerprint_timestamp; // Set the first fingerprint timestamp
                ready_to_publish = true;
            }
            else { // first fingerprint already published, check timestamps
                if((fingerprint_timestamp - last_fingerprint_timestamp) >= 0.500) {
                    ready_to_publish = true;
                }
            }  
            
            if (ready_to_publish == true) {
                int messageLength = strlen(fingerprintData); 
                char FingerPrintResult[150 + messageLength];
                char sendFingerprints[500 + messageLength];
                
                ready_to_publish_to_modbus = true;
                    
                float* spectralFeatures = conductor.getFingerprintsforModbus();
                iuModbusSlave.updateHoldingRegister(modbusGroups::MODBUS_STREAMING_SPECTRAL_FEATURES,FINGERPRINT_KEY_1_L,FINGERPRINT_13_H,spectralFeatures);

                snprintf(FingerPrintResult, 150 + messageLength, "{\"macID\":\"%s\",\"timestamp\": %lf,\"state\":\"%d\",\"accountId\":\"%s\",\"fingerprints\": %s }", m_macAddress.toString().c_str(),fingerprint_timestamp,ledManager.getOperationState(),"XXXAdmin",fingerprintData);
                    
                // if(loopDebugMode) {
                //     debugPrint("Published Fingerprints"); 
                //     char published_time_diff[50];
                //     sprintf(published_time_diff, "%lf", fingerprint_timestamp - last_fingerprint_timestamp);
                //     debugPrint(F("Published time diff : "), false); debugPrint(F(published_time_diff), true);
                // }               
                last_fingerprint_timestamp = fingerprint_timestamp; // update timestamp for next iterations
                if(m_streamingMode == StreamingMode::ETHERNET){
                    /* FingerPrintResult send over Ethernet Mode */
                    if(rbase64.encode(FingerPrintResult) == RBASE64_STATUS_OK) {
                        snprintf(sendFingerprints,messageLength+500,"{\"deviceId\":\"%s\",\"transport\":%d,\"messageType\":%d,\"payload\":\"%s\"}",  /* \"{\\\"macID\\\":\\\"%s\\\",\\\"timestamp\\\":%lf,\\\"state\\\":\\\"%d\\\",\\\"accountId\\\":\\\"%s\\\",\\\"fingerprints\\\":%s\"}\"}" , */
                        m_macAddress.toString().c_str(),0,1,rbase64.result());//m_macAddress.toString().c_str(),fingerprint_timestamp,ledManager.getOperationState(),"XXXAdmin",fingerprintData);
                        iuWiFi.write(sendFingerprints);
                        iuWiFi.write("\n");
                        //rbase64.encode(FingerPrintResult);
                        //debugPrint("Base64 -Fingerprint : ",false);
                        //debugPrint(rbase64.result());
                    
    
                    }else
                    {
                        debugPrint("base64-encoding failed, Please check");
                    }
                        
                        
                }else if(m_streamingMode == StreamingMode::WIFI || m_streamingMode == StreamingMode::WIFI_AND_BLE || m_streamingMode == StreamingMode::BLE)//iuWiFi.isAvailable() && iuWiFi.isWorking())   
                {   /* FingerPrintResult send over Wifi only */
                    //debugPrint("Wifi connected, ....",true);
                    // if(RawDataState::rawDataTransmissionInProgress == false && iuWiFi.isConnected())
                    // {    
                    //     iuWiFi.sendMSPCommand(MSPCommand::SEND_DIAGNOSTIC_RESULTS,FingerPrintResult );    
                    // }
                    if(StreamingMode::BLE && isBLEConnected()){    
                    iuBluetooth.write("Fingerprints Data : ");
                    iuBluetooth.write(FingerPrintResult);
                    }
                }
            }
            else { // not published as time_diff < 500 ms
                // if(loopDebugMode) {
                //     char last_fingerprint_timestamp_string[50];
                //     char fingerprint_timestamp_string[50];
                //     char discarded_time_diff[50];
                //     sprintf(last_fingerprint_timestamp_string, "%lf", last_fingerprint_timestamp);
                //     sprintf(fingerprint_timestamp_string, "%lf", fingerprint_timestamp);
                //     sprintf(discarded_time_diff, "%lf", fingerprint_timestamp - last_fingerprint_timestamp);
                //     debugPrint(F("Fingerprint discarded as time diff < 500, time diff: "),false); debugPrint(F(discarded_time_diff), true);
                //     debugPrint(F("Last fingerprint timestamp: "), false); debugPrint(F(last_fingerprint_timestamp_string), true);
                //     debugPrint(F("Fingerprint timestamp: "), false); debugPrint(F(fingerprint_timestamp_string), true);
                // }                
            }   
    }
    else {        
        //debugPrint(F("Fingerprints have not been configured."), true);
        //getFingerprintsforModbus(); // Only to flush the fingerprint buffer used for modbus
        ready_to_publish_to_modbus = false;
        iuModbusSlave.clearHoldingRegister(modbusGroups::MODBUS_STREAMING_SPECTRAL_FEATURES,FINGERPRINT_KEY_1_L,FINGERPRINT_13_H);
    }   
}

bool Conductor::setFFTParams() {
    bool configured = false;
    JsonObject& config = configureJsonFromFlash("/iuconfig/fft.conf", false);
    if(config.success()) {
        FFTConfiguration::currentSamplingRate = config["samplingRate"];
        FFTConfiguration::currentBlockSize = config["blockSize"];
        //FFTConfiguration::currentSensor = config["sensor"];
        for (int i=0;i<FFTConfiguration::LSMsamplingRateOption;i++)
        {
            if (FFTConfiguration::currentSamplingRate == FFTConfiguration::samplingRates[i])
            {
                FFTConfiguration::currentSensor = FFTConfiguration::lsmSensor;
            }
        }

        for (int i=0;i<FFTConfiguration::KNXsamplingRateOption;i++)
        {
            if (FFTConfiguration::currentSamplingRate == FFTConfiguration::samplingRates2[i])
            {
                FFTConfiguration::currentSensor = FFTConfiguration::kionixSensor;
            }
        }
                // Change the sensor sampling rate 
        // timerISRPeriod = (samplingRate == 1660) ? 600 : 300;  // 1.6KHz->600, 3.3KHz->300
        // timerISRPeriod = int(1000000 / FFTConfiguration::currentSamplingRate); // +1 to ensure that sensor has captured data before mcu ISR gets it, for edge case
        if ((FFTConfiguration::currentSensor == FFTConfiguration::lsmSensor) && (iuAccelerometer.lsmPresence))
        {
            debugPrint(F("LSM Present & LSM set"));
            iuAccelerometer.setSamplingRate(FFTConfiguration::currentSamplingRate);
            FFTConfiguration::currentLowCutOffFrequency = FFTConfiguration::DEFALUT_LOW_CUT_OFF_FREQUENCY_LSM;
            setSensorStatus(SensorStatusCode::LSM_SET);
            if(config.containsKey("grange")){
                FFTConfiguration::currentLSMgRange = config["grange"];
            }else{
                FFTConfiguration::currentLSMgRange = FFTConfiguration::DEFAULT_LSM_G_RANGE;
            }
            iuAccelerometer.setGrange(FFTConfiguration::currentLSMgRange);
        }
        else if((FFTConfiguration::currentSensor == FFTConfiguration::kionixSensor) && (iuAccelerometerKX222.kionixPresence))
        {
            debugPrint(F("KIONIX Present & KIONIX set"));
            iuAccelerometerKX222.setSamplingRate(FFTConfiguration::currentSamplingRate); // will set the ODR for the sensor
            FFTConfiguration::currentLowCutOffFrequency = FFTConfiguration::DEFALUT_LOW_CUT_OFF_FREQUENCY_KNX;
            setSensorStatus(SensorStatusCode::KNX_SET);
            if(config.containsKey("grange")){
                FFTConfiguration::currentKNXgRange = config["grange"];
            }else{
                FFTConfiguration::currentKNXgRange = FFTConfiguration::DEFAULT_KNX_G_RANGE;
            }
            iuAccelerometerKX222.setGrange(FFTConfiguration::currentKNXgRange);
        }else if((FFTConfiguration::currentSensor == FFTConfiguration::lsmSensor) && (!iuAccelerometer.lsmPresence) && (iuAccelerometerKX222.kionixPresence)){
            debugPrint(F("LSM absent & KIONIX set"));
            iuAccelerometerKX222.setSamplingRate(iuAccelerometerKX222.defaultSamplingRate);
            FFTConfiguration::currentSamplingRate = iuAccelerometerKX222.defaultSamplingRate;
            FFTConfiguration::currentSensor = FFTConfiguration::kionixSensor;
            FFTConfiguration::currentBlockSize = FFTConfiguration::DEFAULT_BLOCK_SIZE;
            FFTConfiguration::currentLowCutOffFrequency = FFTConfiguration::DEFALUT_LOW_CUT_OFF_FREQUENCY_KNX;
            setSensorStatus(SensorStatusCode::LSM_ABS);
            FFTConfiguration::currentKNXgRange = FFTConfiguration::DEFAULT_KNX_G_RANGE;
            iuAccelerometerKX222.setGrange(FFTConfiguration::currentKNXgRange);
        }else if((FFTConfiguration::currentSensor == FFTConfiguration::kionixSensor) && (!iuAccelerometerKX222.kionixPresence) && (iuAccelerometer.lsmPresence)){
            debugPrint(F("KIONIX Absent & LSM set"));
            iuAccelerometer.setSamplingRate(iuAccelerometer.defaultSamplingRate);
            FFTConfiguration::currentSamplingRate = iuAccelerometer.defaultSamplingRate;
            FFTConfiguration::currentSensor = FFTConfiguration::lsmSensor;
            FFTConfiguration::currentBlockSize = FFTConfiguration::DEFAULT_BLOCK_SIZE;
            FFTConfiguration::currentLowCutOffFrequency = FFTConfiguration::DEFALUT_LOW_CUT_OFF_FREQUENCY_LSM;
            setSensorStatus(SensorStatusCode::KNX_ABS);
            FFTConfiguration::currentLSMgRange = FFTConfiguration::DEFAULT_LSM_G_RANGE;
            iuAccelerometer.setGrange(FFTConfiguration::currentLSMgRange);
        }
        else{
            debugPrint(F("KIONIX, LSM not found"));
            setSensorStatus(SensorStatusCode::SEN_ABS);
        }
        // // TODO: The following can be configurable in the future
        // // FFTConfiguration::currentLowCutOffFrequency = FFTConfiguration::DEFALUT_LOW_CUT_OFF_FREQUENCY;
        // FFTConfiguration::currentHighCutOffFrequency = FFTConfiguration::currentSamplingRate / FMAX_FACTOR;
        // FFTConfiguration::currentMinAgitation = FFTConfiguration::DEFAULT_MIN_AGITATION;

        // // Change the required sectionCount for all FFT processors 
        // // Update the lowCutFrequency and highCutFrequency for each FFTComputerID
        // int FFTComputerID = 30;  // FFTComputers X, Y, Z have m_id = 0, 1, 2 correspondingly
        // for (int i=0; i<3; ++i) {
        //     FeatureComputer::getInstanceById(FFTComputerID + i)->updateSectionCount(FFTConfiguration::currentBlockSize / 128);
        //     FeatureComputer::getInstanceById(FFTComputerID + i)->updateFrequencyLimits(FFTConfiguration::currentLowCutOffFrequency, FFTConfiguration::currentHighCutOffFrequency);
        // }        

        // // Update the parameters of the diagnostic engine
        // DiagnosticEngine::m_SampleingFrequency = FFTConfiguration::currentSamplingRate;
        // DiagnosticEngine::m_smapleSize = FFTConfiguration::currentBlockSize;
        // DiagnosticEngine::m_fftLength = FFTConfiguration::currentBlockSize / 2;

        if(setupDebugMode) {
            config.prettyPrintTo(Serial);
        }
        configured = true;
    } else {
        if(iuAccelerometerKX222.kionixPresence){
                conductor.setSensorStatus(conductor.SensorStatusCode::KNX_DEFAULT); // TO DO based on hardware identifier
                FFTConfiguration::currentSamplingRate = iuAccelerometerKX222.defaultSamplingRate;
                FFTConfiguration::currentBlockSize = FFTConfiguration::DEFAULT_BLOCK_SIZE;
                FFTConfiguration::currentLowCutOffFrequency = FFTConfiguration::DEFALUT_LOW_CUT_OFF_FREQUENCY_KNX;
                FFTConfiguration::currentKNXgRange = FFTConfiguration::DEFAULT_KNX_G_RANGE;
                iuAccelerometerKX222.setGrange(FFTConfiguration::currentKNXgRange);
            }else if(iuAccelerometer.lsmPresence){
                conductor.setSensorStatus(conductor.SensorStatusCode::LSM_DEFAULT); // TO DO based on hardware identifier
                FFTConfiguration::currentSamplingRate = iuAccelerometer.defaultSamplingRate;
                FFTConfiguration::currentBlockSize = FFTConfiguration::DEFAULT_BLOCK_SIZE;
                FFTConfiguration::currentLowCutOffFrequency = FFTConfiguration::DEFALUT_LOW_CUT_OFF_FREQUENCY_LSM;
                FFTConfiguration::currentLSMgRange = FFTConfiguration::DEFAULT_LSM_G_RANGE;
                iuAccelerometer.setGrange(FFTConfiguration::currentLSMgRange);
            }else{
                conductor.setSensorStatus(conductor.SensorStatusCode::SEN_ABS);
            }
        if(loopDebugMode) debugPrint("Failed to read fft.conf file. Configured with defaults");
        configured = false;
    }
    // TODO: The following can be configurable in the future
        // FFTConfiguration::currentLowCutOffFrequency = FFTConfiguration::DEFALUT_LOW_CUT_OFF_FREQUENCY;
        FFTConfiguration::currentHighCutOffFrequency = FFTConfiguration::currentSamplingRate / FMAX_FACTOR;
        FFTConfiguration::calculatedSamplingRate = FFTConfiguration::currentSamplingRate;
        FFTConfiguration::currentMinAgitation = FFTConfiguration::DEFAULT_MIN_AGITATION;
        
        // Change the required sectionCount for all FFT processors 
        // Update the lowCutFrequency and highCutFrequency for each FFTComputerID
        int FFTComputerID = 30;  // FFTComputers X, Y, Z have m_id = 0, 1, 2 correspondingly
        for (int i=0; i<3; ++i) {
            FeatureComputer::getInstanceById(FFTComputerID + i)->updateSectionCount(FFTConfiguration::currentBlockSize / 128);
            FeatureComputer::getInstanceById(FFTComputerID + i)->updateFrequencyLimits(FFTConfiguration::currentLowCutOffFrequency, FFTConfiguration::currentHighCutOffFrequency);
        }        

        // Update the parameters of the diagnostic engine
        DiagnosticEngine::m_SampleingFrequency = FFTConfiguration::currentSamplingRate;
        DiagnosticEngine::m_smapleSize = FFTConfiguration::currentBlockSize;
        DiagnosticEngine::m_fftLength = FFTConfiguration::currentBlockSize / 2;
    return configured;
}

bool Conductor::configureRPM(JsonVariant &config){
    bool success = false;    
    if (config.success())
    {
        int LOW_RPM  = config["LOW_RPM"];
        int HIGH_RPM = config["HIGH_RPM"];
        float RPM_TRH = config["RPM_TRH"]; 
        const char* messageId = config["messageId"];
           
        if(loopDebugMode){
            debugPrint("LOW RPM : ",false);
            debugPrint(LOW_RPM);
            debugPrint("HIGH RPM : ",false);
            debugPrint(HIGH_RPM);
            debugPrint("RPM_TRH :",false);
            debugPrint(RPM_TRH);
        }
        if(LOW_RPM < FFTConfiguration::currentLowCutOffFrequency || LOW_RPM > FFTConfiguration::currentHighCutOffFrequency ){
            LOW_RPM = FFTConfiguration::currentLowCutOffFrequency;
        }
        if (HIGH_RPM > FFTConfiguration::currentHighCutOffFrequency || HIGH_RPM < FFTConfiguration::currentLowCutOffFrequency)
        { 
            HIGH_RPM = FFTConfiguration::currentHighCutOffFrequency;
        }
        FFTConfiguration::currentLowRPMFrequency = LOW_RPM;
        FFTConfiguration::currentHighRPMFrequency = HIGH_RPM;
        if(RPM_TRH < 0 || RPM_TRH == 0){
            FFTConfiguration::currentRPMThreshold = FFTConfiguration::DEFAULT_RPM_THRESHOLD;
        }else{
            FFTConfiguration::currentRPMThreshold = RPM_TRH;
        }
        success = true;
    }
    
       
  return success;

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
    if (debugMode) {
        debugPrint("VDDA = ", false);
        debugPrint(vdda);
        debugPrint("MCU Temp = ", false);
        debugPrint(temperature);
    }
    size_t len = strlen(destination);
    for (size_t i = 0; i < len; i++) {
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
    port->println();
}

/**
 * Expose current configurations
 */
void Conductor::exposeAllConfigurations()
{
    #ifdef IUDEBUG_ANY
    for (uint8_t i = 0; i < Sensor::instanceCount; ++i) {
        Sensor::instances[i]->expose();
    }
    debugPrint("");
    for (uint8_t i = 0; i < Feature::instanceCount; ++i) {
        Feature::instances[i]->exposeConfig();
        Feature::instances[i]->exposeCounters();
        debugPrint("_____");
    }
    debugPrint("");
    for (uint8_t i = 0; i < FeatureComputer::instanceCount; ++i) {
        FeatureComputer::instances[i]->exposeConfig();
    }
    #endif
}

void Conductor::printConductorMac() {
    debugPrint("BLE MAC ADDRESS SET IN CONDUCTOR : ",false); debugPrint(m_macAddress.toString());
}

void Conductor::removeChar(char * New_BLE_MAC_Address, int charToRemove){ 

  int j, n = strlen(New_BLE_MAC_Address); 
  for (int i=j=0; i<n; i++) 
  if (New_BLE_MAC_Address[i] != charToRemove) 
    New_BLE_MAC_Address[j++] = New_BLE_MAC_Address[i]; 
  New_BLE_MAC_Address[j] = '\0'; 
}

void Conductor::setConductorMacAddress() {
    if(iuBluetooth.isBLEAvailable){
        iuBluetooth.enterATCommandInterface();
        char BLE_MAC_Address[20];
        char New_BLE_MAC_Address[13];
        uint8_t retryCount = 5;
        int mac_Response = iuBluetooth.sendATCommand("mac?", BLE_MAC_Address, 20);
        debugPrint("BLE MAC ID:",false);debugPrint(BLE_MAC_Address,true);
        strncpy(New_BLE_MAC_Address, BLE_MAC_Address + 6,11);
        removeChar(New_BLE_MAC_Address, ':');
        iuBluetooth.setDeviceName(New_BLE_MAC_Address);
        iuBluetooth.queryDeviceName();
        //debugPrint("SET MAC RESPONSE :",false);
        //debugPrint(mac_Response);
        if( mac_Response < 0 || (BLE_MAC_Address[0] != '9' /*&& BLE_MAC_Address[1] == '0' */) ){
            
            // Retry to get BLE MAC
            for (size_t i = 0; i < retryCount; i++)
            {
                // flushed the BLE_MAC_Address buffer
                memset(BLE_MAC_Address,0,20);

                int mac_Response = iuBluetooth.sendATCommand("mac?", BLE_MAC_Address, 20);
                if(debugMode){
                    debugPrint("RetryCount:",false);debugPrint(i);
                    debugPrint("BLE MAC ID IN RETRY : ",false);
                    debugPrint(BLE_MAC_Address);
                }                    
                if(mac_Response > 0 && ( BLE_MAC_Address[0] == '9')){
                    if(debugMode){
                        debugPrint("Found the BLE MAC ADDRESS");
                    }
                    break;
                 }     
                if(i>=2){
                    // RESET the Device   
                    if(debugMode){
                        debugPrint("RESET THE DEVICE All Retries Failed.");
                    }
                    delay(1000);
                    DOSFS.end();
                    delay(10);
                    STM32.reset();
                }  
                
            }
            // Success
        }
        
        iuBluetooth.exitATCommandInterface();
        m_macAddress.fromString(BLE_MAC_Address);
    }else
    {   //set the macAddress from Ethernet Module
        m_macAddress.fromString(iuEthernet.m_ethernetMacAddress);
    }
    
}

/* =============================================================================
    Segmented Message
============================================================================= */

void Conductor::extractPayloadFromSegmentedMessage(const char* segment, char* payload) {
    // payload has to be a c string with  maximum length of 16 characters

    // For segmentIndex = 0, segment will be "M/m | <messageID> |0| <payload>" 
    // This '0' at segment[2] will be treated as null byte by strlen() Method
    // Workaround - first 2 bytes are messageID and segmentCount, 
    // Length = str(payload) + 2 for messageID, segmentCount
    
    int index = 0;
    int segmentLength = strlen(&segment[3]) + 2;
    while ((index+3) <= segmentLength) {    // here, segment does not have ";" as the last char, copy all the bytes
        payload[index] = segment[index+3];
        index++;
    }
    payload[index] = '\0';  // make it c_string to allow string functions and debugPrint
}

bool Conductor::checkMessageActive(int messageID) {
    bool messageActive = false;
    if ((segmentedMessages[messageID].messageID != -1) && (segmentedMessages[messageID].messageID < MAX_SEGMENTED_MESSAGES)) 
        { messageActive = true; }
    return messageActive;
}

void Conductor::processSegmentedMessage(const char* buff) {
    // buff will contain maximum 20 bytes, message terminated with ';'

    // processSegmentedMessage() will be called when buff[0] == 'm' or 'M'
    // 'M' -> control message
    // 'm' -> data message

    if (buff[0] == 'M') {

        // Control messages - M | <messageID> | <segmentCount> | INIT / FINISHED | ;
        // M - 1 byte
        // <messageID> - 1 byte, value ranges from 0 to 127, both inclusive
        // <segmentCount> - 1 byte, value ranges from 0 to 127, both inclusive
        // INIT / FINISHED/ - messageType
        // ; - sentinel character, 1 byte

        char messageType[PAYLOAD_LENGTH+1];
        extractPayloadFromSegmentedMessage(buff, messageType);       
        #ifdef IU_DEBUG_SEGMENTED_MESSAGES 
        debugPrint("DEBUG: M: messageType: ", false); debugPrint(messageType);
        #endif

        if (strncmp(messageType, "INIT", 4) == 0) {
            
            #ifdef IU_DEBUG_SEGMENTED_MESSAGES
            debugPrint("DEBUG: M: processing INIT message");
            #endif

            if (int(buff[1]) >= MAX_SEGMENTED_MESSAGES) { 
                #ifdef IU_DEBUG_SEGMENTED_MESSAGES 
                debugPrint("ERROR: M: INIT: messageID: ", false); debugPrint(int(buff[1]));
                debugPrint("ERROR: M: INIT: messageID exceeded MAX_SEGMENTED_MESSAGES");
                #endif
                // While processing INIT, if messageID exceeds MAX_SEGMENTED_MESSAGES, then let the rest of the transmission
                // continue with ERRORS in the logs, a response will be sent when a FINISHED message is received
                return;
            }

            if (int(buff[2]) > MAX_NUMBER_OF_SEGMENTS_PER_MESSAGE) {  
                #ifdef IU_DEBUG_SEGMENTED_MESSAGES
                debugPrint("ERROR: M: INIT: segmentCount: ", false); debugPrint(int(buff[2]));
                debugPrint("ERROR: M: INIT: segmentCount exceeded MAX_NUMBER_OF_SEGMENTS_PER_MESSAGE");
                #endif
                // While processing INIT, if segmentCount exceeds MAX_NUMBER_OF_SEGMENTS_PER_MESSAGE, then let the rest of the transmission
                // continue with ERRORS in the logs, a response will be sent when a FINISHED message is received
                return;
            }

            int messageID = int(buff[1]);  // setting messageID indicates message is now active i.e. data segment transmission is on-going
            int segmentCount = int(buff[2]);
            int startTimestamp = millis();
            int timeout = (segmentCount + 2) * 500 + TIMEOUT_OFFSET; // 2 added for hash message and FINISHED message

            segmentedMessages[messageID].messageID = messageID;
            segmentedMessages[messageID].segmentCount = segmentCount;
            segmentedMessages[messageID].startTimestamp = startTimestamp;
            segmentedMessages[messageID].timeout = timeout;

            #ifdef IU_DEBUG_SEGMENTED_MESSAGES
            debugPrint("DEBUG: M: INIT: messageID: ", false); debugPrint(segmentedMessages[messageID].messageID);
            debugPrint("DEBUG: M: INIT: segmentCount: ", false); debugPrint(segmentedMessages[messageID].segmentCount);
            debugPrint("DEBUG: M: INIT: startTimestamp: ", false); debugPrint(segmentedMessages[messageID].startTimestamp);   
            #endif
        }   
        
        else if (strncmp(messageType, "FINISHED", 8) == 0) {
            #ifdef IU_DEBUG_SEGMENTED_MESSAGES
            debugPrint("DEBUG: M: FINISHED: processing FINISHED message");
            #endif IU_DEBUG_SEGMENTED_MESSAGES

            int messageID = int(buff[1]);    
            int segmentCount = int(buff[2]);

            // Send BLE failure response for messageID >= MAX_SEGMENTED_MESSAGES and return
            if (messageID >= MAX_SEGMENTED_MESSAGES) { 
                #ifdef IU_DEBUG_SEGMENTED_MESSAGES 
                debugPrint("ERROR: M: FINISHED: messageID: ", false); debugPrint(messageID);
                debugPrint("ERROR: M: FINISHED: messageID exceeded MAX_SEGMENTED_MESSAGES");   
                #endif             
                char finishedResponse[21];
                char finishedFailure[] = "FAILURE-MSGID;"; 
                for (int i=0; i<3; i++) { finishedResponse[i] = buff[i]; }
                for (int i=0; i<strlen(finishedFailure); i++) { finishedResponse[i+3] = finishedFailure[i]; }
                finishedResponse[strlen(finishedFailure)+3] = '\0';
                #ifdef IU_DEBUG_SEGMENTED_MESSAGES
                debugPrint("DEBUG: M: FINSIHED: SENDING FAILURE-MSGID RESPONSE");
                #endif
                if (isBLEConnected()) {
                    iuBluetooth.write(finishedResponse);
                    #ifdef IU_DEBUG_SEGMENTED_MESSAGES
                    debugPrint("DEBUG: M: FINISHED: RESPONSE sent via BLE");
                    #endif IU_DEBUG_SEGMENTED_MESSAGES
                }                
                return;
            }

            // Send a BLE failure response if segmentCount >= MAX_NUMBER_OF_SEGMENTS_PER_MESSAGE and return
            if (segmentCount > MAX_NUMBER_OF_SEGMENTS_PER_MESSAGE) {
                #ifdef IU_DEBUG_SEGMENTED_MESSAGES
                debugPrint("ERROR: M: FINISHED: segmentCount: ", false); debugPrint(int(buff[1]));
                debugPrint("ERROR: M: FINISHED: segmentCount exceeded MAX_NUMBER_OF_SEGMENTS_PER_MESSAGE");
                #endif
                char finishedResponse[21];
                char finishedFailure[] = "FAILURE-SEGCNT;"; 
                for (int i=0; i<3; i++) { finishedResponse[i] = buff[i]; }
                for (int i=0; i<strlen(finishedFailure); i++) { finishedResponse[i+3] = finishedFailure[i]; }
                finishedResponse[strlen(finishedFailure)+3] = '\0';
                #ifdef IU_DEBUG_SEGMENTED_MESSAGES
                debugPrint("DEBUG: M: FINSIHED: SENDING FAILURE-SEGCNT RESPONSE");
                #endif
                if (isBLEConnected()) {
                        iuBluetooth.write(finishedResponse);
                        #ifdef IU_DEBUG_SEGMENTED_MESSAGES
                        debugPrint("DEBUG: M: FINISHED: RESPONSE sent via BLE");
                        #endif
                }
                return;
            }

            if (checkMessageActive(messageID)) {   // check if message is still active, it hasn't timed out 
                // check if all segments have been received and hashes match
                if(checkAllSegmentsReceived(messageID)) {
                    compileSegmentedMessage(messageID);
                    computeSegmentedMessageHash(messageID);
                    if(strncmp(segmentedMessages[messageID].computedHash, segmentedMessages[messageID].receivedHash, PAYLOAD_LENGTH) == 0) {
                        #ifdef IU_DEBUG_SEGMENTED_MESSAGES
                        debugPrint("DEBUG: M: FINISHED: message compiled and hashes verified");
                        #endif                       
                        // set messageReady to indicate message can be consumed
                        segmentedMessages[messageID].messageReady = true;
                        // set messageState for response that message is received successfully
                        segmentedMessages[messageID].messageState = SEGMENTED_MESSAGE_STATE::MESSAGE_SUCCESSFUL;                         
                    } else {
                        // set messageState for response that hash verification failed
                        segmentedMessages[messageID].messageState = SEGMENTED_MESSAGE_STATE::MESSAGE_HASH_VERIFICATION_FAILED;
                    }
                } else {
                    // set messageState for response that some segments have not been received
                    segmentedMessages[messageID].messageState = SEGMENTED_MESSAGE_STATE::MESSAGE_SEGMENTS_MISSING;
                }

                // send a response over BLE
                sendSegmentedMessageResponse(messageID);

                // if the message failed, then clean the failed message
                cleanFailedSegmentedMessage(messageID);

            }
            else {
                #ifdef IU_DEBUG_SEGMENTED_MESSAGES
                debugPrint("ERROR: M: Last transmission attempt timed out or INIT not received, retry transmission");
                #endif
                // Last transmission attempt timed out, after which the message container was reset and made inactive
                // or INIT was not received, so no message in segmentedMessages was made active to store incoming segments                
                
                // set messageState for response that last transmission timed out
                segmentedMessages[messageID].messageState = SEGMENTED_MESSAGE_STATE::MESSAGE_TIMED_OUT;
                
                // here, segmentCount will be 0 by default, which, in the response message will be  M|<messageID>|0|FINISHED-TIMEDOUT|;
                // this segmentCount will be considered as null byte by strelen() in iuBluetooth.write(), becausbe of which BLE message won't sent
                // to avoid this, we set segmentCount to '-', since this segmentCount will not be used anyway.
                segmentedMessages[messageID].segmentCount = '-';

                // send a response over BLE
                sendSegmentedMessageResponse(messageID);
            }
        }
    }
    else {
        // buff[0] == 'm'
        // Data message - m | <messageID> | <segmentIndex> | __<dataPayload>__ | ;
        // m - 1 byte
        // <messageID> - 1 byte, ranges from 0 to 127
        // <segmentIndex> - 1 byte, ranges from 0 to 126 (maximum segmentCount is 127, segmentIndex will be [0, segmentCount-1])
        // __<dataPayload>__ - upto 16 bytes of message segment
        // ; - 1 byte sentinel character

        int messageID = int(buff[1]);
        if (int(buff[1]) >= MAX_SEGMENTED_MESSAGES) {
            #ifdef IU_DEBUG_SEGMENTED_MESSAGES 
            debugPrint("ERROR: m: messageID: ", false); debugPrint(int(buff[1]));
            debugPrint("ERROR: m: messageID exceeded MAX_SEGMENTED_MESSAGES");
            #endif
            return;
        }
        #ifdef IU_DEBUG_SEGMENTED_MESSAGES
        debugPrint("DEBUG: m: messageID: ", false); debugPrint(messageID);
        #endif

        int segmentIndex = int(buff[2]);
        if (segmentIndex >= MAX_NUMBER_OF_SEGMENTS_PER_MESSAGE && segmentIndex != 127) { 
            // segmentIndex can never be 127 ->  0 < segmentIndex <MAX_NUMBER_OF_SEGMENTS_PER_MESSAGE, as upper bound for MAX_NUMBER_OF_SEGMENTS_PER_MESSAGE = 127
            // 127 is reserved for HASH message
            #ifdef IU_DEBUG_SEGMENTED_MESSAGES
            debugPrint("ERROR: m: segmentIndex: ", false);  debugPrint(segmentIndex);
            debugPrint("ERROR: m: segmentIndex exceeded MAX_NUMBER_OF_SEGMENTS_PER_MESSAGE");
            #endif
            return;
        }
        if (segmentIndex >= segmentedMessages[messageID].segmentCount && segmentIndex != 127) { 
            // 0 <= segmentIndex < segmentCount
            #ifdef IU_DEBUG_SEGMENTED_MESSAGES
            debugPrint("ERROR: m: segmentIndex: ", false);  debugPrint(segmentIndex);
            debugPrint("ERROR: m: segmentIndex exceeded segmentCount");
            #endif
            return;
        }
        #ifdef IU_DEBUG_SEGMENTED_MESSAGES
        debugPrint("DEBUG: m: segmentIndex: ", false); debugPrint(segmentIndex);
        #endif

        if(checkMessageActive(messageID)) {
            if(segmentIndex != 127) {
                // add the data segment
                char payload[PAYLOAD_LENGTH+1];
                extractPayloadFromSegmentedMessage(buff, payload);
                #ifdef IU_DEBUG_SEGMENTED_MESSAGES
                debugPrint("DEBUG: m: payload: ", false); debugPrint(payload);
                #endif
                strcpy(segmentedMessages[messageID].dataSegments[segmentIndex], payload);
                segmentedMessages[messageID].dataSegments[segmentIndex][strlen(payload)] = '\0'; // TODO: might not be needed as strcpy is used
                #ifdef IU_DEBUG_SEGMENTED_MESSAGES
                debugPrint("DEBUG: m: added payload to segmentedMessage: ", false); debugPrint(segmentedMessages[messageID].dataSegments[segmentIndex]);
                #endif

                // update the bool vector tracking received segmentedMessage
                segmentedMessages[messageID].receivedSegments[segmentIndex] = true;
            }
            else { // segmentIndex value is 127, indicating it's the HASH message
                // save the received hash 
                char receivedHash[PAYLOAD_LENGTH+1];
                extractPayloadFromSegmentedMessage(buff, receivedHash);
                strcpy(segmentedMessages[messageID].receivedHash, receivedHash);
                #ifdef IU_DEBUG_SEGMENTED_MESSAGES
                debugPrint("DEBUG: m: processed HASH message, receivedHash: ", false); debugPrint(segmentedMessages[messageID].receivedHash);
                #endif IU_DEBUG_SEGMENTED_MESSAGES
            }
            #ifdef IU_DEBUG_SEGMENTED_MESSAGES            
            debugPrint("DEBUG: m: processed segmentIndex: ", false); debugPrint(segmentIndex);      
            #endif              
        }
        else {
            #ifdef IU_DEBUG_SEGMENTED_MESSAGES
            debugPrint("ERROR: m: INIT not received, retry transmission");
            #endif
            // In case INIT is not received, a FAILURE resopnse will be sent for the FINISHED message after all 'm' messages 
        }
            
    }
    #ifdef IU_DEBUG_SEGMENTED_MESSAGES
    debugPrint("DEBUG: processSegmentedMessage returning");
    #endif
}

bool Conductor::checkAllSegmentsReceived(int messageID) {
    // check if all segments have been received 
    bool allSegmentsReceived = true;
    #ifdef IU_DEBUG_SEGMENTED_MESSAGES
    debugPrint("DEBUG: in checkAllSegmentsReceived(): segmentCount: ", false); debugPrint(segmentedMessages[messageID].segmentCount);
    #endif
    for (int segmentIndex=0; segmentIndex<segmentedMessages[messageID].segmentCount; segmentIndex++) {
        if (segmentedMessages[messageID].receivedSegments[segmentIndex] != true) {
            #ifdef IU_DEBUG_SEGMENTED_MESSAGES
            debugPrint("DEBUG: in checkAllSegmentsReceived(): missing segmentIndex: ", false); debugPrint(segmentIndex);
            #endif
            allSegmentsReceived = false;
            break;
        }   
    }
    #ifdef IU_DEBUG_SEGMENTED_MESSAGES
    debugPrint("DEBUG: in checkAllSegmentsReceived(): all segments recevied: ", false); debugPrint(allSegmentsReceived);
    #endif
    return allSegmentsReceived;
}

void Conductor::compileSegmentedMessage(int messageID) {
    // compile message from the segments
    for (int segmentIndex=0; segmentIndex < segmentedMessages[messageID].segmentCount; segmentIndex++) {
        #ifdef IU_DEBUG_SEGMENTED_MESSAGES
        debugPrint("DEBUG: in compileSegmentedMessages(): segmentIndex: ", false); debugPrint(segmentIndex, false);
        debugPrint(" segmentData: ", false); debugPrint(segmentedMessages[messageID].dataSegments[segmentIndex]);
        #endif
        strcat(segmentedMessages[messageID].message, segmentedMessages[messageID].dataSegments[segmentIndex]);
    }
    #ifdef IU_DEBUG_SEGMENTED_MESSAGES
    debugPrint("DEBUG: in compileSegmentedMessages(): message compiled: ", false); debugPrint(segmentedMessages[messageID].message);
    #endif
}

void Conductor::computeSegmentedMessageHash(int messageID) {
    // message has to be successfully compiled before calling this method
    unsigned char* md5hash = MD5::make_hash(segmentedMessages[messageID].message, strlen(segmentedMessages[messageID].message));
    memcpy(segmentedMessages[messageID].computedHash, MD5::make_digest(md5hash, 16), PAYLOAD_LENGTH);
    segmentedMessages[messageID].computedHash[PAYLOAD_LENGTH] = '\0';
    #ifdef IU_DEBUG_SEGMENTED_MESSAGES
    debugPrint("DEBUG: in computeSegmentedMessageHash(): message: ", false); debugPrint(segmentedMessages[messageID].message);
    debugPrint("DEBUG: in computeSegmentedMessageHash(): computedHash: ", false); debugPrint(segmentedMessages[messageID].computedHash);
    #endif
}

void Conductor::cleanSegmentedMessage(int messageID) {
    // resets all data members, message reverts to inactive state
    segmentedMessages[messageID].messageID = -1;
    segmentedMessages[messageID].segmentCount = 0;
    for (int i=0; i<MAX_NUMBER_OF_SEGMENTS_PER_MESSAGE; i++) {  }
    for (int i=0; i<MAX_NUMBER_OF_SEGMENTS_PER_MESSAGE; i++) {
        segmentedMessages[messageID].receivedSegments[i] = false;
        strcpy(segmentedMessages[messageID].dataSegments[i], "");
    }
    segmentedMessages[messageID].startTimestamp = 0;
    segmentedMessages[messageID].timeout = 0;   
    strcpy(segmentedMessages[messageID].message, ""); 
    strcpy(segmentedMessages[messageID].receivedHash, "");
    strcpy(segmentedMessages[messageID].computedHash, "");
    segmentedMessages[messageID].messageReady = false;
    segmentedMessages[messageID].messageConsumed = false;
    segmentedMessages[messageID].messageState = -1;
}

void Conductor::cleanTimedoutSegmentedMessages() {
    for (int messageID=0; messageID<MAX_SEGMENTED_MESSAGES; messageID++) {
        if (checkMessageActive(messageID)) {
            int timeDiff = millis() - segmentedMessages[messageID].startTimestamp;;
            if  (timeDiff > segmentedMessages[messageID].timeout) {
                #ifdef IU_DEBUG_SEGMENTED_MESSAGES 
                debugPrint("DEBUG: Conductor::cleanTimedoutSegmentedMessages(): timeDiff: ", false); debugPrint(timeDiff, false);
                debugPrint(" for messageID: ", false);  debugPrint(messageID, false);
                debugPrint(" exceeded timeout: ", false); debugPrint(segmentedMessages[messageID].timeout);
                #endif
                cleanSegmentedMessage(messageID);
            }
        }        
    }
}

void Conductor::cleanConsumedSegmentedMessages() {
    // this method will be called in main loop(), right after consuming the ready message
    for (int messageID=0; messageID<MAX_SEGMENTED_MESSAGES; messageID++) {
        if (checkMessageActive(messageID) && segmentedMessages[messageID].messageConsumed) {
            #ifdef IU_DEBUG_SEGMENTED_MESSAGES
            debugPrint("DEBUG: Conductor::cleanConsumedSegmentedMessages(): messageID: ", false); debugPrint(messageID);
            #endif
            cleanSegmentedMessage(messageID);
        }
    }
}

void Conductor::cleanFailedSegmentedMessage(int messageID) {
    if(checkMessageActive(messageID)) {
        if (segmentedMessages[messageID].messageState == SEGMENTED_MESSAGE_STATE::MESSAGE_HASH_VERIFICATION_FAILED || 
            segmentedMessages[messageID].messageState == SEGMENTED_MESSAGE_STATE::MESSAGE_SEGMENTS_MISSING || 
            segmentedMessages[messageID].messageState == SEGMENTED_MESSAGE_STATE::MESSAGE_TIMED_OUT) {
                #ifdef IU_DEBUG_SEGMENTED_MESSAGES
                debugPrint("DEBUG: in cleanFailedSegmentedMessage: messageID: ", false); debugPrint(messageID, false);
                debugPrint(" messageState: ", false); debugPrint(segmentedMessages[messageID].messageState);
                #endif
                cleanSegmentedMessage(messageID);
        }
    }
}

bool Conductor::consumeReadySegmentedMessage(char* returnMessage) {
    // if message is ready, set messageConsumed to true and return the message
    // if multiple messages are ready, it will return the first ready message, and next ones will be returned in subsequent iterations

    bool readyMessageConsumed = false;
    for (int messageID=0; messageID<MAX_SEGMENTED_MESSAGES; messageID++) {
        if (checkMessageActive(messageID) && segmentedMessages[messageID].messageReady) {
            strcpy(returnMessage, segmentedMessages[messageID].message);
            segmentedMessages[messageID].messageConsumed = true;
            readyMessageConsumed = true;
            break;
        }
    } 
    return readyMessageConsumed;
}

void Conductor::sendSegmentedMessageResponse(int messageID) {

    char messageState[18];
    switch (segmentedMessages[messageID].messageState)
    {
    case SEGMENTED_MESSAGE_STATE::MESSAGE_SUCCESSFUL :
        strcpy(messageState, "SUCCESS;");
        break;
    case SEGMENTED_MESSAGE_STATE::MESSAGE_SEGMENTS_MISSING :
        strcpy(messageState, "FAILURE-MISSSGMT;");
        break;
    case SEGMENTED_MESSAGE_STATE::MESSAGE_HASH_VERIFICATION_FAILED :
        strcpy(messageState, "FAILURE-HASH;");
        break;
    case SEGMENTED_MESSAGE_STATE::MESSAGE_TIMED_OUT :
        strcpy(messageState, "FAILURE-TIMEDOUT;");
        break;
    }
    #ifdef IU_DEBUG_SEGMENTED_MESSAGES
    debugPrint("DEBUG: sendSegmentedMessageResponse: messageState: ", false);  debugPrint(messageState);
    #endif

    char finishedResponse[21];
    finishedResponse[0] = 'M';
    finishedResponse[1] = segmentedMessages[messageID].messageID;
    finishedResponse[2] = segmentedMessages[messageID].segmentCount;
    for (int i=0; i<strlen(messageState); i++) { finishedResponse[i+3] = messageState[i]; } 
    finishedResponse[strlen(messageState)+3] = '\0';
    
    if (isBLEConnected()) {
            iuBluetooth.write(finishedResponse);
            delay(1000);  // ensure response has been sent
            #ifdef IU_DEBUG_SEGMENTED_MESSAGES
            debugPrint("DEBUG: M: sendSegmentedMessageResponse: response sent via BLE");
            #endif
    }
}

bool Conductor::setThresholdsFromFile() 
{
    const size_t bufferSize = 6*JSON_ARRAY_SIZE(3) + 6*JSON_OBJECT_SIZE(2) + JSON_OBJECT_SIZE(3) + JSON_OBJECT_SIZE(6) + 972;
    StaticJsonBuffer<bufferSize> jsonBuffer;
    JsonVariant config = JsonVariant(
            iuFlash.loadConfigJson(IUFlash::CFG_FEATURE, jsonBuffer));
    bool configStatus = true;
    // config.prettyPrintTo(Serial);
    if (config.success()) {
        const char* threshold = "TRH";
        float low, mid, high;
        
        debugPrint("Current active group name is :",false);debugPrint(m_mainFeatureGroup->getName());
        for(uint8_t i=0;i<m_mainFeatureGroup->getFeatureCount();i++) {
            char featureName[Feature::nameLength + 1];
            strncpy(featureName, m_mainFeatureGroup->getFeature(i)->getName(), Feature::nameLength);
            featureName[Feature::nameLength]='\0';
            
            //To handle discrepency between TMP, TMA, TMB. The google MQTT broker sends "TMA" as 
            //the temperature key, but locally the temperature feature's name is "TMP"
            // see https://infinite-uptime.atlassian.net/browse/IF-18 
            if (strncmp(featureName, "TM", 2) == 0) {
                if (!config[featureName].success()) {   //local temperature name and JSON temperature name do not match
                    char* tempNames[3] = {"TMA", "TMB", "TMP"};
                    int tempCounter;
                    for (tempCounter = 0; tempCounter < 3; tempCounter++) {
                        if (config[tempNames[tempCounter]].success()) 
                            break;
                    }
                strcpy(featureName, tempNames[tempCounter]);    //use correct JSON key to set new thresholds     
                } 
            }
            low = config[featureName][threshold][0];
            mid = config[featureName][threshold][1];
            high = config[featureName][threshold][2];

            debugPrint("Setting thresholds for feature name: ",false);debugPrint(featureName,false);
            debugPrint(" low : ",false);debugPrint(low,false);
            debugPrint(" mid : ",false);debugPrint(mid,false);
            debugPrint(" high : ",false);debugPrint(high);

            opStateComputer.setThresholds(i, low, mid, high);

        }
        processLegacyCommand("6000000:1.1.1.1.1.1");            //ensure these features are activated
        computeFeatures();
        configStatus = true;                                      //compute current state with these thresholds
    }
    else {
        configStatus = false;
        debugPrint("Threshold file read was not successful.");
    }
    return configStatus;
}
//set the sensor Configuration

bool Conductor::setSensorConfig(char* filename){
    
    JsonObject& config = configureJsonFromFlash(filename,1);      
    JsonVariant variant = config;

    if (!config.success()) {
        if (debugMode) {
            debugPrint("parseObject() failed");
        }
        return false;
    }
    else{

        m_temperatureOffset = config["sensorConfig"]["TMP_OFFSET"];
        m_audioOffset = config["sensorConfig"]["SND_OFFSET"];
        if(loopDebugMode){
            debugPrint("Tempearature Offset: ",false);
            debugPrint(m_temperatureOffset);
            debugPrint("Audio Offset:",false);
            debugPrint(m_audioOffset);
        }    
        //variant.prettyPrintTo(Serial);
        
        return true;
    }
}

/**
 * @brief 
 * 
 * @param filename - Storage file 
 * @return true 
 * @return false 
 */
bool Conductor::setEthernetConfig(char* filename){
    JsonObject& config = configureJsonFromFlash(filename,1);      
    JsonVariant variant = config;

    if (!config.success()) {
        if (debugMode) {
            debugPrint("parseObject() failed for relayAgentConfig.conf");
        }
        return false;
    }
    else{
        // Read availabel relayAgentConfig details
        iuEthernet.m_workMode = config["relayAgentConfig"]["workMode"];
        iuEthernet.m_remoteIP = config["relayAgentConfig"]["remoteAddr"];
        iuEthernet.m_remotePort = config["relayAgentConfig"]["remotePort"];
        
        iuEthernet.m_enableHeartbeat = config["relayAgentConfig"]["enableHeartbeat"];
        iuEthernet.m_heartbeatInterval = config["relayAgentConfig"]["heartbeatInterval"];
        iuEthernet.m_heartbeatDir = config["relayAgentConfig"]["heartbeatDir"];
        iuEthernet.m_heartbeatMsg = config["relayAgentConfig"]["heartbeatMsg"];
        
        if(loopDebugMode){
            debugPrint("workMode: ",false);
            debugPrint(iuEthernet.m_workMode,true);
            debugPrint("remoteAddr:",false);
            debugPrint(iuEthernet.m_remoteIP,true);
            debugPrint("remotePort:",false);
            debugPrint(iuEthernet.m_remotePort,true);
            // Heartbeat configurations
            debugPrint("isHeartbeatEnabled:",false);
            debugPrint(iuEthernet.m_enableHeartbeat);
            debugPrint("Heartbeat Interval:",false);
            debugPrint(iuEthernet.m_heartbeatInterval);
            debugPrint("Hearbeat Direction:",false);
            debugPrint(iuEthernet.m_heartbeatDir);
            debugPrint("Heartbeat Message:",false);
            debugPrint(iuEthernet.m_heartbeatMsg);
        }    
        
        return true;
    }

    return false;
}

void Conductor::setSensorStatus(SensorStatusCode errorCode)
{
    statusCode = errorCode;
    switch (errorCode)
    {
        case SensorStatusCode::LSM_SET:
            FFTConfiguration::currentSensor = FFTConfiguration::lsmSensor;
            strcpy(status, "Running with LSM");
            break;
        case SensorStatusCode::KNX_SET:
            FFTConfiguration::currentSensor = FFTConfiguration::kionixSensor;
            strcpy(status, "Running with Kionix");
            break;
        case SensorStatusCode::LSM_ABS:
            FFTConfiguration::currentSensor = FFTConfiguration::kionixSensor;
            strcpy(status, "LSM not found, Running with Kionix defaults");
            break;
        case SensorStatusCode::KNX_ABS:
            FFTConfiguration::currentSensor = FFTConfiguration::lsmSensor;
            strcpy(status, "Kionix not found, Running with LSM defaults");
            break;
        case SensorStatusCode::SEN_ABS:
            strcpy(status, "Sensors not found, Please check the Hardware");
            break;
        case SensorStatusCode::KNX_DEFAULT:
            FFTConfiguration::currentSensor = FFTConfiguration::kionixSensor;
            strcpy(status, "Configuration not found, Using Kionix defaults");
            break;
        case SensorStatusCode::LSM_DEFAULT:
            FFTConfiguration::currentSensor = FFTConfiguration::lsmSensor;
            strcpy(status, "Configuration not found, Using LSM defaults");
            break;
    }
}

void Conductor::sendSensorStatus()
{
    char SensorResponse[256];
    double TimeStamp = conductor.getDatetime();            
    snprintf(SensorResponse, 256, "{\"deviceIdentifier\":\"%s\",\"type\":\"%s\",\"statusCode\":\"%d\",\"status\":\"%s\",\"timestamp\":%.2f}",
    m_macAddress.toString().c_str(), "vEdge", statusCode, status ,TimeStamp);
    iuWiFi.sendMSPCommand(MSPCommand::SEND_SENSOR_STATUS,SensorResponse);
}
/**
 * @brief 
 * This function checks for time out of OTA packet download. Once OTA is started and Packet reception
 * stopped from WiFi, then this function tineout shall abort the OTA process. 
 * @param None 
 * @return None 
  */
void Conductor::otaChkFwdnldTmout()
{
    uint32_t now = millis();
    if (waitingDnldStrart == true)
    {
        if (now - otaFwdnldTmout > fwDnldStartTmout)
        {
            waitingDnldStrart = false;
            for(int i = 0 ; i < 15; i++) {
                ledManager.overrideColor(RGB_RED);
                delay(200);
                ledManager.stopColorOverride();
                delay(200);
            }
            sendOtaStatusMsg(MSPCommand::OTA_FDW_ABORT,OTA_DOWNLOAD_ERR, String(iuOta.getOtaRca(OTA_DOWNLOAD_TMOUT)).c_str());
            if (loopDebugMode) { debugPrint(F("Switching Device mode:OTA -> OPERATION")); }
            iuWiFi.m_setLastConfirmedPublication();  // Download Timeout , No response
            changeUsageMode(UsageMode::OPERATION);
            delay(100);
            if (debugMode)
            {
                debugPrint("Exceeded OTA download start time-out");
            }
        }
    }
    if(otaInitTimeoutFlag == true) {
        if((now - otaInitWaitTimeout) > 3000)
        {
            otaInitTimeoutFlag = false;
            if (loopDebugMode) { debugPrint("OTA - Get MQTT Message timeout ! "); }
            conductor.sendOtaStatusMsg(MSPCommand::OTA_FDW_ABORT,OTA_DOWNLOAD_ERR, String(iuOta.getOtaRca(OTA_DOWNLOAD_TMOUT)).c_str());
            if (loopDebugMode) { debugPrint(F("Switching Device mode:OTA -> OPERATION")); }
            iuWiFi.m_setLastConfirmedPublication();
            conductor.changeUsageMode(UsageMode::OPERATION);
        }
    }
    // Certificates Init Timeout 
    if (certDownloadInProgress == true )
    {   
        if ( ((now - certDownloadInitWaitTimeout ) >= m_certDownloadInitTimeout/2 ) && m_certDownloadStarted != true && m_getDownloadConfig != true && sendCertInitAck == false)
        {
            sendOtaStatusMsg(MSPCommand::CERT_DOWNLOAD_INIT_ACK,CERT_DOWNLOAD_ACK, String(iuOta.getOtaRca(CERT_DOWNLOAD_INIT_REQ_ACK)).c_str());
            sendCertInitAck = true;
        }
        if ( ((now - certDownloadInitWaitTimeout ) > m_certDownloadInitTimeout ) && m_certDownloadStarted != true && m_getDownloadConfig != true && sendCertInitAck == true)
        {
            certDownloadInProgress = false;
            sendCertInitAck = false;
            if (loopDebugMode)
            {
                debugPrint("CERT - Init Request Timeout, not received next command");
            }
            certDownloadMode = false;
            if (loopDebugMode) { debugPrint(F("Switching Device mode:OTA/CERT -> OPERATION")); }
            iuWiFi.m_setLastConfirmedPublication();
            conductor.changeUsageMode(UsageMode::OPERATION);
        
        }
        
    }
    // Certificates Download Config Timeout 
    if (m_getDownloadConfig == true )
    {   
        if ( ((now - certDownloadConfigTimeout ) > m_certDownloadConfigTimeout ) && m_certDownloadStarted != true )
        {
            certDownloadInProgress = false;
            m_getDownloadConfig = false;
            if (loopDebugMode)
            {
                debugPrint("CERT - Download Config Request Timeout, not received next command");
            }
            if (loopDebugMode) { debugPrint(F("Switching Device mode:OTA/CERT -> OPERATION")); }
            iuWiFi.m_setLastConfirmedPublication();
            conductor.changeUsageMode(UsageMode::OPERATION);
        
        }
        
    }
    // if upgrade success response is not received before timeout , switch to Operation Mode
    if (m_downloadSuccess == true && m_upgradeSuccess == false)
    {
        if ( (now - m_downloadSuccessStartTime ) > 5000 )
        {
            if ((iuWiFi.isConnected() && m_upgradeSuccess)|| m_mqttConnected == true)
            {   
                certDownloadInProgress = false;
                m_downloadSuccess = false;
                // m_downloadSuccessStartTime = millis();
                // Certificate Upgrade went Successful
                ledManager.overrideColor(RGB_PURPLE);
                // sendOtaStatusMsg(MSPCommand::CERT_UPGRADE_SUCCESS,CERT_UPGRADE_COMPLETE,"CERT-RCA-0000");
                // TODO : Add Visuals
                // Show Visuals min 15 sec
                for(int i = 0 ; i < 20; i++) {
                    ledManager.overrideColor(RGB_GREEN);
                    delay(200);
                    ledManager.stopColorOverride();
                    delay(200);
                }
                if (loopDebugMode)
                {
                    debugPrint("CERT - Upgrade Success");
                }
                certDownloadMode = false;
                if (loopDebugMode) { debugPrint(F("Switching Device mode:OTA/CERT -> OPERATION")); }
                iuWiFi.m_setLastConfirmedPublication();
                conductor.changeUsageMode(UsageMode::OPERATION);

                iuWiFi.hardReset();

            }
                
        }
        if((now - m_downloadSuccessStartTime ) > m_upgradeMessageTimeout && m_mqttConnected == false )
        {
            certDownloadInProgress = false;
            m_downloadSuccess = false;
            ledManager.overrideColor(RGB_ORANGE);
            sendOtaStatusMsg(MSPCommand::CERT_UPGRADE_ABORT,CERT_UPGRADE_ERR,"CERT-RCA-0013");
            for(int i = 0 ; i < 20; i++) {
                ledManager.overrideColor(RGB_RED);
                delay(200);
                ledManager.stopColorOverride();
                delay(200);
            }
            
            if (loopDebugMode)
            {
                debugPrint("CERT - Download Request Timeout, Upgrade Status not received ");
            }
            certDownloadMode = false;
            if (loopDebugMode) { debugPrint(F("Switching Device mode:OTA/CERT -> OPERATION")); }
            iuWiFi.m_setLastConfirmedPublication();
            conductor.changeUsageMode(UsageMode::OPERATION);
            if(!otaSendMsg){
                iuWiFi.hardReset();
            }
        }
    }    
    
}
/**
 * @brief 
 * This function does the FW validation after OTA process downloads new FW.
 * @param None 
 * @return None 
  */
uint32_t Conductor::firmwareValidation()
{
    File ValidationFile;
    double timestamp;
    if(loopDebugMode){ debugPrint(F("FW Validation Start")); }
    if(DOSFS.exists("Validation.txt"))
    {
        ValidationFile = DOSFS.open("Validation.txt", "a");
    }
    else
    {
        ValidationFile = DOSFS.open("Validation.txt", "w");
    }
    if (!ValidationFile)
    {
        if(loopDebugMode){ debugPrint(F("Validation-File Open: Failed !")); }
        return OTA_VALIDATION_RETRY;
    }
    if(FW_Valid_State == 0) {
        ValidationFile.println(F("***************************************************************************************" ));
        ValidationFile.println(F("**************************  vEdge OTA FW VALIDATION REPORT  ***************************" ));
        ValidationFile.println(F("***************************************************************************************" ));
        ValidationFile.println(F(""));
        ValidationFile.print(F("Device Type    : " ));
        ValidationFile.println(OTA_DEVICE_TYPE);
        ValidationFile.print(F("Main FW Version: " ));
        ValidationFile.println(FIRMWARE_VERSION);
        ValidationFile.print(F("WiFi FW Version: " ));
        ValidationFile.println(iuWiFi.espFirmwareVersion);
        ValidationFile.print(F("Validation Time: " ));
        ValidationFile.println(conductor.getDatetime());
        ValidationFile.println(F("Validation[DEV]-FLASH Read: OK"));
        if(loopDebugMode){ debugPrint(F("Validation[DEV]-File Open: OK")); }

        firmwareConfigValidation(&ValidationFile);

        if(firmwareDeviceValidation(&ValidationFile))
        {
            ValidationFile.println("");
            ValidationFile.println(F("" ));
            ValidationFile.println(F("******************************* FW VALIDATION RETRY  **********************************" ));
            ValidationFile.print(F("Retry Validation Time: " ));
            ValidationFile.println(conductor.getDatetime());
            ValidationFile.close();
            return OTA_VALIDATION_RETRY;        
        }
 //       ValidationFile.close();
    }    
    return (firmwareWifiValidation(&ValidationFile));
}

/**
 * @brief 
 * Part of main FW validation for WiFi functionality.
 * @param None 
 * @return None 
 */
uint8_t Conductor::firmwareWifiValidation(File *ValidationFile)
{
    bool WiFiCon = false;
    uint8_t ret = 0;
    ValidationFile->println(F("***** DEVICE WIFI COMMUNICATION DETAILS  *****" ));
    if(FW_Valid_State == 0) {
        ValidationFile->println(F("DEVICE WIFI STATUS:"));
        if(loopDebugMode){ debugPrint(F("DEVICE WIFI STATUS")); }
        WiFiCon = iuWiFi.isConnected();
        if(WiFiCon)
        {
            ValidationFile->println(F("DEVICE WIFI STATUS: CONNECTED"));
            if(loopDebugMode){ debugPrint(F("DEVICE WIFI STATUS:CONNECTED")); }
            FW_Valid_State = 2;
            ret = OTA_VALIDATION_SUCCESS;
        }
        else
        {
            ValidationFile->println(F("DEVICE WIFI STATUS: NOT CONNECTED !"));
            if(loopDebugMode){ debugPrint(F("DEVICE WIFI STATUS:NOT CONNECTED !")); }
            ValidationFile->println(F("DEVICE WIFI TRYING CONNECT (WIFI RESET)"));
            iuWiFi.turnOff();
            delay(100);
            iuWiFi.turnOn(1);   
            FW_Valid_State = 1;
            ret = OTA_VALIDATION_WIFI;
        }
        ValidationFile->close();
    }
    else if(FW_Valid_State == 1)
    {
        ValidationFile->println(F("DEVICE WIFI CONNECT:"));
        if(loopDebugMode){ debugPrint(F("DEVICE WIFI CONNECT:")); }
        WiFiCon = iuWiFi.isConnected();    
        if(WiFiCon == 0)
        {
            ValidationFile->println(F("DEVICE WIFI CONNECT-FAILED"));
            if(loopDebugMode){ debugPrint(F("DEVICE WIFI CONNECT-FAILED")); }
        }
        else
        {
            ValidationFile->println(F("DEVICE WIFI CONNECT-OK"));
            if(loopDebugMode){ debugPrint(F("DEVICE WIFI CONNECT-OK")); }
        }
        FW_Valid_State = 2;
        ValidationFile->close();
        ret = OTA_VALIDATION_SUCCESS;
        if(loopDebugMode){ debugPrint(F("FIRMWARE VALIDATION COMPLETED")); }
    }    
    return ret;
}

/**
 * @brief 
 * Part of main FW validation for configuration files check.
 * @param None 
 * @return None 
 */
uint8_t Conductor::firmwareConfigValidation(File *ValidationFile)
{
    ValidationFile->println(F("***** DEVICE CONFIGURATION DETAILS  *****" ));
    if(loopDebugMode){ debugPrint(F("DEVICE MQTT CONFIG CHECK:-")); }
    ValidationFile->println(F("DEVICE MQTT CONFIG CHECK:-"));
    // 1. Check default parameter setting
    ValidationFile->print(F(" - MQTT DEFAULT SERVER IP:"));
    ValidationFile->println(MQTT_DEFAULT_SERVER_IP);
    if(strcmp(MQTT_DEFAULT_SERVER_IP,"mqtt.infinite-uptime.com") != 0)
    {
        ValidationFile->println(F("   Validation [MQTT]-Default IP Add: Fail !"));
        if(loopDebugMode){ debugPrint(F("Validation [MQTT]-Default IP Add: Fail !")); }
    }
    ValidationFile->print(F(" - MQTT DEFAULT SERVER PORT:"));
    ValidationFile->println(MQTT_DEFAULT_SERVER_PORT);
    if(MQTT_DEFAULT_SERVER_PORT != 8883)
    {
        ValidationFile->println(F("   Validation [MQTT]-Default Port: Fail !"));
        if(loopDebugMode){ debugPrint(F("Validation [MQTT]-Default Port: Fail !")); }
    }

    // 2. Check MQTT update from config file stored in ext. flash
    conductor.configureMQTTServer("MQTT.conf");
    // 3. Check default parameter setting changed to read from config file ?
    if(strcmp(m_mqttServerIp,"mqtt.infinite-uptime.com") == 0 && m_mqttServerPort == 8883 &&
      (strcmp(m_mqttUserName,NULL) == 0) && (strcmp(m_mqttPassword,NULL) == 0))
    {
        ValidationFile->println(F("   Validation [MQTT]-Read Config File: Fail !"));
        if(loopDebugMode){ debugPrint(F("Validation [MQTT]-Read Config File: Fail !")); }
    }
    ValidationFile->println(F(" - MQTT FLASH CONFIG FILE READ: OK"));
    if(loopDebugMode){ debugPrint(F("MQTT FLASH CONFIG FILE READ: OK")); }

    ValidationFile->println(F("DEVICE HTTP CONFIG CHECK:-"));
    //http configuration
    if(configureBoardFromFlash("httpConfig.conf",1) == false)
    {
        ValidationFile->println(F("   Validation [HTTP]-Read Config File: Fail !"));
        ValidationFile->print(F(" - HTTP DEFAULT HOST IP:"));
        ValidationFile->println(m_httpHost);
        if(strcmp(m_httpHost,"api-idap.infinite-uptime.com"))
        {
            ValidationFile->println(F("   Validation [HTTP]-Default HOST IP: Fail !"));
            if(loopDebugMode){ debugPrint(F("Validation [HTTP]-Default HOST IP: Fail !")); }
        }
        ValidationFile->print(F(" - HTTP DEFAULT HOST PORT:"));
        ValidationFile->println(m_httpPort);
        if(m_httpPort != 443)
        {
            ValidationFile->println(F("   Validation [HTTP]-Default HOST PORT: Fail !"));
            if(loopDebugMode){ debugPrint(F("Validation [HTTP]-Default HOST PORT: Fail !")); }    
        }
        ValidationFile->print(F(" - HTTP DEFAULT HOST END POINT:"));
        ValidationFile->println(m_httpPath);
        if(strcmp(m_httpPath,"/api/2.0/datalink/http_dump_v2"))
        {
            ValidationFile->println(F("   Validation [HTTP]-Default HOST END Point: Fail !"));
            if(loopDebugMode){ debugPrint(F("Validation [HTTP]-Default HOST END Point: Fail !")); }
        }
    }
    else
    {
        ValidationFile->println(F(" - HTTP FLASH CONFIG FILE READ: OK"));
        if(loopDebugMode){ debugPrint(F("HTTP FLASH CONFIG FILE READ: OK")); }
    }    
    ValidationFile->println(F("DEVICE FFT CONFIG CHECK:-"));
    ValidationFile->print(F(" - FFT DEFAULT SAMPLING RATE:"));
    ValidationFile->println(FFTConfiguration::DEFAULT_SAMPLING_RATE);
    if(FFTConfiguration::DEFAULT_SAMPLING_RATE != 25600)
    {
        ValidationFile->println(F("   Validation [FFT]-Default Sampling Rate: Fail !"));
        if(loopDebugMode){ debugPrint(F("Validation [FFT]-Default Sampling Rate: Fail !")); }
    }
    ValidationFile->print(F(" - FFT DEFAULT BLOCK SIZE:"));
    ValidationFile->println(FFTConfiguration::DEFAULT_BLOCK_SIZE);
    if(FFTConfiguration::DEFAULT_BLOCK_SIZE != 8192)
    {
        ValidationFile->println(F("   Validation [FFT]-Default Block Size: Fail !"));
        if(loopDebugMode){ debugPrint(F("Validation [FFT]-Default Block Size: Fail !")); }
    }

    // Update the configuration of FFT computers from fft.conf
    if(setFFTParams() == false)
    {
        ValidationFile->println(F("   Validation [FFT]-Read Config File: Fail !"));
        ValidationFile->println(F("   Validation [FFT]- Using Default FFT Configs"));
        if(loopDebugMode){ debugPrint(F("Validation [FFT]-Read Config File: Fail !")); }
    }
    else
    {
        ValidationFile->println(F(" - FFT FLASH CONFIG FILE READ: OK"));
        if(loopDebugMode){ debugPrint(F("FFT FLASH CONFIG FILE READ: OK")); }
    }
}

/**
 * @brief 
 * Part of main FW validation for device parameter check.
 * @param None 
 * @return None 
 */
uint8_t Conductor::firmwareDeviceValidation(File *ValidationFile)
{
    ValidationFile->println(F("***** DEVICE PERIPHERAL DETAILS  *****" ));
    char Cnt = 0;
    uint8_t otaRtryValidation = 0;

    ValidationFile->println(F("DEVICE MEMORY: "));
    ValidationFile->print(F(" - Internal RAM Free: "));
    ValidationFile->print((freeMemory()/1024),DEC);
    ValidationFile->println(F(" KBytes"));
    if(freeMemory() < 30000)
    {
        ValidationFile->println(F("Validation [DEV]-Free Memory: Fail !"));
        if(loopDebugMode){ debugPrint(F("Validation [DEV]-Free Memory: Fail !")); }      
        ValidationFile->println(F("***************************************************************************************" ));
        ValidationFile->println(F(""));
        // return OTA_VALIDATION_FAIL;  // Bypass Freememory check because of low memory when we configure Diagnostics.
    }
    if(loopDebugMode){ debugPrint(F("Validation [DEV]-Free Memory: Ok")); }  
    F_SPACE space;
    f_getfreespace(&space);
    size_t total_space = (uint64_t)space.total | ((uint64_t)space.total_high << 32);
    size_t used_space = (uint64_t)space.used  | ((uint64_t)space.used_high << 32);
    ValidationFile->print(F(" - External Flash Memory Total: "));
    ValidationFile->print((total_space/1024));
    ValidationFile->println(" KBytes");
    ValidationFile->print(F(" - External Flash Memory Used: "));
    ValidationFile->print((used_space/1024));
    ValidationFile->println(" KBytes");
    ValidationFile->print(F(" - External Flash Memory Free: "));
    ValidationFile->print(((total_space-used_space)/1024));
    ValidationFile->println(" KBytes");
    if(loopDebugMode) {
        debugPrint("External Flash Total Memory:",false);
        debugPrint((total_space/1024),false);
        debugPrint(" KBytes");
        debugPrint("External Flash Used Memory:",false);
        debugPrint((used_space/1024),false);
        debugPrint(" KBytes");
        debugPrint("External Flash Free Memory:",false);
        debugPrint(((total_space-used_space)/1024),false);
        debugPrint(" KBytes");
    }
    ValidationFile->print(F("DEVICE BLE MAC ADDRESS :"));
    ValidationFile->println(m_macAddress);
    MacAddress Mac(00,00,00,00,00,00);
    if(m_macAddress == Mac || !iuBluetooth.isBLEAvailable)
    {
        ValidationFile->println(F("Validation [DEV]-BLE MAC: Fail !"));
        if(loopDebugMode){ debugPrint(F("Validation [DEV]-BLE MAC: Fail !")); }
        otaRtryValidation++;
    }
    if(loopDebugMode){
        debugPrint(F("Validation [DEV]-BLE MAC: OK "));
    }
    ValidationFile->print(F("DEVICE WIFI MAC ADDRESS:"));
    ValidationFile->println(iuWiFi.getMacAddress());
    if(iuWiFi.getMacAddress() == Mac)
    {
        ValidationFile->println(F("Validation [DEV]-WiFi MAC: Fail !"));
        if(loopDebugMode){ debugPrint(F("Validation [DEV]-WiFi MAC: Fail !")); }
        otaRtryValidation++;
    }
    if(loopDebugMode){
        debugPrint(F("Validation [DEV]-WIFI MAC: OK "));
    }
    ValidationFile->println(F("DEVICE SENSOR INTERFACE CHECK:-"));

    if ((FFTConfiguration::currentSensor == FFTConfiguration::lsmSensor) && (iuAccelerometer.lsmPresence))
    {
        iuI2C.scanDevices();
        ValidationFile->println(F("DEVICE LSM COMM CHECK:-"));
        if (iuI2C.checkComponentWhoAmI("LSM6DSM ACC", iuAccelerometer.ADDRESS,iuAccelerometer.WHO_AM_I,iuAccelerometer.I_AM) == false)
        {
            ValidationFile->println(F("   Validation [LSM]-Read Add: Fail !"));
            if(loopDebugMode){ debugPrint(F("Validation [LSM]-Read Add: Fail !")); }
            otaRtryValidation++;
        }
        else
        {        // iuI2C.releaseReadLock();
            if(iuI2C.i2c_dev[0] == iuAccelerometer.I_AM || iuI2C.i2c_dev[1] == iuAccelerometer.I_AM)
            {
                ValidationFile->print(F("LSM6DSM I2C Add: 0x"));
                ValidationFile->println(iuAccelerometer.I_AM,HEX);
                ValidationFile->println(F("   Validation [LSM]-Read Add: Ok"));
                if(loopDebugMode){ debugPrint(F("Validation [LSM]-Read Add: Ok")); }
            }
            else
            {
                ValidationFile->println(F("   Validation [LSM]-Read Add: Fail !"));
                if(loopDebugMode){ debugPrint(F("Validation [LSM]-Read Add: Fail")); }
                otaRtryValidation++;
            }
        }
    }
    else if((FFTConfiguration::currentSensor == FFTConfiguration::kionixSensor) && (iuAccelerometerKX222.kionixPresence))
    {
        if(iuAccelerometerKX222.checkWHO_AM_I() == false)
        {
            ValidationFile->println(F("   Validation [KX222]-Read ID: Fail !"));
            if(loopDebugMode){ debugPrint(F("Validation [KX222]-Read ID: Fail")); }
            otaRtryValidation++;
        }
        else
        {
            ValidationFile->print(F("KX222 WIA ID: 0x"));
            ValidationFile->println(IUKX222_WHO_AM_I_WIA_ID,HEX);
            ValidationFile->println(F("   Validation [KX222]-Read Add: Ok"));
            if(loopDebugMode){ debugPrint(F("Validation [KX222]-Read Add: Ok")); }
        }
    }

#if 0 // On board temperature Sensor detetction
    if(iuI2C.i2c_dev[0] == iuTemp.ADDRESS || iuI2C.i2c_dev[1] == iuTemp.ADDRESS)
    {
        ValidationFile->print(F("TMP116 I2C Device ID: 0x"));
        ValidationFile->println(iuTemp.I_AM,HEX);
        ValidationFile->println(F("   Validation [TMP]-Read Add: Ok"));
        if(loopDebugMode){ debugPrint(F("Validation [TMP]-Read Add: Ok")); }
    }
    else
    {
        ValidationFile->println(F("   Validation [TMP]-Read Add: Fail !"));
        if(loopDebugMode){ debugPrint(F("Validation [TMP]-Read Add: Fail !")); }
        otaRtryValidation++;
    }
#endif
    // iuI2C1.scanDevices();
    if(iuI2C1.i2c_dev[0] == iuTemp.ADDRESS || iuI2C1.i2c_dev[1] == iuTemp.ADDRESS)
    {
        ValidationFile->print(F("TMP116 I2C1 Device ID: 0x"));
        ValidationFile->println(iuTemp.I_AM,HEX);
        ValidationFile->println(F("   Validation [TMP]-Read Add: Ok"));
        if(loopDebugMode){ debugPrint(F("Validation [TMP]-Read Add: Ok")); }
    }
    else
    {
        ValidationFile->println(F("   Validation [TMP]-Read Add: Fail !"));
        if(loopDebugMode){ debugPrint(F("Validation [TMP]-Read Add: Fail !")); }
        // otaRtryValidation++;
    }

    ValidationFile->print(F("DEVICE READ Audio Data: "));
    ValidationFile->print(audioDB4096Computer.dBresult);
    ValidationFile->println(F(" dB"));

    // if ( (audioDB4096Computer.dBresult < 58.0) && (audioDB4096Computer.dBresult > 160.0) ){
    //     ValidationFile->println(F("   Validation [AUD]-Read Acoustic: Fail !"));
    //     otaRtryValidation++;
    // }
    return otaRtryValidation;

// To be added Keonics sensor
    /* Device communication check with Keonics Sensor usign SPI */
}

/**
 * @brief 
 * Read STM internal flash status flag values
 * @param None 
 * @return None 
 */
void Conductor::getOtaStatus()
{
    readOtaConfig();
    readForceOtaConfig();
    iuOta.readOtaFlag();
    uint8_t otaStatus = iuOta.getOtaFlagValue(OTA_STATUS_FLAG_LOC);
    if (setupDebugMode) {
        debugPrint("Main FW:OTA Status Code: ",false);
        debugPrint(otaStatus);
    }
    otaSendMsg = false;
    switch(otaStatus)
    {
        case OTA_FW_VALIDATION_SUCCESS:
            if (setupDebugMode) debugPrint("Main FW:OTA Validation Success...");
            break;                  // Alrady validated FW, continue running it.
        case OTA_FW_UPGRADE_SUCCESS:
            if (setupDebugMode) debugPrint("Main FW:OTA Upgrade Success, Doing validation..");
            doOnceFWValid = true;   // New FW upgraded, perform validation
            break;
        case OTA_FW_UPGRADE_FAILED:
            if (setupDebugMode) debugPrint("FW OTA Upgrade Failed ! Upgrade retry ");
            otaSendMsg = true;
            break;
        case OTA_FW_INTERNAL_ROLLBACK:
            if (setupDebugMode) debugPrint("FW OTA Upgrade Failed ! Internal Rollback ");
            otaSendMsg = true;
            break;
        case OTA_FW_FORCED_ROLLBACK: // Reset, as L2 shall perform Upgrade,Rollback or Forced Rollback
            if (setupDebugMode) debugPrint("FW OTA Upgrade Failed ! Forced Rollback ");
            otaSendMsg = true;
            break;
        case OTA_FW_FILE_CHKSUM_ERROR:
            if (setupDebugMode) debugPrint("FW OTA Upgrade Failed ! File Checksum Failed ");
            otaSendMsg = true;
            break;
        case OTA_FW_FILE_SYS_ERROR:
            if (setupDebugMode) debugPrint("FW OTA Upgrade Failed ! Missing or Invalid File(s) ");
            otaSendMsg = true;
            break;
        default:
            if (setupDebugMode) debugPrint("Main FW:Unknown OTA Status code !",false);
            break;
    }

    otaStatus = iuOta.getOtaFlagValue(OTA_PEND_STATUS_MSG_LOC);
    if (setupDebugMode) {
        debugPrint("Main FW:OTA Pending Status Code: ",false);
        debugPrint(otaStatus);
    }
    switch(otaStatus)
    {
        case OTA_FW_DNLD_FAIL_PENDING:
            strcpy(WiFiDisconnect_OTAErr,"OTA-RCA-0003");
            /* Send this message in case WiFi Disconnection during last OTA FW Download */
            if (setupDebugMode) debugPrint("FW OTA download Failed ! Message pending. ");
            otaSendMsg = true;
            break;
        case OTA_FW_UPGRD_OK_PENDING:
            /* Send this message in case WiFi Disconnection during last OTA FW Download */
            if (setupDebugMode) debugPrint("FW OTA Upgrade Ok ! Message pending. ");
            otaSendMsg = true;
            break;
        // case OTA_FW_DNLD_OK_PENDING:
        //     /* Send this message in case WiFi Disconnection during last OTA FW Download */
        //     if (setupDebugMode) debugPrint("FW OTA Upgrade Ok ! Message pending. ");
        //     otaSendMsg = true;
        //     break;
        case OTA_FW_UPGRD_FAIL_PENDING:
            /* Send this message in case WiFi Disconnection during last OTA FW Download */
            if (setupDebugMode) debugPrint("FW OTA Upgrade Fail ! Message pending. ");
            otaSendMsg = true;
            break;
        default:
            if (setupDebugMode) debugPrint("Main FW:Unknown OTA Status code !",false);
            break;            
    }
}

/**
 * @brief 
 * Send MQTT message based on Internal STM flash status flag values
 * @param None 
 * @return None 
 */
void Conductor::sendOtaStatus()
{
    if((iuWiFi.isConnected()) && (otaSendMsg == true) && validTimeStamp() && iuWiFi.getConnectionStatus()) 
    {
        iuOta.readOtaFlag();
        uint8_t otaStatus = iuOta.getOtaFlagValue(OTA_STATUS_FLAG_LOC);
        if (setupDebugMode) {
            debugPrint("Main FW:OTA Status Code: ",false);
            debugPrint(otaStatus);
        }
        switch(otaStatus)
        {
            case OTA_FW_UPGRADE_FAILED:
                if (setupDebugMode) debugPrint("FW OTA Upgrade Failed ! Upgrade retry ");
                sendOtaStatusMsg(MSPCommand::OTA_FUG_ABORT,OTA_UPGRADE_ERR,String(iuOta.getOtaRca(OTA_UPGRADE_FAIL)).c_str());
                delay(1000);
                break;
            case OTA_FW_INTERNAL_ROLLBACK:
                if (setupDebugMode) debugPrint("FW OTA Upgrade Failed ! Internal Rollback ");
                sendOtaStatusMsg(MSPCommand::OTA_FUG_ABORT,OTA_UPGRADE_ERR,String(iuOta.getOtaRca(OTA_INT_RLBK_FAIL)).c_str());
                delay(1000);
                break;
            case OTA_FW_FORCED_ROLLBACK: // Reset, as L2 shall perform Upgrade,Rollback or Forced Rollback
                if (setupDebugMode) debugPrint("FW OTA Upgrade Failed ! Forced Rollback ");
                sendOtaStatusMsg(MSPCommand::OTA_FUG_ABORT,OTA_UPGRADE_ERR,String(iuOta.getOtaRca(OTA_FORCED_RLBK_FAIL)).c_str());
                delay(1000);
                break;
            case OTA_FW_FILE_CHKSUM_ERROR:
                if (setupDebugMode) debugPrint("FW OTA Upgrade Failed ! File Checksum Failed ");
                sendOtaStatusMsg(MSPCommand::OTA_FUG_ABORT,OTA_UPGRADE_ERR,String(iuOta.getOtaRca(OTA_CHECKSUM_FAIL)).c_str());
                delay(1000);
                break;
            case OTA_FW_FILE_SYS_ERROR:
                if (setupDebugMode) debugPrint("FW OTA Upgrade Failed ! Missing or Invalid File(s) ");
                sendOtaStatusMsg(MSPCommand::OTA_FUG_ABORT,OTA_UPGRADE_ERR,String(iuOta.getOtaRca(OTA_FILE_MISSING)).c_str());
                delay(1000);
                break;
            default:
                if (setupDebugMode) debugPrint("Main FW:Unknown OTA Status code !",false);
                break;
        }

        otaSendMsg = false;
        /* Send Error message only once. Not to send on every bootup */
        iuOta.updateOtaFlag(OTA_STATUS_FLAG_LOC,OTA_FW_VALIDATION_SUCCESS);

        otaStatus = iuOta.getOtaFlagValue(OTA_PEND_STATUS_MSG_LOC);
        if (setupDebugMode) {
            debugPrint("Main FW:OTA Status Code: ",false);
            debugPrint(otaStatus);
        }
        switch(otaStatus)
        {
            case OTA_FW_DNLD_FAIL_PENDING:
                /* Send this message in case WiFi Disconnection during last OTA FW Download */
                if (setupDebugMode) debugPrint("FW OTA download Failed ! WiFi Disconnected for last FW download. ");
                sendOtaStatusMsg(MSPCommand::OTA_FDW_ABORT,OTA_DOWNLOAD_ERR,WiFiDisconnect_OTAErr);
                delay(1000);
                break;
            case OTA_FW_UPGRD_OK_PENDING:
                /* Send this message in case WiFi Disconnection during last OTA FW Download */
                if (setupDebugMode) debugPrint("FW OTA Upgrade Ok ");
                sendOtaStatusMsg(MSPCommand::OTA_FUG_SUCCESS,OTA_UPGRADE_OK,OTA_RESPONE_OK);
                delay(1000);
                break;
            // case OTA_FW_DNLD_OK_PENDING:
            //     /* Send this message in case WiFi Disconnection during last OTA FW Download */
            //     if (setupDebugMode) debugPrint("FW OTA Download Ok ");
            //     sendOtaStatusMsg(MSPCommand::OTA_FDW_SUCCESS,OTA_DOWNLOAD_OK,OTA_RESPONE_OK);
            //     delay(1000);
            //     break;
            case OTA_FW_UPGRD_FAIL_PENDING:
                /* Send this message in case WiFi Disconnection during last OTA FW Download */
                if (setupDebugMode) debugPrint("FW OTA Upgrade Failed ");
                sendOtaStatusMsg(MSPCommand::OTA_FUG_ABORT,OTA_UPGRADE_ERR,(char *)iuOta.getOtaRca(OTA_VALIDATION_FAILED).c_str());
                delay(1000);
                break;
            default:
                if (setupDebugMode) debugPrint("Main FW:Unknown OTA Status code !",false);
                break;            
        }
        otaSendMsg = false;
        /* Send Error message only once. Not to send on every bootup */
        iuOta.updateOtaFlag(OTA_PEND_STATUS_MSG_LOC,OTA_FW_VALIDATION_SUCCESS);
    }
}

/**
 * @brief 
 * Send MQTT message for OTA status
 * @param None 
 * @return None 
 */
void Conductor::sendOtaStatusMsg(MSPCommand::command type, char *msg, const char *errMsg)
{
    char otaResponse[256];
    double otaInitTimeStamp = conductor.getDatetime(); 
    if(MSPCommand::OTA_FDW_SUCCESS == type || MSPCommand::OTA_FUG_START == type || MSPCommand::CERT_DOWNLOAD_SUCCESS == type ||
                                              MSPCommand::CERT_UPGRADE_SUCCESS == type || MSPCommand::CERT_UPGRADE_ABORT == type    ||
                                              MSPCommand::CERT_DOWNLOAD_ABORT == type  || MSPCommand::ALL_MQTT_CONNECT_ATTEMPT_FAILED == type ||
                                              MSPCommand::CERT_DOWNLOAD_INIT == type   || MSPCommand::CERT_DOWNLOAD_INIT_ACK == type ||
                                              MSPCommand::DOWNLOAD_TLS_SSL_START == type  || MSPCommand::OTA_FDW_START == type ||
                                              MSPCommand::OTA_FDW_SUCCESS == type)
    {
        if (loopDebugMode) { debugPrint(F("Sending OTA Status Message")); }   
        snprintf(otaResponse, 256, "{\"messageId\":\"%s\",\"deviceIdentifier\":\"%s\",\"type\":\"%s\",\"status\":\"%s\",\"reasonCode\":\"%s\",\"timestamp\":%.2f}",
        m_otaMsgId,m_macAddress.toString().c_str(), OTA_DEVICE_TYPE,msg, errMsg ,otaInitTimeStamp);
        iuOta.otaSendResponse(type, otaResponse);  // Checksum failed
    }
    else {
        if(iuWiFi.isConnected() && validTimeStamp() && iuWiFi.getConnectionStatus()) {
            if (loopDebugMode) { debugPrint(F("WiFi Conntected, Sending OTA | Cert Status Message")); }   
            snprintf(otaResponse, 256, "{\"messageId\":\"%s\",\"deviceIdentifier\":\"%s\",\"type\":\"%s\",\"status\":\"%s\",\"reasonCode\":\"%s\",\"timestamp\":%.2f}",
            m_otaMsgId,m_macAddress.toString().c_str(), OTA_DEVICE_TYPE,msg, errMsg ,otaInitTimeStamp);
            iuOta.otaSendResponse(type, otaResponse);  // Checksum failed
        }
        else
        {
            if (loopDebugMode) {
                debugPrint(F("WiFi Not Conntected, Storing OTA Status Message:"));
                debugPrint(type);
            }
            if(MSPCommand::OTA_FDW_ABORT == type)
            {
                strcpy(WiFiDisconnect_OTAErr,errMsg);
                iuOta.updateOtaFlag(OTA_PEND_STATUS_MSG_LOC,OTA_FW_DNLD_FAIL_PENDING);
            }
            else if(MSPCommand::OTA_FUG_ABORT == type)
            {
                iuOta.updateOtaFlag(OTA_PEND_STATUS_MSG_LOC,OTA_FW_UPGRD_FAIL_PENDING);
            }
            else if(MSPCommand::OTA_FUG_SUCCESS == type)
            {
                iuOta.updateOtaFlag(OTA_PEND_STATUS_MSG_LOC,OTA_FW_UPGRD_OK_PENDING);
            }
            otaSendMsg = true;
            delay(10); 
        }
    }
}

/**
 * @brief 
 * OTA FW Validation main function. After bootup, waits for get device details, streaming to start
 * 1. Check external flash and create Validation.txt
 * 2. Check configuration files
 * 3. Check device parameter, peripheral access, RAM, External flash
 * On Validation success, copy new Rollback files to backup folder, and new OTA files to rollback folder
 * On Validation fail, update status flag for internal rollback
 * @param None 
 * @return None 
 */
void Conductor::otaFWValidation()
{
#if 1 // FW Validation
    if(doOnceFWValid == true)
    {
        if((FWValidCnt % 2000) == 0 && FWValidCnt > 0)
        {
            uint32_t ret = 0;
            if(loopDebugMode) debugPrint("Running Firmware Validation ");
            ret = conductor.firmwareValidation();
            if(ret == OTA_VALIDATION_WIFI)
            {// Waiting for WiFi Disconnect/Connect Cycle.
                doOnceFWValid = true;
                FWValidCnt = 1;         
            }
            else if(ret == OTA_VALIDATION_RETRY)
            {
                iuOta.readOtaFlag();
                uint8_t otaVldnRetry = iuOta.getOtaFlagValue(OTA_VLDN_RETRY_FLAG_LOC);
                otaVldnRetry++;
                iuOta.updateOtaFlag(OTA_VLDN_RETRY_FLAG_LOC,otaVldnRetry);
                if (loopDebugMode) {
                    debugPrint("OTA Validation Retry No: ",false);
                    debugPrint(otaVldnRetry);
                    debugPrint("Retrying Validation. Rebooting Device.....");
                }
                if(otaVldnRetry > OTA_MAX_VALIDATION_RETRY)
                {
                    ledManager.overrideColor(RGB_RED);
                    conductor.sendOtaStatusMsg(MSPCommand::OTA_FUG_ABORT,OTA_UPGRADE_ERR, (char *)iuOta.getOtaRca(OTA_VALIDATION_FAILED).c_str());
                    /*  Initialize OTA FW Validation retry count */
                    iuOta.updateOtaFlag(OTA_VLDN_RETRY_FLAG_LOC,0);  
                    iuOta.updateOtaFlag(OTA_STATUS_FLAG_LOC,OTA_FW_INTERNAL_ROLLBACK);
                    if (loopDebugMode) {
                        debugPrint("OTA FW Validation Retry Overflow ! Validation Failed");
                        debugPrint("Initiating Rollback FW. Rebooting Device.....");
                    }
                }
                delay(2000);
                DOSFS.end();
                delay(10);
                STM32.reset();
            }
            else if(ret == OTA_VALIDATION_SUCCESS)
            {
                FW_Valid_State = 0;
                ledManager.overrideColor(RGB_PURPLE);
                /* Copy FW binaries, MD5 from rollback to Backup folder */
                iuOta.otaFileCopy(iuFlash.IUFWBACKUP_SUBDIR, iuFlash.IUFWROLLBACK_SUBDIR,vEdge_Main_FW_BIN);
                iuOta.otaFileCopy(iuFlash.IUFWBACKUP_SUBDIR, iuFlash.IUFWROLLBACK_SUBDIR,vEdge_Wifi_FW_BIN);
                iuOta.otaFileCopy(iuFlash.IUFWBACKUP_SUBDIR, iuFlash.IUFWROLLBACK_SUBDIR,vEdge_Main_FW_MD5);
                iuOta.otaFileCopy(iuFlash.IUFWBACKUP_SUBDIR, iuFlash.IUFWROLLBACK_SUBDIR,vEdge_Wifi_FW_MD5);
                delay(10);
                /* Copy FW binaries, MD5 from Temp folder to rollback folder */
                iuOta.otaFileCopy(iuFlash.IUFWROLLBACK_SUBDIR, iuFlash.IUFWTMPIMG_SUBDIR,vEdge_Main_FW_BIN);
                iuOta.otaFileCopy(iuFlash.IUFWROLLBACK_SUBDIR, iuFlash.IUFWTMPIMG_SUBDIR,vEdge_Wifi_FW_BIN);
                iuOta.otaFileCopy(iuFlash.IUFWROLLBACK_SUBDIR, iuFlash.IUFWTMPIMG_SUBDIR,vEdge_Main_FW_MD5);
                iuOta.otaFileCopy(iuFlash.IUFWROLLBACK_SUBDIR, iuFlash.IUFWTMPIMG_SUBDIR,vEdge_Wifi_FW_MD5);
                conductor.sendOtaStatusMsg(MSPCommand::OTA_FUG_SUCCESS,OTA_UPGRADE_OK,OTA_RESPONE_OK);
                iuOta.updateOtaFlag(OTA_STATUS_FLAG_LOC,OTA_FW_VALIDATION_SUCCESS);
                /*  Initialize OTA FW Validation retry count */
                doOnceFWValid = false;
                iuOta.updateOtaFlag(OTA_VLDN_RETRY_FLAG_LOC,0);
                if(DOSFS.exists("MQTT.conf")){
                    JsonObject& config = configureJsonFromFlash("MQTT.conf",1);
                    if(config.success()){
                        int serverPort = config["mqtt"]["port"];
                        if(serverPort != 8883 && serverPort != 8884){
                            char mqttConfig[510];
                            sprintf(mqttConfig,"{\"mqtt\":{\"mqttServerIP\":\"%s\",\"port\":%d,\"username\":\"%s\",\"password\":\"%s\"}}",MQTT_DEFAULT_SERVER_IP,MQTT_DEFAULT_SERVER_PORT,MQTT_DEFAULT_USERNAME,MQTT_DEFAULT_ASSWORD);
                            debugPrint("Loading Default Secure MQTT Config : ",false);debugPrint(mqttConfig);
                            processConfiguration(mqttConfig,true);
                        }
                    }
                }
                if(DOSFS.exists("httpConfig.conf")){
                    JsonObject& config = configureJsonFromFlash("httpConfig.conf",1);
                    if(config.success()){
                        int httpPort = config["httpConfig"]["port"];
                        if(httpPort != 443){
                            char httpConfig[510];
                            sprintf(httpConfig,"{\"httpConfig\":{\"host\":\"%s\",\"port\":%d,\"path\":\"%s\",\"username\":\"%s\",\"password\":\"%s\",\"oauth\":\"%s\"}}",HTTP_DEFAULT_HOST,HTTP_DEFAULT_PORT,HTTP_DEFAULT_PATH,HTTP_DEFAULT_USERNAME,HTTP_DEFAULT_PASSWORD,HTTP_DEFAULT_OUTH);
                            debugPrint("Loading Default Secure Http Config : ",false);debugPrint(httpConfig);
                            httpOtaValidation = true; // To avoid Reboot
                            processConfiguration(httpConfig,true);
                        }
                    }
                }
                if (loopDebugMode) debugPrint("OTA FW Validation Successful. Rebooting device....");
                delay(2000);
                DOSFS.end();
                delay(10);               
                STM32.reset();
            }
            else if(ret == OTA_VALIDATION_FAIL)
            {   
                ledManager.overrideColor(RGB_RED);
                conductor.sendOtaStatusMsg(MSPCommand::OTA_FUG_ABORT,OTA_UPGRADE_ERR, (char *)iuOta.getOtaRca(OTA_VALIDATION_FAILED).c_str());
                /*  Initialize OTA FW Validation retry count */
                iuOta.updateOtaFlag(OTA_VLDN_RETRY_FLAG_LOC,0);  
                iuOta.updateOtaFlag(OTA_STATUS_FLAG_LOC,OTA_FW_INTERNAL_ROLLBACK);
                if (loopDebugMode) {
                    debugPrint("OTA FW Validation Failed ! ");
                    debugPrint("Initiating Rollback Rollback. Rebooting Device.....");
                }
                delay(2000);
                DOSFS.end();
                delay(10);
                STM32.reset();
            }
        }
        else
        {
            FWValidCnt++;
        }                  
    }
#endif
}

void Conductor::onBootFlashTest()
{
    if(DOSFS.exists("temp.conf")){  
        File tempFile = DOSFS.open("temp.conf","r");
        String fileContent = tempFile.readString();
        debugPrint("File Present");
        if(strcmp(fileContent.c_str(),"SUCCESS")==0){
            debugPrint("File Read Success");
        }else{
            debugPrint("File Content:", false);
            debugPrint(fileContent);
            debugPrint("File Read Failed...Formating Flash Please wait");
            ledManager.overrideColor(RGB_RED);
            DOSFS.format();
            ledManager.overrideColor(RGB_WHITE);
            debugPrint("Formated Successfully");
            File tempFile = DOSFS.open("temp.conf","w");
            if(tempFile){
                tempFile.print("SUCCESS");
                tempFile.flush();
                tempFile.close();
                debugPrint("File Write Success");
            }else{
                debugPrint("Formated failed");
            }
        }
        tempFile.close();
    }else{
        File tempFile = DOSFS.open("temp.conf","w");
        if(tempFile){
            tempFile.print("SUCCESS");
            tempFile.flush();
            tempFile.close();
            debugPrint("File Write Success");
        }else{
            debugPrint("File Read Failed...Formating Flash Please wait");
            ledManager.overrideColor(RGB_RED);
            DOSFS.format();
            ledManager.overrideColor(RGB_WHITE);
            debugPrint("Formated Successfully");
            File tempFile = DOSFS.open("temp.conf","w");
            if(tempFile){
                tempFile.print("SUCCESS");
                tempFile.flush();
                tempFile.close();
                debugPrint("File Write Success");
            }else{
                debugPrint("Formated failed");
            }
        }
    }
}

void Conductor::periodicFlashTest()
{
    if(DOSFS.exists("temp.conf"))
    {  
        File tempFile = DOSFS.open("temp.conf","r");
        String fileContent = tempFile.readString();
        debugPrint("File Present");
        if(strcmp(fileContent.c_str(),"SUCCESS")==0)
        {
            debugPrint("File Read Success");
        }
        else
        {
            debugPrint("File Read Failed...Formating Flash...");
            conductor.sendFlashStatusMsg(FLASH_ERROR,"Formating Flash...");
            DOSFS.format();
            delay(3000);
            DOSFS.end();
            delay(10);
            STM32.reset();
        }
            tempFile.close();
        }
        else
        {
        debugPrint("File Read Failed...Formating Flash...");
        conductor.sendFlashStatusMsg(FLASH_ERROR,"Formating Flash...");
        DOSFS.format();
        delay(3000);
        DOSFS.end();
        delay(10);
        STM32.reset();
    }
}
/**
 * @brief 
 * Send Flash Status Message
 * @param None 
 * @return None 
 */
void Conductor::sendFlashStatusMsg(int flashStatus, char *deviceStatus)
{
    char falshStatusResponse[256];
    char falshStatusCode[20];
    double timeStamp = conductor.getDatetime();     
    switch(flashStatus)
        {
            case FLASH_SUCCESS:
                strcpy(falshStatusCode,"FLASH-RCA-0000");
                break;
            case FLASH_ERROR:
                strcpy(falshStatusCode,"FLASH-RCA-0001");
                break;
        }       
    snprintf(falshStatusResponse, 256, "{ \"flashStatus\":{\"mac\":\"%s\",\"flash_status\":\"%s\",\"device_status\":\"%s\",\"timestamp\":%.2f}}",
        m_macAddress.toString().c_str(),falshStatusCode, deviceStatus ,timeStamp);
    iuWiFi.sendMSPCommand(MSPCommand::SEND_FLASH_STATUS,falshStatusResponse);
}

/**
 * @brief 
 * 
 * @param buff : Input data buffer 
 * @return uint8_t  : return the radioMode from 0 - 11
 */
uint8_t Conductor::processWiFiRadioModes(char* buff){

    // 400-[0-9] int idx(0), th1(0), th2(0), th3(0);
    int cmd(0), radioMode(0);
    sscanf(buff, "%d-%d", &cmd, &radioMode);

    iuWiFi.sendMSPCommand(MSPCommand::WIFI_SET_TX_POWER,buff);
    return radioMode;
}

/**
 * @brief This method reads the computated fingerprint JSON and extract all the keys and values and zero padd empty buffer elements
 * 
 * @return float* returns the fingerprints output data with [key1,value1,key2,value2,....keyN,valueN> format.
 */
float* Conductor::getFingerprintsforModbus(){
    
    static float modbusFingerprintDestinationBuffer[26];
    uint8_t bufferLength = sizeof(modbusFingerprintDestinationBuffer)/sizeof(float);

    if (strlen(fingerprintData)<5 || ready_to_publish_to_modbus == false  )
    {
        if (debugMode)
        {
            //debugPrint("Empty Fingerprint data buffer, reset the fingerprint Destination buffer");
        }
        //flush the buffer
        for (size_t i = 0; i < bufferLength; i++)
        {
            modbusFingerprintDestinationBuffer[i]= 0;
        }
        
    }else
    {  
         DynamicJsonBuffer object;
         JsonObject& root = object.parseObject(fingerprintData);
        
        int i = 0;    
        for (auto jsonKeyValue : root) {

             //debugPrint("count :",false);debugPrint(i);   
             int key = atoi(jsonKeyValue.key);
             float value = jsonKeyValue.value.as<float>();                   
             //debugPrint("KEY :",false);debugPrint(key); 
             
             modbusFingerprintDestinationBuffer[i] = (float)key;
             modbusFingerprintDestinationBuffer[i+1] = value;
             i = i + 2;
        }
      //debugPrint("i Count : ",false);debugPrint(i);
      if(i > 0 && i < bufferLength ){
        for (size_t index = bufferLength - 1; index >= i; --index) 
        {   
            modbusFingerprintDestinationBuffer[index] = 0.0;
        }
      }else
      {
          //debugPrint("MODBUS DEBUG : Fingerprints Buffer are completely filled or negative index");
      }
     // Enable in case want to see the data elements
   #if 0
   if(loopDebugMode){ 
        debugPrint("MODBUS DEBUG : FINGERPRINTS DATA: [ ",false);
        for (size_t i = 0; i < bufferLength; i= i+1)
        {   
            debugPrint(modbusFingerprintDestinationBuffer[i],false);debugPrint("->",false);
        }
        debugPrint(" ]");
     }
    #endif
 }
   return modbusFingerprintDestinationBuffer;    
}

bool Conductor::checkforModbusSlaveConfigurations(){

    // check if configurations are present in the internal flash storage of STM32
    bool validJson = false;
    bool success = true;
    
    if (iuFlash.checkConfig(CONFIG_MODBUS_SLAVE_CONFIG_FLASH_ADDRESS) && ! DOSFS.exists("/iuconfig/modbusSlave.conf") )
    {
        // Read the configurations
        String config = iuFlash.readInternalFlash(CONFIG_MODBUS_SLAVE_CONFIG_FLASH_ADDRESS);
        if(debugMode){
            debugPrint("MODBUS DEBUG : INTERNAL CONFIG # ",false);
            debugPrint("Intarnal Flash content #:");
            debugPrint(config);
        }
        char* modbusConfiguration =(char*)config.c_str();

        validJson =  processConfiguration(modbusConfiguration,true);
        free(modbusConfiguration);
    }else if (DOSFS.exists("/iuconfig/modbusSlave.conf"))
    {
        configureFromFlash(IUFlash::CFG_MODBUS_SLAVE);
    }else if(!validJson){
        // Configs not available in Internal Flash then apply default MODBUS Slave configurations
        iuModbusSlave.m_id =      DEFAULT_MODBUS_SLAVEID;
        iuModbusSlave.m_baud =    DEFAULT_MODBUS_BAUD;
        iuModbusSlave.m_databit = DEFAULT_MODBUS_DATABIT;
        iuModbusSlave.m_stopbit = DEFAULT_MODBUS_STOPBIT;
        iuModbusSlave.m_parity =  DEFAULT_MODBUS_PARITY;
        //LOAD THE CONFIGS
        iuModbusSlave.begin(iuModbusSlave.m_baud,iuModbusSlave.m_databit,iuModbusSlave.m_stopbit,iuModbusSlave.m_parity);
        iuModbusSlave.configure(iuModbusSlave.m_id,TOTAL_REGISTER_SIZE+1);
        modbusStreamingMode = true;   // Set the Streaming mode as MODBUS
        if(debugMode){
            debugPrint("MODBUS DEBUG : DEFAULT CONFIGS APPLIED");
            debugPrint("SLAVE ID  : ",false);debugPrint(iuModbusSlave.m_id);
            debugPrint("BAUD RATE :",false);debugPrint(iuModbusSlave.m_baud);
            debugPrint("DATA BIT  :",false);debugPrint(iuModbusSlave.m_databit);
            debugPrint("PARITY    :",false);debugPrint(iuModbusSlave.m_parity);
        }
        success = false;
    }
    

return success;
}

void Conductor::checkforWiFiConfigurations(){

    // check if configurations are present in the internal flash storage of STM32
    bool validJson = false;
    bool success = true;
    
    if (iuFlash.checkConfig(CONFIG_WIFI_CONFIG_FLASH_ADDRESS) && ! DOSFS.exists("/iuconfig/wifi0.conf") )
    {
        // Read the configurations
        String config = iuFlash.readInternalFlash(CONFIG_WIFI_CONFIG_FLASH_ADDRESS);
        if(debugMode){
            debugPrint("WIFI DEBUG : INTERNAL CONFIG # ",false);
            debugPrint("Intarnal Flash content #:");
            debugPrint(config);
        }
        char* wifiConfiguration =(char*)config.c_str();

        validJson =  processConfiguration(wifiConfiguration,true);
        free(wifiConfiguration);
    }else if (DOSFS.exists("/iuconfig/wifi0.conf"))
    {
        configureFromFlash(IUFlash::CFG_WIFI0);
    }
}

void Conductor::setDefaultMQTT(){
    strncpy((char *)m_mqttServerIp,MQTT_DEFAULT_SERVER_IP,strlen(MQTT_DEFAULT_SERVER_IP));
    m_mqttServerPort = MQTT_DEFAULT_SERVER_PORT;
    strncpy((char *)m_mqttUserName,MQTT_DEFAULT_USERNAME,strlen(MQTT_DEFAULT_USERNAME));
    strncpy((char *)m_mqttPassword,MQTT_DEFAULT_ASSWORD,strlen(MQTT_DEFAULT_ASSWORD));
    // m_tls_enabled = true;
}

void Conductor::setDefaultHTTP(){
    strncpy((char*)m_httpHost,HTTP_DEFAULT_HOST,strlen(HTTP_DEFAULT_HOST));
    m_httpPort = HTTP_DEFAULT_PORT;
    strncpy((char*)m_httpPath,HTTP_DEFAULT_PATH,strlen(HTTP_DEFAULT_PATH));
    strncpy((char*)m_httpUsername,HTTP_DEFAULT_USERNAME,strlen(HTTP_DEFAULT_USERNAME));
    strncpy((char*)m_httpPassword,HTTP_DEFAULT_PASSWORD,strlen(HTTP_DEFAULT_PASSWORD));
    strncpy((char*)m_httpOauth,HTTP_DEFAULT_OUTH,strlen(HTTP_DEFAULT_OUTH));
}

bool Conductor::updateModbusStatus(){

    bool connected = false;
    if(iuModbusSlave.modbusConnectionStatus == true){
            if(debugMode){
                debugPrint("MODBUS DEBUG : MODBUS CONNECTED");
            }
        if(StreamingMode::BLE && isBLEConnected()){    
            iuBluetooth.write("MODBUS-CONNECTED;");
        }
        connected = true;
    }
    else
    {
        if(debugMode){
            debugPrint("MODBUS DEBUG : MODBUS DISCONNECTED");
        }
        if(StreamingMode::BLE && isBLEConnected()){    
            iuBluetooth.write("MODBUS-DISCONNECTED;");
        }
        connected = false;
    }
    
    return connected;
}

void Conductor::updateWiFiHash()
{
    char wifiHash[34];  
    iuOta.otaGetMD5(IUFSFlash::CONFIG_SUBDIR,"wifi0.conf",wifiHash);
    iuWiFi.sendMSPCommand(MSPCommand::SEND_WIFI_HASH,wifiHash);
}

void Conductor::sendConfigRequest()
{
    char response[128];
    double TimeStamp = conductor.getDatetime();
          
    sprintf(response,"%s%s%s%s","{\"DEVICEID\":\"", m_macAddress.toString().c_str(),"\"",",\"CONFIGTYPE\":\"features\"}");
    iuWiFi.sendMSPCommand(MSPCommand::PUBLISH_CONFIG_CHECKSUM, response);
}
#if 0
void Conductor::computeDiagnoticState(JsonVariant &digList)
{
        // uint16_t m_minSpan[maxDiagnosticStates] = {50,40,50};
        // uint16_t m_aleartRepeat[maxDiagnosticStates] = {60,50,60};
        // uint16_t m_maxGap[maxDiagnosticStates] = {30,25,30};
    //getAlertPolicyTime();
    //JsonObject &root = configureJsonFromFlash("/iuRule/diginp.conf", 1); // Expected input { "UNBAL": 1, "MISALIG": 1, "BPFO": 1, ...} 
    JsonObject& root =  digList["DIG"];
    debugPrint("\nDIG LIST :",false);
    root.printTo(Serial);

    bool state[maxDiagnosticStates];
    bool exposeDebugPrints = true;  // Enable this flag to get debugPrints
    int index = 0;
    int resultIndex = 0;
    if(validTimeStamp())  // Waiting for TimeSync to Avoid False Trigger
    {
        // uint32_t now = millis();
        for (auto iterate : root)
        {
            if(exposeDebugPrints){
                debugPrint("____________________________________");
            }
            state[index] = (iterate.value.as<bool>());
            if (state[index])
            {
                if (first_active_flag[index] == false)
                {
                    if (exposeDebugPrints)
                    {
                        debugPrint(iterate.key, false);
                        debugPrint(" : Activated ");
                    }
                    first_active[index] = getDatetime();
                    last_active[index] = getDatetime();
                    first_active_flag[index] = true;
                    last_active_flag[index] = true;
                    reset_alert_flag[index] = false;
                }

                if ((getDatetime() - first_active[index] > m_minSpan[index]) && last_alert_flag[index] == false && first_active_flag[index])
                {
                    last_alert_flag[index] = true;
                    last_alert[index] = getDatetime();
                    diagAlertResults[resultIndex]=iterate.key;
                    ++m_totalDigCount[index];  
                    ++m_activeDigCount[index];
                    iuTrigger.pushToStack(STACK::REPORTABLE_DIG,iterate.key);   // push to R stack
                    if (exposeDebugPrints)
                    {
                        debugPrint(iterate.key, false);
                        debugPrint(" : ALERT");
                        debugPrint(diagAlertResults[resultIndex]);
                    }
                   ++ resultIndex;
                }
                
                if(getDatetime() - last_alert[index] > m_aleartRepeat[index] && last_alert_flag[index])
                {
                    last_alert[index] = getDatetime();
                    diagAlertResults[resultIndex]=iterate.key;
                    ++m_totalDigCount[index];  
                    ++m_activeDigCount[index];
                    iuTrigger.pushToStack(STACK::REPORTABLE_DIG,iterate.key);   // push to R stack
                    if (exposeDebugPrints)
                    {
                        debugPrint(iterate.key, false);
                        debugPrint(" : ALERT REP");
                        debugPrint(diagAlertResults[resultIndex]);
                    }
                    ++resultIndex;
                }
                last_active_flag[index] = true;
                if (exposeDebugPrints)
                {
                    debugPrint(iterate.key, false);
                    debugPrint(" : ", false);
                    debugPrint(state[index]);
                    debugPrint("MIN SPAN : ", false);
                    debugPrint(m_minSpan[index]);
                    debugPrint("ALERT REP : ", false);
                    debugPrint(m_aleartRepeat[index]);
                    debugPrint("MAX GAP : ", false);
                    debugPrint(m_maxGap[index]);
                    debugPrint("Total Active Time : ", false);
                    debugPrint((getDatetime() - first_active[index]));
                    debugPrint("Calc Rep Time : ", false);
                    debugPrint(getDatetime() - last_alert[index]);
                    debugPrint("Diagnostic Active Count :",false);
                    debugPrint(m_activeDigCount[index]);
                    debugPrint("Toatl Diagnostic Active Count :",false);
                    debugPrint(m_totalDigCount[index]);
                    debugPrint("____________________________________");
                }
            }
            else{
                if(last_active_flag[index]){
                    last_active[index] = getDatetime();
                    last_active_flag[index] = false;
                }
                if((getDatetime() - last_active[index] < m_maxGap[index]) && last_active_flag[index] == false && reset_alert_flag[index] == false)
                {
                   last_active[index] = getDatetime();
                   reset_alert_flag[index] = true;
                }else if((getDatetime() - last_active[index] > m_maxGap[index]) && last_active_flag[index] == false && reset_alert_flag[index] == true){
                    first_active_flag[index] = false;
                    last_active_flag[index] = false;
                    last_alert_flag[index] = false;
                    reset_alert_flag[index] = false;
                    m_activeDigCount[index] = 0 ;
                    if (exposeDebugPrints)
                    {
                        debugPrint(iterate.key, false);
                        debugPrint(" : Reset Alert");
                    }
                }
                if (exposeDebugPrints)
                {
                    debugPrint(iterate.key, false);
                    debugPrint(" : ", false);
                    debugPrint(state[index]);
                    debugPrint("MIN SPAN : ", false);
                    debugPrint(m_minSpan[index]);
                    debugPrint("ALERT REP : ", false);
                    debugPrint(m_aleartRepeat[index]);
                    debugPrint("MAX GAP : ", false);
                    debugPrint(m_maxGap[index]);
                    debugPrint("Current Min Gap : ", false);
                    debugPrint(getDatetime() - last_active[index]);
                    debugPrint("Total De-Active Time : ", false);
                    debugPrint(getDatetime() - last_active[index]);
                    debugPrint("Diagnostic Active Count :",false);
                    debugPrint(m_activeDigCount[index]);
                    debugPrint("Toatl Diagnostic Active Count :",false);
                    debugPrint(m_totalDigCount[index]);
                    debugPrint("____________________________________");
                }
            }
            ++index;
        }
        
        debugPrint("Result Array : ",false);
        for(int i=0;i<maxDiagnosticStates;i++){
            debugPrint(diagAlertResults[i],false);
            debugPrint(",",false);
        }
        debugPrint("");
        clearDiagResultArray(); // In actual condition. Need to call this method after Publishing Alert Results 
    }
}
#endif

void Conductor::computeDiagnoticState(String *diagInput, int totalConfiguredDiag)
{
    int resultIndex = 0;
    bool exposeDebugPrints = false;  // Enable this flag to get debugPrints    int resultIndex = 0;
    if(validTimeStamp())  // Waiting for TimeSync to Avoid False Trigger
    {
        for (int index = 0; index< totalConfiguredDiag;index++)
        {
            if(exposeDebugPrints){
                debugPrint("____________________________________");
            }
            // state[index] = (iterate.value.as<bool>());
            if (diagInput[index].endsWith("1"))
            {
                diagInput[index].remove(diagInput[index].length() - 1);
                if (first_active_flag[index] == false)
                {
                    if (exposeDebugPrints)
                    {
                        debugPrint(diagInput[index], false);
                        debugPrint(" : Activated ");
                    }
                    first_active[index] = getDatetime();
                    last_active[index] = getDatetime();
                    first_active_flag[index] = true;
                    last_active_flag[index] = true;
                    reset_alert_flag[index] = false;
                }
                if ((getDatetime() - first_active[index] > m_minSpan[index]) && last_alert_flag[index] == false && first_active_flag[index])
                {
                    last_alert_flag[index] = true;
                    last_alert[index] = getDatetime();
                    alert_repeat_state[resultIndex] = false;
                    diagAlertResults[resultIndex]=(char*)diagInput[index].c_str();
                    reportable_m_id[resultIndex] = getm_id(diagAlertResults[resultIndex],totalConfiguredDiag);
                    reportableDIGID[reportableIndexCounter] = index;   
                    if (exposeDebugPrints)
                    {
                        debugPrint(diagInput[index], false);
                        debugPrint(" : ALERT");
                        debugPrint(diagAlertResults[resultIndex]);
                        debugPrint(reportable_m_id[resultIndex]);
                    }
                   ++ resultIndex;
                   reportableIndexCounter ++; 
                }
                if(getDatetime() - last_alert[index] > m_aleartRepeat[index] && last_alert_flag[index])
                {
                    last_alert[index] = getDatetime();                    
                    alert_repeat_state[resultIndex] = true;
                    diagAlertResults[resultIndex]=(char*) diagInput[index].c_str();
                    reportable_m_id[resultIndex] = getm_id(diagAlertResults[resultIndex],totalConfiguredDiag);
                    reportableDIGID[reportableIndexCounter] = index;
                    if (exposeDebugPrints)
                    {
                        debugPrint(diagInput[index], false);
                        debugPrint(" : ALERT REP");
                        debugPrint(diagAlertResults[resultIndex]);
                        debugPrint(reportable_m_id[resultIndex]);
                    }
                    ++resultIndex;
                    reportableIndexCounter ++;
                }
                last_active_flag[index] = true;
                if (exposeDebugPrints)
                {
                    debugPrint(diagInput[index], false);
                    debugPrint(" : ", false);
                    debugPrint("1");
                    debugPrint("MIN SPAN : ", false);
                    debugPrint(m_minSpan[index]);
                    debugPrint("ALERT REP : ", false);
                    debugPrint(m_aleartRepeat[index]);
                    debugPrint("MAX GAP : ", false);
                    debugPrint(m_maxGap[index]);
                    debugPrint("Total Active Time : ", false);
                    debugPrint((getDatetime() - first_active[index]));
                    debugPrint("Calc Rep Time : ", false);
                    debugPrint(getDatetime() - last_alert[index]);
                    debugPrint("____________________________________");
                }
            }
            else if (diagInput[index].endsWith("0")){
                diagInput[index].remove(diagInput[index].length() - 1);
                if(last_active_flag[index]){
                    last_active[index] = getDatetime();
                    last_active_flag[index] = false;
                }
                if((getDatetime() - last_active[index] < m_maxGap[index]) && last_active_flag[index] == false && reset_alert_flag[index] == false)
                {
                   last_active[index] = getDatetime();
                   reset_alert_flag[index] = true;
                }else if((getDatetime() - last_active[index] > m_maxGap[index]) && last_active_flag[index] == false && reset_alert_flag[index] == true){
                    first_active_flag[index] = false;
                    last_active_flag[index] = false;
                    last_alert_flag[index] = false;
                    reset_alert_flag[index] = false;
                    if (exposeDebugPrints)
                    {
                        debugPrint(diagInput[index], false);
                        debugPrint(" : Reset Alert");
                    }
                }
                if (exposeDebugPrints)
                {
                    debugPrint(diagInput[index], false);
                    debugPrint(" : ", false);
                    debugPrint("0");
                    debugPrint("MIN SPAN : ", false);
                    debugPrint(m_minSpan[index]);
                    debugPrint("ALERT REP : ", false);
                    debugPrint(m_aleartRepeat[index]);
                    debugPrint("MAX GAP : ", false);
                    debugPrint(m_maxGap[index]);
                    debugPrint("Current Min Gap : ", false);
                    debugPrint(getDatetime() - last_active[index]);
                    debugPrint("Total De-Active Time : ", false);
                    debugPrint(getDatetime() - last_active[index]);
                    debugPrint("____________________________________");
                }
            }
        }
        // Display Reportable Diagnostic list        
        if(exposeDebugPrints){
            debugPrint("Result Array : ",false);
            for(int i=0;i<maxDiagnosticStates;i++){
                debugPrint(diagAlertResults[i],false);
                debugPrint(",",false);
            }
            debugPrint("");
            debugPrint("Reportable M_id : ,",false);
            for(int i=0;i<maxDiagnosticStates;i++){
                debugPrint(reportable_m_id[i],false);
                debugPrint(",",false);
            }
            debugPrint("");
        }
        //TODO: Copy reportable_m_id array to temp array
        for(int i=0;i<totalConfiguredDiag;i++){
            modbus_reportable_m_id[i] = reportable_m_id[i];
        }

        //clearDiagResultArray(); // In actual condition. Need to call this method after Publishing Alert Results 
    }
    reportableDIGLength = resultIndex;  // number of reportable diagnostic
    
}

void Conductor::configureAlertPolicy()
{
    JsonObject &diag = configureJsonFromFlash("/iuRule/diagnostic.conf", 1);
    if(diag.success()){
    size_t totalDiagnostics = diag["CONFIG"]["DIG"]["DID"].size();

    // debugPrint("\nTotal No. Of Duiagnostics = ",false);
    // debugPrint(totalDiagnostics);

    for(int i = 0; i < totalDiagnostics; i++){
        m_minSpan[i] =      diag["CONFIG"]["DIG"]["ALTP"]["MINSPN"][i];
        m_aleartRepeat[i] = diag["CONFIG"]["DIG"]["ALTP"]["ALRREP"][i];
        m_maxGap[i] =       diag["CONFIG"]["DIG"]["ALTP"]["MAXGAP"][i];
        m_id[i] = diag["CONFIG"]["DIG"]["MID"][i];
        d_id[i] = diag["CONFIG"]["DIG"]["DID"][i];
    }

    // for(int i = 0; i < totalDiagnostics; i++){
    //     debugPrint("MIN SPAN : ", false);debugPrint(m_minSpan[i]);
    //     debugPrint("ALR REP : ", false);debugPrint(m_aleartRepeat[i]);
    //     debugPrint("MAX GAP : ", false);debugPrint(m_maxGap[i]);
    // }
    }
    else{
        if(debugMode){debugPrint("failed to read diagnostic conf file ");} 
    }
}

void Conductor::clearDiagStateBuffers()
{
    memset(last_active,'\0',sizeof(last_active));
    memset(first_active,'\0',sizeof(first_active));
    memset(last_alert,'\0',sizeof(last_alert));
    memset(first_active_flag,'\0',sizeof(first_active_flag));
    memset(last_active_flag,'\0',sizeof(last_active_flag));
    memset(last_alert_flag,'\0',sizeof(last_alert_flag));
    memset(reset_alert_flag,'\0',sizeof(reset_alert_flag));
    memset(m_id, '\0', sizeof(m_id));
    memset(d_id, '\0', sizeof(d_id));
}

void Conductor::clearDiagResultArray(){
    memset(diagAlertResults,'\0',sizeof(diagAlertResults));
    memset(alert_repeat_state,'\0',sizeof(alert_repeat_state));
    memset(reportable_m_id, '\0', sizeof(reportable_m_id));
}

int Conductor::getTotalDigCount(const char* diagName){

 
}
int Conductor::getActiveDigCount(const char* diagName){

}
        
char* Conductor::GetStoredMD5(IUFlash::storedConfig configType, JsonObject &inputConfig)
{
    char hash[40];

    if(inputConfig.containsKey("CONFIG_HASH"))
    {
        switch (configType)
        {
        case IUFlash::CFG_DEVICE:
            strcpy(hash,inputConfig["CONFIG_HASH"]["device"]);
            break;
        case IUFlash::CFG_FEATURE:
            strcpy(hash,inputConfig["CONFIG_HASH"]["features"]);
            break;
        default:
            break;
        }
        return hash;
    }else{
        return "error";
    }
    
}

void Conductor::mergeJson(JsonObject& dest, const JsonObject& src) {
   for (auto kvp : src) {
     dest[kvp.key] = kvp.value;
   }
}

bool Conductor::validTimeStamp(){
    if(getDatetime() > 1600000000.00){
        return true;
    }else{
       return false;
    }
}

uint8_t Conductor::getm_id(char* did,  int totalConfiguredDiag){
    for(int i = 0 ; i < totalConfiguredDiag ; i++){
        if(strcmp(did,d_id[i]) == 0){
            return m_id[i];
        }
    }
}

void Conductor::addAdvanceFeature(JsonObject& destJson, uint8_t index , String* id, float* value){
    for(size_t i=0;i<index;i++){
        destJson[id[i]] = value[i];
    }        
}