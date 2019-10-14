#include<string.h>
#include "Conductor.h"
#include "rBase64.h"
#include "FFTConfiguration.h"
#include "RawDataState.h"
#include <MemoryFree.h>
#include "stm32l4_iap.h"
extern "C" char FW_Valid_State;

const char* fingerprintData;
const char* fingerprints_X;
const char* fingerprints_Y;
const char* fingerprints_Z;

int m_temperatureOffset;
int m_audioOffset;
        
char Conductor::START_CONFIRM[11] = "IUOK_START";
char Conductor::END_CONFIRM[9] = "IUOK_END";

IUFlash::storedConfig Conductor::CONFIG_TYPES[Conductor::CONFIG_TYPE_COUNT] = {
    IUFlash::CFG_DEVICE,
    IUFlash::CFG_COMPONENT,
    IUFlash::CFG_FEATURE,
    IUFlash::CFG_OP_STATE};


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
    StaticJsonBuffer<JSON_BUFFER_SIZE> jsonBuffer;
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
    }
    return success;
}

/**
 *
 */
void Conductor::sendConfigChecksum(IUFlash::storedConfig configType)
{
    if (!(m_streamingMode == StreamingMode::WIFI ||
          m_streamingMode == StreamingMode::WIFI_AND_BLE)) {
        if (debugMode) {
            debugPrint("Config checksum can only be sent via WiFi.");
        }
        return;
    }
    size_t configMaxLen = 200;
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
    size_t fullStrLen = md5Len + 44;
    char fullStr[fullStrLen];
    for (size_t i = 0; i < fullStrLen; i++) {
        fullStr[i] = 0;
    }
    int written = -1;
    switch (configType) {
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
        case IUFlash::CFG_OP_STATE:
            written = snprintf(
                fullStr, fullStrLen, "{\"mac\":\"%s\",\"opState\":\"%s\"}",
                m_macAddress.toString().c_str(), md5str);
            break;
        default:
            if (debugMode) {
                debugPrint("Unhandled config type: ", false);
                debugPrint((uint8_t) configType);
            }
            break;
    }
    if (written > 0 && written < fullStrLen) {
        // Send checksum
        iuWiFi.sendMSPCommand(MSPCommand::PUBLISH_CONFIG_CHECKSUM, fullStr,
                              written);
    } else if (loopDebugMode) {
        debugPrint(F("Failed to format config checksum: "), false);
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
         m_streamingMode == StreamingMode::WIFI_AND_BLE) {
        uint32_t now = millis();
        if (now - m_configTimerStart > SEND_CONFIG_CHECKSUM_TIMER) {
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
 * Parse the given config json and apply the configuration
 */
bool Conductor::processConfiguration(char *json, bool saveToFlash)
{
    // String to hold the result of validation of the config json
    char validationResultString[400];

    //StaticJsonBuffer<JSON_BUFFER_SIZE> jsonBuffer;            // make it dynamic
    //const size_t bufferSize = JSON_ARRAY_SIZE(2) + JSON_OBJECT_SIZE(3) + 60;        // dynamically allociated memory
    const size_t bufferSize = JSON_OBJECT_SIZE(1) + 41*JSON_OBJECT_SIZE(5) + JSON_OBJECT_SIZE(41) + 2430;
    DynamicJsonBuffer jsonBuffer(bufferSize);
    //Serial.print("JSON 1 SIZE :");Serial.println(bufferSize);
    
    JsonObject& root = jsonBuffer.parseObject(json);
    String jsonChar;
    root.printTo(jsonChar);
    
    JsonVariant variant = root;
    char ack_configEthernet[200];
  
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
            char ack_config[150];
            
            snprintf(ack_config, 150, "{\"messageId\":\"%s\",\"macId\":\"%s\"}", messageId,m_macAddress.toString().c_str());
            
            //Serial.println(ack_config);
            if(iuWiFi.isWorking()){
                iuWiFi.sendMSPCommand(MSPCommand::RECEIVE_DIAGNOSTIC_ACK, ack_config);
            }else if (!iuEthernet.isEthernetConnected && StreamingMode::ETHERNET)    // Ethernet is connected
            {       debugPrint("Sending Fetures ACK over Ethernet");
                    snprintf(ack_configEthernet, 200, "{\"deviceId\":\"%s\",\"transport\":%d,\"messageType\":%d,\"payload\": \"{\\\"macId\\\":\\\"%s\\\",\\\"messageId\\\":\\\"%s\\\"}\"}",
                      m_macAddress.toString().c_str(),0, 2, m_macAddress.toString().c_str(),messageId);
                   
                    debugPrint(ack_configEthernet,true);
                    iuEthernet.write(ack_configEthernet); 
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
            char ack_config[150];
            snprintf(ack_config, 150, "{\"messageId\":\"%s\",\"macId\":\"%s\"}", messageId,m_macAddress.toString().c_str());
            
            //Serial.println(ack_config);
            if(iuWiFi.isWorking()){
                 iuWiFi.sendMSPCommand(MSPCommand::RECEIVE_DIAGNOSTIC_ACK, ack_config);
            }else if (!iuEthernet.isEthernetConnected && StreamingMode::ETHERNET)    // Ethernet is connected
            {       debugPrint("Sending Fingerpritns Threshold ACK over Ethernet");
                    snprintf(ack_configEthernet, 200, "{\"deviceId\":\"%s\",\"transport\":%d,\"messageType\":%d,\"payload\": \"{\\\"macId\\\":\\\"%s\\\",\\\"messageId\\\":\\\"%s\\\"}\"}",
                      m_macAddress.toString().c_str(),0, 2, m_macAddress.toString().c_str(),messageId);
                   
                    debugPrint(ack_configEthernet,true);
                    iuEthernet.write(ack_configEthernet); 
                    iuEthernet.write("\n");
                   
            } 
        }
    }
     // MQTT Server configuration
    subConfig = root["mqtt"];
    if (subConfig.success()) {
        //configureAllFeatures(subConfig);
        bool dataWritten = false;
        
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
            
          
            char ack_config[150];
            snprintf(ack_config, 150, "{\"messageId\":\"%s\",\"macId\":\"%s\"}", messageId,m_macAddress.toString().c_str());

            if(iuWiFi.isWorking()){
                 iuWiFi.sendMSPCommand(MSPCommand::RECEIVE_DIAGNOSTIC_ACK, ack_config); 
            }
            else if (!iuEthernet.isEthernetConnected && StreamingMode::ETHERNET)    // Ethernet is connected
            {       debugPrint("Sending Fingerprints ACK over Ethernet");
                    snprintf(ack_configEthernet, 200, "{\"deviceId\":\"%s\",\"transport\":%d,\"messageType\":%d,\"payload\": \"{\\\"macId\\\":\\\"%s\\\",\\\"messageId\\\":\\\"%s\\\"}\"}",
                      m_macAddress.toString().c_str(),0, 2, m_macAddress.toString().c_str(),messageId);
                   
                    debugPrint(ack_configEthernet,true);
                    iuEthernet.write(ack_configEthernet); 
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
          //configureBoardFromFlash("httpConfig.conf",dataWritten);
          JsonObject& config = configureJsonFromFlash("httpConfig.conf",1);

           const char* messageId = config["messageId"];
          //Serial.print("File Content :");Serial.println(jsonChar);
          //Serial.print("http details :");Serial.print(m_httpHost);Serial.print(",");Serial.print(m_httpPort);Serial.print(",");Serial.print(m_httpPath);Serial.println("/***********/");
          //iuWiFi.sendMSPCommand(MSPCommand::SET_RAW_DATA_ENDPOINT_HOST,m_httpHost); 
          //iuWiFi.sendMSPCommand(MSPCommand::SET_RAW_DATA_ENDPOINT_PORT,String(m_httpPort).c_str()); 
          //iuWiFi.sendMSPCommand(MSPCommand::SET_RAW_DATA_ENDPOINT_ROUTE,m_httpPath); 

          char httpConfig_ack[150];
          snprintf(httpConfig_ack, 150, "{\"messageId\":\"%s\",\"macId\":\"%s\"}", messageId,m_macAddress.toString().c_str());
            
          debugPrint(F("httpConfig ACK :"));debugPrint(httpConfig_ack);
          if (iuWiFi.isWorking())
          {
              iuWiFi.sendMSPCommand(MSPCommand::RECEIVE_HTTP_CONFIG_ACK, httpConfig_ack);
          
          }else if (!iuEthernet.isEthernetConnected && StreamingMode::ETHERNET)    // Ethernet is connected
          {     
                snprintf(ack_configEthernet, 200, "{\"deviceId\":\"%s\",\"transport\":%d,\"messageType\":%d,\"payload\": \"{\\\"macId\\\":\\\"%s\\\",\\\"messageId\\\":\\\"%s\\\"}\"}",
                      m_macAddress.toString().c_str(),0, 2, m_macAddress.toString().c_str(),messageId);
                   
                    debugPrint(ack_configEthernet,true);
                    iuEthernet.write(ack_configEthernet); 
                    iuEthernet.write("\n");
                //iuEthernet.write(httpConfig_ack);
          }
          
          
          
          //stm reset
          delay(10);
          if(subConfig = root["httpConfig"]["host"] != m_httpHost ){
                STM32.reset();
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
                STM32.reset();
            }
        } else {
            if(loopDebugMode) debugPrint("Received invalid FFT configuration");

            // Acknowledge incorrect configuration, send the errors on /ide_plus/command_response topic
            // If streaming mode is BLE, send an acknowledgement on BLE as well
            iuWiFi.sendMSPCommand(MSPCommand::CONFIG_ACK, validationResultString);
            if(m_streamingMode == StreamingMode::BLE && isBLEConnected()) { iuBluetooth.write("FFT_CFG_FAILURE;"); delay(100); }
        }
    } // If json is incorrect, it will result in parsing error in jsonBuffer.parseObject(json) which will cause the processConfiguration call to return
 
    return true;
}

/*
 * Read the MQTT Configutation details
 * 
 */

 void Conductor::configureMQTTServer(String filename){

  // Open the configuration file
  IPAddress tempAddress;
  
  File myFile = DOSFS.open(filename,"r");
  
  
  StaticJsonBuffer<512> jsonBuffer;

  // Parse the root object
  JsonObject &root = jsonBuffer.parseObject(myFile);

  if (!root.success()){
    debugPrint(F("Failed to read MQTT.conf file, using default configuration"));
    m_mqttServerIp = MQTT_DEFAULT_SERVER_IP;
    m_mqttServerPort = MQTT_DEFAULT_SERVER_PORT;
    m_mqttUserName = MQTT_DEFAULT_USERNAME;
    m_mqttPassword = MQTT_DEFAULT_ASSWORD;
    //m_accountid = "XXXAdmin";
  }
 else {
  
  
  
  String mqttServerIP = root["mqtt"]["mqttServerIP"];
  int mqttport = root["mqtt"]["port"];
  
  debugPrint("INside MQTT.conf .......");
  m_mqttServerIp.fromString(mqttServerIP);//mqttServerIP;
  m_mqttServerPort = mqttport;
  m_mqttUserName = root["mqtt"]["username"]; //MQTT_DEFAULT_USERNAME;
  m_mqttPassword = root["mqtt"]["password"]; //MQTT_DEFAULT_ASSWORD;
  m_accountId = root["accountid"];
  
  //mqttusername = MQTT_DEFAULT_USERNAME;
/*
  Serial.println("Before Swap :");
  Serial.print("UserName :");Serial.println( userName);
  Serial.print("Password :");Serial.println( password);
  
  fastSwap (&mqttusername, &userName); 
  fastSwap (&mqttpassword, &password);
  
  Serial.println("After Swap :");
  Serial.print("UserName :");Serial.println( userName);
  Serial.print("Password :");Serial.println( password);

  m_mqttUserName = userName;
  m_mqttPassword = password;
*/  
//  iuWiFi.hardReset();
  if (debugMode) {
        debugPrint(F("MQTT ServerIP :"),false);
        debugPrint(m_mqttServerIp);
        debugPrint(F("Mqtt Port :"),false);
        debugPrint(m_mqttServerPort);
        debugPrint(F("Mqtt UserName :"),false);
        debugPrint(m_mqttUserName);
        debugPrint(F("Mqtt Password :"),false);
        debugPrint(m_mqttPassword);
        debugPrint(F("Account ID :"));
        debugPrint(m_accountId);
  }   
 
 }
  myFile.close();
  
}
/*
 * swap credentails
 */
void Conductor::fastSwap (const char **i, const char **d)
{
    const char *t = *d;
    *d = *i;
    *i = t;
}

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
  
  StaticJsonBuffer<1024> jsonBuffer;

  // Parse the root object
  JsonObject &root = jsonBuffer.parseObject(myFile);
  
  JsonObject& root2 = root["httpConfig"];
  if (!root.success()){
    debugPrint(F("Failed to read httpConf.conf file, using default configuration"));
    m_httpHost = "13.232.122.10";
    m_httpPort = 8080;
    m_httpPath = "/iu-web/rawaccelerationdata";
   
  }
 else {

  // Read configuration from the file

static const char* host = root2["host"];
static uint16_t    port = root2["port"];
static const char* path = root2["path"];
static const char* username = root2["username"];
static const char* password = root2["password"];
static const char* oauth = root2["oauth"];

m_httpHost  = host;
m_httpPort = port;
m_httpPath = path;
m_httpUsername = username;
m_httpPassword = password;
m_httpOauth = oauth;

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
  const size_t bufferSize = JSON_OBJECT_SIZE(1) + 45*JSON_OBJECT_SIZE(5) + JSON_OBJECT_SIZE(45) + 2430;
  
  DynamicJsonBuffer jsonBuffer(bufferSize);
 // Serial.print("JSON 2 SIZE :");Serial.println(bufferSize);
 // Parse the root object
  JsonObject &root = jsonBuffer.parseObject(myFile);
  //JsonObject& root2 = root["fingerprints"];
  
  if (!root.success()){
    debugPrint(F("Failed to read file, using default configuration"));
   
  }
 else {
  // close file
 // Serial.println("Closing the fingerprints.conf file.....");
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
   // Serial.println(buff);
    
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
            break;
        case 'I': // ping device
            if (strcmp(buff, "IDE-HARDRESET") == 0)
            {
                if (isBLEConnected()) {
                    iuBluetooth.write("WIFI-DISCONNECTED;");
                }
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
                if (tempAddress.fromString(&buff[12])) {
                    m_mqttServerIp = tempAddress;     // temp adress ?
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
                }
            }
        case '3':  // Collect acceleration raw data
            if (buff[7] == '0' && buff[9] == '0' && buff[11] == '0' &&
                buff[13] == '0' && buff[15] == '0' && buff[17] == '0')
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
    //Serial.print("Leagacy CMD Input :");Serial.println(buff);
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
                for (uint8_t i = 0; i < 6; i++) {
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
                    iuAccelerometer.setSamplingRate(samplingRate);
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
                  //Serial.println("START CUSTOM.....1");
                  changeUsageMode(UsageMode::CUSTOM);   // switch to CUSTOM usage mode
                  //Serial.println("START CUSTOM.....2");
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
                if (strcmp(buff, "IUGET_HTTP_CONFIG") == 0)
                {
                    if (DOSFS.exists("httpConfig.conf")){
                    const char* _httpHost;
                    const char* _httpPort;
                    const char* _httpPath;
                   
                    JsonObject& config = configureJsonFromFlash("httpConfig.conf",1);
                    _httpHost = config["httpConfig"]["host"];
                    _httpPort = config["httpConfig"]["port"];
                    _httpPath = config["httpConfig"]["path"];


                    iuUSB.port->println("*****HTTP_CONFIG*****");
                    iuUSB.port->print("HTTP_HOST : ");
                    iuUSB.port->println(_httpHost);
                    iuUSB.port->print("HTTP_PORT : ");
                    iuUSB.port->println(_httpPort);
                    iuUSB.port->print("HTTP_PATH : ");
                    iuUSB.port->println(_httpPath);
                  }else{
                      debugPrint("httpConfig.conf file does not exists");
                  }
                }
                if (strcmp(buff, "IUGET_MQTT_CONFIG") == 0)
                {
                    if  (DOSFS.exists("MQTT.conf")){
                    const char* _serverIP;
                    const char* _serverPort;
                    const char* _UserName;
                    const char* _Password;
                    JsonObject& config = configureJsonFromFlash("MQTT.conf",1);
                    _serverIP = config["mqtt"]["mqttServerIP"];
                    _serverPort = config["mqtt"]["port"];
                    _UserName = config["mqtt"]["username"];
                    _Password = config["mqtt"]["password"];

                    iuUSB.port->println("*****MQTT_CONFIG*****");
                    iuUSB.port->print("MQTT_SERVER_IP : ");
                    iuUSB.port->println(_serverIP);
                    iuUSB.port->print("MQTT_PORT : ");
                    iuUSB.port->println(_serverPort);
                    iuUSB.port->print("MQTT_USERNAME : ");
                    iuUSB.port->println(_UserName);
                    iuUSB.port->print("MQTT_PASSWORD : ");
                    iuUSB.port->println(_Password);
                  }else
                  {
                      debugPrint("MQTT.conf file does not exists");
                  }
                  
                }  
                if (strcmp(buff, "IUGET_FFT_CONFIG") == 0) {
                    iuUSB.port->print("FFT: Sampling Rate: ");
                    iuUSB.port->println(FFTConfiguration::currentSamplingRate);
                    iuUSB.port->print("FFT: Block Size: ");
                    iuUSB.port->println(FFTConfiguration::currentBlockSize);
                }
                break;
            case UsageMode::CUSTOM:
                if (strcmp(buff, "IUEND_DATA") == 0) {
                    iuUSB.port->println(END_CONFIRM);
                    //Serial.println("End CUSTOM ....");
                    changeUsageMode(UsageMode::OPERATION);    //back to Operation Mode
                   return; 
                }  
                result = strstr(buff, "Arange");
                if (result != NULL) {
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
                    iuAccelerometer.setSamplingRate(samplingRate);
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
    uint8_t idx = 0;
    switch (iuWiFi.getMspCommand()) {
        case MSPCommand::ESP_DEBUG_TO_STM_HOST:
            Serial.write(buff);
            Serial.write('\n');
            break;
        // MSP Status messages
        case MSPCommand::MSP_INVALID_CHECKSUM:
            if (loopDebugMode) { debugPrint(F("MSP_INVALID_CHECKSUM")); }
            Serial.println("Invalid Checksum !");
            break;
        case MSPCommand::MSP_TOO_LONG:
            if (loopDebugMode) { debugPrint(F("MSP_TOO_LONG")); }
            break;
        case MSPCommand::RECEIVE_WIFI_FV:{
            if (loopDebugMode) { debugPrint(F("RECEIVE_WIFI_FV")); }
            strncpy(iuWiFi.espFirmwareVersion, buff, 6);
            iuWiFi.espFirmwareVersionReceived = true;
        }
        case MSPCommand::ASK_BLE_MAC:
            if (loopDebugMode) { debugPrint(F("ASK_BLE_MAC")); }
            iuWiFi.sendBleMacAddress(m_macAddress);
            break;
	      case MSPCommand::ASK_HOST_FIRMWARE_VERSION:
            if (loopDebugMode){ debugPrint(F("ASK_HOST_FIRMWARE_VERSION")); }
            iuWiFi.sendHostFirmwareVersion(FIRMWARE_VERSION);               
            break;
        case MSPCommand::ASK_HOST_SAMPLING_RATE:        
            if (loopDebugMode){ debugPrint(F("ASK_HOST_SAMPLING_RATE")); }
            iuWiFi.sendHostSamplingRate(FFTConfiguration::currentSamplingRate);    
            break;
        case MSPCommand::ASK_HOST_BLOCK_SIZE:
            if (loopDebugMode){ debugPrint(F("ASK_HOST_BLOCK_SIZE")); }
            iuWiFi.sendHostBlockSize(FFTConfiguration::currentBlockSize);
            break;
        case MSPCommand::HTTP_ACK: {
            if (loopDebugMode){ debugPrint(F("HTTP_ACK")); }
            if (buff[0] == 'X') {
                httpStatusCodeX = atoi(&buff[1]);
            } else if (buff[0] == 'Y') {
                httpStatusCodeY = atoi(&buff[1]);
            } else if (buff[0] == 'Z') {
                httpStatusCodeZ = atoi(&buff[1]);
            }
            if (loopDebugMode) {
                debugPrint("HTTP status codes X | Y | Z ",false);
                debugPrint(httpStatusCodeX, false);debugPrint(" | ", false);
                debugPrint(httpStatusCodeY, false);debugPrint(" | ", false);
                debugPrint(httpStatusCodeZ, true);
            }
#if 1
            if(httpStatusCodeX != 200 || (httpStatusCodeX == 200 && httpStatusCodeY != 200 && httpStatusCodeY != 0) || (httpStatusCodeX == 200 && httpStatusCodeY == 200 && httpStatusCodeZ != 200 && httpStatusCodeZ != 0))
            {
                Serial.println("Aborting Raw Data Send !");
                Serial.print(httpStatusCodeX);
                Serial.print(" | ");
                Serial.print(httpStatusCodeY);
                Serial.print(" | ");
                Serial.println(httpStatusCodeZ);
                // Abort transmission on failure  // IDE1.5_PORT_CHANGE Change
                RawDataState::rawDataTransmissionInProgress = false;
                RawDataState::startRawDataCollection = false;             
            }
#endif
            break;
        }
        case MSPCommand::WIFI_ALERT_CONNECTED:
            if (loopDebugMode) { debugPrint(F("WIFI-CONNECTED;")); }
            if (isBLEConnected()) {
                iuBluetooth.write("WIFI-CONNECTED;");
            }
            break;
        case MSPCommand::WIFI_ALERT_DISCONNECTED:
            if (isBLEConnected()) {
                iuBluetooth.write("WIFI-DISCONNECTED;");
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
            if (loopDebugMode) { debugPrint(F("SET_DATETIME")); }
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
        case MSPCommand::GET_RAW_DATA_ENDPOINT_INFO:
            // TODO: Implement
            { 
              if(DOSFS.exists("httpConfig.conf")) {  
                JsonObject& config = configureJsonFromFlash("httpConfig.conf",1);
              
                m_httpHost = config["httpConfig"]["host"];
                m_httpPath = config["httpConfig"]["path"];
                m_httpPort = config["httpConfig"]["port"];
              }  
              else{
                m_httpHost = "13.232.122.10";                                       //"ideplus-dot-infinite-uptime-1232.appspot.com";
                m_httpPort =  8080;                                                        //80;
                m_httpPath = "/iu-web/rawaccelerationdata";           //"/raw_data?mac="; 
                }
                iuWiFi.sendMSPCommand(MSPCommand::SET_RAW_DATA_ENDPOINT_HOST,m_httpHost); 
                iuWiFi.sendMSPCommand(MSPCommand::SET_RAW_DATA_ENDPOINT_PORT,String(m_httpPort).c_str()); 
                iuWiFi.sendMSPCommand(MSPCommand::SET_RAW_DATA_ENDPOINT_ROUTE,m_httpPath);
                            
                break;
           }
            
        case MSPCommand::GET_MQTT_CONNECTION_INFO:
            if (loopDebugMode) { debugPrint(F("GET_MQTT_CONNECTION_INFO")); }
            {
            JsonObject& config = configureJsonFromFlash("MQTT.conf",1);

            //m_mqttServerIp = config["mqtt"]["mqttServerIP"];
            //m_mqttServerPort = config["mqtt"]["port"];
            m_mqttUserName = config["mqtt"]["username"];
            m_mqttPassword = config["mqtt"]["password"];
            m_accountId = config["accountid"];
            //Serial.print("Account ID. 1... :");Serial.println(m_accountId);
            //Serial.println("MQTT DEtails :"); Serial.print("IP:");Serial.println(m_mqttServerIp);Serial.print("PORT:");Serial.println(m_mqttServerPort);
            //Serial.print("USERNAME :");Serial.println(m_mqttUserName); Serial.print("PASSWORD:");Serial.println(m_mqttPassword);
            if(m_mqttUserName == NULL || m_mqttPassword == NULL || m_mqttServerPort == NULL){
              // load default configurations
              //m_mqttServerIp = MQTT_DEFAULT_SERVER_IP;
              m_mqttServerPort = MQTT_DEFAULT_SERVER_PORT;
              m_mqttUserName = MQTT_DEFAULT_USERNAME;
              m_mqttPassword = MQTT_DEFAULT_ASSWORD;
            }

            //Serial.print("UserName :");Serial.println(m_mqttUserName);
            //Serial.print("Password 1 :");Serial.println(m_mqttPassword);
            
            iuWiFi.mspSendIPAddress(MSPCommand::SET_MQTT_SERVER_IP,
                                    m_mqttServerIp);
            iuWiFi.sendMSPCommand(MSPCommand::SET_MQTT_SERVER_PORT,
                                  String(m_mqttServerPort).c_str());
            iuWiFi.sendMSPCommand(MSPCommand::SET_MQTT_USERNAME,
                                  m_mqttUserName);// MQTT_DEFAULT_USERNAME);
            iuWiFi.sendMSPCommand(MSPCommand::SET_MQTT_PASSWORD,
                                  m_mqttPassword); //MQTT_DEFAULT_ASSWORD);
            
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
            
            break;
        case MSPCommand::SET_PENDING_HTTP_CONFIG:
            {
             //Serial.print("HTTP Pending Response ..............................................:");
             //Serial.println(buff);
             // create the JSON objects 
             DynamicJsonBuffer jsonBuffer;
             JsonObject& pendingConfigObject = jsonBuffer.parseObject(buff); 
             size_t msgLen = strlen(buff);

             //Serial.print("Size of buff : ");Serial.println(msgLen);
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
            iuAccelerometer.resetScale();
            msg = "operation";
            break;
        case UsageMode::CUSTOM:
            ledManager.overrideColor(RGB_CYAN);
            //configureGroupsForOperation();
            //iuAccelerometer.resetScale();
            msg = "custom";
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
    //Serial.println("BBBBBBBBBBBBBB");
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
    bool force = false;
    // If EXPERIMENT mode, send last data batch before collecting the new data
    if (m_usageMode == UsageMode::EXPERIMENT) {
        if (loopDebugMode && (m_acquisitionMode != AcquisitionMode::RAWDATA ||
                              m_streamingMode != StreamingMode::WIRED)) {
            debugPrint(F("EXPERIMENT should be RAW DATA + USB mode."));
        }
    if (inCallback) {
            iuI2S.sendData(iuUSB.port);             // raw audio data 
            iuAccelerometer.sendData(iuUSB.port);   // raw accel data
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

          //Serial.print("Audio :");Serial.println(aucostic);
          snprintf(rawData,50,"%04.3f,%04.3f,%04.3f,%.3f",acceleration[0],acceleration[1],acceleration[2],aucostic);
          
          String payload = "";
          payload = "$";
          payload += m_macAddress.toString().c_str();
          payload += ",";
          payload += rawData;
          payload += "#";
          
          //Serial.println(payload);
          iuUSB.port->write(payload.c_str());
          
          //Serial.print(features[0],4);Serial.print(",");Serial.print(features[1],4);Serial.print(",");Serial.println(features[2],4);
          //Serial.print("Data:");Serial.println(rawAccel);
       }
            
        force = true;
    }
    // Collect the new data
    for (uint8_t i = 0; i < Sensor::instanceCount; ++i) {
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
    for (uint8_t i = 0; i < FeatureGroup::instanceCount; ++i) {
        // TODO Switch to new streaming format once the backend is ready
        if (ser1) {
            if (m_streamingMode == StreamingMode::WIFI ||
                m_streamingMode == StreamingMode::WIFI_AND_BLE || m_streamingMode == StreamingMode::ETHERNET)
            { 
                  //Serial.print("@@@@@");
//                FeatureGroup::instances[i]->bufferAndStream(
//                    ser1, IUSerial::MS_PROTOCOL, m_macAddress,
//                    ledManager.getOperationState(), batteryLoad, timestamp,
//                    sendFeatureGroupName1);
                FeatureGroup::instances[i]->bufferAndQueue(
                    &sendingQueue, IUSerial::MS_PROTOCOL, m_macAddress,
                    ledManager.getOperationState(), batteryLoad, timestamp,
                    sendFeatureGroupName1);
            } else {
                
                //Serial.print("1234454667674534");
                FeatureGroup::instances[i]->legacyStream(ser1, m_macAddress,
                    ledManager.getOperationState(), batteryLoad, timestamp,
                    sendFeatureGroupName1);
            }
        }
        if (ser2) {
            //Serial.print("SER222222222222222222222222222222222222222");
            FeatureGroup::instances[i]->legacyStream(ser2, m_macAddress,
                ledManager.getOperationState(), batteryLoad, timestamp,
                sendFeatureGroupName2, 1);
        }
//        FeatureGroup::instances[i]->bufferAndQueue(
//            &sendingQueue, IUSerial::MS_PROTOCOL, m_macAddress,
//            ledManager.getOperationState(), batteryLoad, timestamp,
//            true);
    }
    CharBufferNode *nodeToSend = sendingQueue.getNextBufferToSend();
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
        uint16_t maxLen = 15000;   //3500
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
 * TODO : Implement a retry mechanism.
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
        Serial.println(F("Raw data request: starting transmission"));
        RawDataState::rawDataTransmissionInProgress = true;
        httpStatusCodeX = httpStatusCodeY = httpStatusCodeZ = 0;
        XSentToWifi = YsentToWifi = ZsentToWifi = false;    
    }

    if (RawDataState::rawDataTransmissionInProgress) {
        // double timeSinceLastSentToESP = millis() - lastPacketSentToESP; // use later for retry mechanism
        if (!XSentToWifi) {
            prepareRawDataPacketAndSend('X');
            XSentToWifi = true; 
            RawDataTimeout = millis(); // IDE1.5_PORT_CHANGE
            if(loopDebugMode) {
                debugPrint("Raw data request: X sent to wifi");
            }
            Serial.println(F("Raw data request: X sent to wifi"));
            // lastPacketSentToESP = millis();
        } else if (httpStatusCodeX == 200 && !YsentToWifi) { 
            prepareRawDataPacketAndSend('Y');
            YsentToWifi = true;
            RawDataTimeout = millis(); // IDE1.5_PORT_CHANGE
            if(loopDebugMode) {
                debugPrint("Raw data request: X delivered, Y sent to wifi");
            }
            Serial.println(F("Raw data request: X delivered, Y sent to wifi"));
            // lastPacketSentToESP = millis();
        } else if (httpStatusCodeY == 200 && !ZsentToWifi) {
            prepareRawDataPacketAndSend('Z');
            ZsentToWifi = true;
            RawDataTimeout = millis(); // IDE1.5_PORT_CHANGE
            if(loopDebugMode) {
                debugPrint("Raw data request: Y delivered, Z sent to wifi");
            }
            Serial.println(F("Raw data request: Y delivered, Z sent to wifi")); 
            // lastPacketSentToESP = millis();
        }
        if (httpStatusCodeX == 200 && httpStatusCodeY == 200 && httpStatusCodeZ == 200) {
            // End the transmission session, reset RawDataState::startRawDataCollection and RawDataState::rawDataTransmissionInProgress
            // Rest of the tracking variables are reset when rawDataRequest() is called
            if(loopDebugMode) {
                debugPrint("Raw data request: Z delivered, ending transmission session");
            }
            Serial.println(F("Raw data request: Z delivered, ending transmission session"));
            RawDataState::startRawDataCollection = false;
            RawDataState::rawDataTransmissionInProgress = false;    
        }
        if((millis() - RawDataTimeout) > 10000)
        { // IDE1.5_PORT_CHANGE -- On timeout of 4 Sec. if no response OK/FAIL then abort transmission
            Serial.println("Raw Data Send Timeout !!");
            RawDataState::startRawDataCollection = false;
            RawDataState::rawDataTransmissionInProgress = false;              
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
    iuWiFi.sendLongMSPCommand(MSPCommand::SEND_RAW_DATA, 3000000,
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
    if (now - m_rawDataPublicationStart > m_rawDataPublicationTimer) {
        Serial.println(F("***  Sending Raw Data ***"));
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
                        
                        
                }else if(m_streamingMode == StreamingMode::WIFI || m_streamingMode == StreamingMode::WIFI_AND_BLE)//iuWiFi.isAvailable() && iuWiFi.isWorking())   
                {   /* FingerPrintResult send over Wifi only */
                    debugPrint("Wifi connected, ....",true);
                    if(RawDataState::rawDataTransmissionInProgress == false)
                    { 
                        iuWiFi.sendMSPCommand(MSPCommand::SEND_DIAGNOSTIC_RESULTS,FingerPrintResult );    
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
    }   
}

bool Conductor::setFFTParams() {
    bool configured = false;
    JsonObject& config = configureJsonFromFlash("/iuconfig/fft.conf", false);
    if(config.success()) {
        FFTConfiguration::currentSamplingRate = config["samplingRate"];
        FFTConfiguration::currentBlockSize = config["blockSize"];
        // TODO: The following can be configurable in the future
        FFTConfiguration::currentLowCutOffFrequency = FFTConfiguration::DEFALUT_LOW_CUT_OFF_FREQUENCY;
        FFTConfiguration::currentHighCutOffFrequency = FFTConfiguration::currentSamplingRate / FMAX_FACTOR;
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

        // Change the sensor sampling rate 
        // timerISRPeriod = (samplingRate == 1660) ? 600 : 300;  // 1.6KHz->600, 3.3KHz->300
        // timerISRPeriod = int(1000000 / FFTConfiguration::currentSamplingRate); // +1 to ensure that sensor has captured data before mcu ISR gets it, for edge case
        iuAccelerometer.setSamplingRate(FFTConfiguration::currentSamplingRate); // will set the ODR for the sensor
        if(setupDebugMode) {
            config.prettyPrintTo(Serial);
        }
        configured = true;
    } else {
        if(loopDebugMode) debugPrint("Failed to read fft.conf file");
        configured = false;
    }
    return configured;
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

void Conductor::setConductorMacAddress() {
    if(iuBluetooth.isBLEAvailable){
        iuBluetooth.enterATCommandInterface();
        char BLE_MAC_Address[20];
        iuBluetooth.sendATCommand("mac?", BLE_MAC_Address, 20);
        debugPrint("BLE MAC ID:",false);debugPrint(BLE_MAC_Address,true);
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

void Conductor::setThresholdsFromFile() 
{
    StaticJsonBuffer<JSON_BUFFER_SIZE> jsonBuffer;
    JsonVariant config = JsonVariant(
            iuFlash.loadConfigJson(IUFlash::CFG_FEATURE, jsonBuffer));
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
        computeFeatures();                                      //compute current state with these thresholds
    }
    else {
        debugPrint("Threshold file read was not successful.");
    }
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
        //Serial.println("READING FROM STARTUP COMPLETE...");   

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
uint32_t Conductor::FW_Validation()
{
    bool WiFiCon = false;

    if(FW_Valid_State == 0) {
        Serial.println(F("FW Validation Started....."));
        Serial.println(F("*************************************************************" ));
        Serial.println(F("*********  STM/ESP FIRMWARE VALIDATION REPORT  **************" ));
        Serial.println(F("*************************************************************" ));
        File ValidationFile = DOSFS.open("Validation.txt", "w");
        if (ValidationFile)
        {
            ValidationFile.println(F("*************************************************************" ));
            ValidationFile.println(F("*********  STM/ESP FIRMWARE VALIDATION REPORT  **************" ));
            ValidationFile.println(F("*************************************************************" ));
            ValidationFile.println(F(""));
            ValidationFile.println(F("Validation[DEV]-File Open: OK"));
        }
#if 0
        /***** Firmware version Check *****/
        const char FIRMWARE_VERSION_STM_OTA[8] = "1.1.3";
        Serial.print(F("STM FIRMWARE_VERSION:"));
        Serial.println(FIRMWARE_VERSION);
        ValidationFile.print(F("STM FIRMWARE_VERSION:"));
        ValidationFile.println(FIRMWARE_VERSION);
        if(strcmp(FIRMWARE_VERSION, FIRMWARE_VERSION_STM_OTA))
        {
            Serial.println(F("Validation[DEV]-STM FW Version: Fail !"));
            ValidationFile.println(F("Validation[DEV]-STM FW Version: Fail !"));
        }

        /***** Firmware version Check *****/
        const char FIRMWARE_VERSION_ESP_OTA[8] = "1.0.6";
        Serial.print(F("ESP FIRMWARE_VERSION:"));
        Serial.println(iuWiFi.espFirmwareVersion);
        ValidationFile.print(F("ESP FIRMWARE_VERSION:"));
        ValidationFile.println(iuWiFi.espFirmwareVersion);
        if(strcmp(iuWiFi.espFirmwareVersion, FIRMWARE_VERSION_ESP_OTA))
        {
            Serial.println(F("Validation[DEV]-ESP FW Version: Fail !"));
            ValidationFile.println(F("Validation[DEV]-ESP FW Version: Fail !"));
        }

    //   Serial.println(F("Validation[DEV]-STM FW Version:OK !"));
    //   ValidationFile.println(F("Validation[DEV]-STM FW Version: OK"));
        /***** Device Type Check *****/
        Serial.print(F("DEVICE_TYPE:"));
        Serial.println(DEVICE_TYPE);
        ValidationFile.print(F("DEVICE_TYPE:"));
        ValidationFile.println(DEVICE_TYPE);
        if(strcmp(DEVICE_TYPE, "ide_plus"))
        {
            Serial.println(F("Validation[DEV]-Device Type: Fail !"));
            ValidationFile.println(F("Validation[DEV]-Device Type: Fail !"));
        }
    //   Serial.println(F("Validation[DEV]-Device Type: OK"));
    //   ValidationFile.println(F("Validation[DEV]-Device Type: OK"));
#endif
        Serial.println(F("STM MCU INFORMATION:"));
        ValidationFile.println(F("STM MCU INFORMATION:"));
        char Cnt = 0;
        for(int i = 0 ; i < 10; i++)
        {
            float vdda = STM32.getVREF();
            float temperature = STM32.getTemperature();
            Serial.println("Voltage: " + String(vdda) + " Temp." + String(temperature));
            ValidationFile.print(F("Voltage:"));
            ValidationFile.print(vdda);
            ValidationFile.print(F(" Volts, Temperature:"));
            ValidationFile.print(temperature);
            ValidationFile.println(F(" C"));
            if (vdda < 3.0 || temperature > 40)
            {
                Cnt++;            
            }
            if(Cnt > 7)
            {
                Serial.println(F("Validation [DEV]-MCU INFO: Fail !"));
                ValidationFile.println(F("Validation [DEV]-MCU INFO: Fail !"));
            }
            delay(500);
        }
        Serial.print(F("DEVICE FREE MEMORY(RAM): "));
        Serial.println(freeMemory(),DEC);
        ValidationFile.print(F("DEVICE FREE MEMORY(RAM): "));
        ValidationFile.println(freeMemory(),DEC);  
        if(freeMemory() < 30000)
        {
            Serial.println(F("Validation [DEV]-Free Memory: Fail !"));
            ValidationFile.println(F("Validation [DEV]-Free Memory: Fail !"));      
        }
        Serial.print(F("DEVICE BLE  MAC ADDRESS:"));
        Serial.println(m_macAddress);
        ValidationFile.print(F("DEVICE BLE  MAC ADDRESS:"));
        ValidationFile.println(m_macAddress);
        MacAddress Mac(00,00,00,00,00,00);
        if(m_macAddress == Mac || !iuBluetooth.isBLEAvailable)
        {
            Serial.println("Validation [DEV]-BLE MAC: Fail !");
            ValidationFile.println(F("Validation [DEV]-BLE MAC: Fail !"));
        }
        Serial.print(F("DEVICE WIFI MAC ADDRESS:"));
        Serial.println(iuWiFi.getMacAddress());
        ValidationFile.print(F("DEVICE WIFI MAC ADDRESS:"));
        ValidationFile.println(iuWiFi.getMacAddress());
        if(iuWiFi.getMacAddress() == Mac)
        {
            Serial.println("Validation [DEV]-WiFi MAC: Fail !");
            ValidationFile.println(F("Validation [DEV]-WiFi MAC: Fail !"));
        }
        Serial.println(F("DEVICE SENSOR INTERFACE CHECK:-"));
        ValidationFile.println(F("DEVICE SENSOR INTERFACE CHECK:-"));
        Get_Device_Details(&ValidationFile);

        Serial.println(F("DEVICE MQTT CONFIG CHECK:-"));
        ValidationFile.println(F("DEVICE MQTT CONFIG CHECK:-"));
        STM_MQTT_Validation(&ValidationFile);
        Serial.println(F("DEVICE HTTP CONFIG CHECK:-"));
        ValidationFile.println(F("DEVICE HTTP CONFIG CHECK:-"));
        STM_HTTP_Validation(&ValidationFile);
        Serial.println(F("DEVICE FFT CONFIG CHECK:-"));
        ValidationFile.println(F("DEVICE FFT CONFIG CHECK:-"));
        STM_FFT_Validation(&ValidationFile);

        Serial.println(F("DEVICE WIFI STATUS:"));
        ValidationFile.println(F("DEVICE WIFI STATUS:"));
        WiFiCon = iuWiFi.isConnected();
        if(WiFiCon)
        {
            Serial.println(F("DEVICE WIFI STATUS: CONNECTED"));
            ValidationFile.println(F("DEVICE WIFI STATUS: CONNECTED"));
            iuWiFi.turnOff();
            FW_Valid_State = 1;
        }
        else
        {
            Serial.print(F("DEVICE WIFI STATUS: NOT CONNECTED !"));
            ValidationFile.print(F("DEVICE WIFI STATUS: NOT CONNECTED !"));
            FW_Valid_State = 0;
            ValidationFile.close();
            return 0;
        }
        ValidationFile.close();
        return 1; 
    }
    else if(FW_Valid_State == 1)
    {
        File ValidationFile = DOSFS.open("Validation.txt", "w");
        Serial.println(F("DEVICE WIFI DISCONNECT TEST:"));
        ValidationFile.println(F("DEVICE WIFI DISCONNECT TEST:"));
        WiFiCon = iuWiFi.isConnected();    
        if(WiFiCon == 1)
        {
            Serial.println(F("DEVICE WIFI DISCONNECT-FAILED"));
            ValidationFile.println(F("DEVICE WIFI DISCONNECT-FAILED"));
            FW_Valid_State = 0;
            ValidationFile.close();
            return 0;
        }
        else
        {
            Serial.println(F("DEVICE WIFI DISCONNECT-PASSED"));
            ValidationFile.println(F("DEVICE WIFI DISCONNECT-PASSED"));
            iuWiFi.turnOff();
            delay(100);
            iuWiFi.turnOn(1);   
            FW_Valid_State = 2;
            ValidationFile.close();
            return 1;
        }
    }
    else if(FW_Valid_State == 2)
    {
        File ValidationFile = DOSFS.open("Validation.txt", "w");
        Serial.println(F("DEVICE WIFI CONNECT TEST:"));
        ValidationFile.println(F("DEVICE WIFI CONNECT TEST:"));
        WiFiCon = iuWiFi.isConnected();    
        if(WiFiCon == 0)
        {
            Serial.println(F("DEVICE WIFI CONNECT-FAILED"));
            ValidationFile.println(F("DEVICE WIFI CONNECT-FAILED"));
        }
        else
        {
            Serial.println(F("DEVICE WIFI CONNECT-PASSED"));
            ValidationFile.println(F("DEVICE WIFI CONNECT-PASSED"));
        }
        FW_Valid_State = 3;
        ValidationFile.close();
//        iuWiFi.hardReset(); // Do Hardreset before concluding test, Write result, restart STM
        return 0;
    } 
}

bool Conductor::STM_MQTT_Validation(File *ValidationFile)
{
    // 1. Check default parameter setting
    Serial.print(F(" - MQTT DEFAULT SERVER IP:"));
    Serial.println(MQTT_DEFAULT_SERVER_IP);
    ValidationFile->print(F(" - MQTT DEFAULT SERVER IP:"));
    ValidationFile->println(MQTT_DEFAULT_SERVER_IP);
    if(MQTT_DEFAULT_SERVER_IP != IPAddress(13,233,38,155))
    {
        Serial.println("   Validation [MQTT]-Default IP Add: Fail !");
        ValidationFile->println(F("   Validation [MQTT]-Default IP Add: Fail !"));
    }
    Serial.print(F(" - MQTT DEFAULT SERVER PORT:"));
    Serial.println(MQTT_DEFAULT_SERVER_PORT);
    ValidationFile->print(F(" - MQTT DEFAULT SERVER PORT:"));
    ValidationFile->println(MQTT_DEFAULT_SERVER_PORT);
    if(MQTT_DEFAULT_SERVER_PORT != 1883)
    {
        Serial.println("   Validation [MQTT]-Default Port: Fail !");
        ValidationFile->println(F("   Validation [MQTT]-Default Port: Fail !"));
    }

    Serial.print(F(" - MQTT DEFAULT USERNAME:"));
    Serial.println(MQTT_DEFAULT_USERNAME);
    ValidationFile->print(F(" - MQTT DEFAULT USERNAME:"));
    ValidationFile->println(MQTT_DEFAULT_USERNAME);
    if(strcmp(MQTT_DEFAULT_USERNAME,"ispl"))
    {
        Serial.println("   Validation [MQTT]-Default Username: Fail !");
        ValidationFile->println(F("   Validation [MQTT]-Default Username: Fail !"));
    }

    Serial.print(F(" - MQTT DEFAULT PASSOWRD:"));
    Serial.println(MQTT_DEFAULT_ASSWORD);
    ValidationFile->print(F(" - MQTT DEFAULT PASSOWRD:"));
    ValidationFile->println(MQTT_DEFAULT_ASSWORD);
    if(strcmp(MQTT_DEFAULT_ASSWORD,"indicus"))
    {
        Serial.println("   Validation [MQTT]-Default Password: Fail !");
        ValidationFile->println(F("   Validation [MQTT]-Default Password: Fail !"));
    }

    // 2. Check MQTT update from config file stored in ext. flash
    conductor.configureMQTTServer("MQTT.conf");
    // 3. Check default parameter setting changed to read from config file ?
    if(m_mqttServerIp == IPAddress(13,233,38,155) && m_mqttServerPort == 1883 &&
      (strcmp(m_mqttUserName,"ispl") == 0) && (strcmp(m_mqttPassword,"indicus") == 0))
    {
        Serial.println("   Validation [MQTT]-Read Config File: Fail !");
        ValidationFile->println(F("   Validation [MQTT]-Read Config File: Fail !"));
    }
    Serial.println(F(" - MQTT FLASH CONFIG FILE READ: OK"));
    ValidationFile->println(F(" - MQTT FLASH CONFIG FILE READ: OK"));
}

bool Conductor::STM_HTTP_Validation(File *ValidationFile)
{
    //http configuration
    if(configureBoardFromFlash("httpConfig.conf",1) == false)
    {
        Serial.println("   Validation [HTTP]-Read Config File: Fail !");
        ValidationFile->println(F("   Validation [HTTP]-Read Config File: Fail !"));
        Serial.print(F(" - HTTP DEFAULT HOST IP:"));
        Serial.println(m_httpHost);
        ValidationFile->print(F(" - HTTP DEFAULT HOST IP:"));
        ValidationFile->println(m_httpHost);
        if(strcmp(m_httpHost,"13.232.122.10"))
        {
            Serial.println("   Validation [HTTP]-Default HOST IP: Fail !");
            ValidationFile->println(F("   Validation [HTTP]-Default HOST IP: Fail !")); 
        }
        Serial.print(F(" - HTTP DEFAULT HOST PORT:"));
        Serial.println(m_httpPort);
        ValidationFile->print(F(" - HTTP DEFAULT HOST PORT:"));
        ValidationFile->println(m_httpPort);
        if(m_httpPort != 8080)
        {
            Serial.println("   Validation [HTTP]-Default HOST PORT: Fail !");
            ValidationFile->println(F("   Validation [HTTP]-Default HOST PORT: Fail !"));        
        }
        Serial.print(F(" - HTTP DEFAULT HOST END POINT:"));
        Serial.println(m_httpPath);
        ValidationFile->print(F(" - HTTP DEFAULT HOST END POINT:"));
        ValidationFile->println(m_httpPath);
        if(strcmp(m_httpPath,"/iu-web/rawaccelerationdata"))
        {
            Serial.println("   Validation [HTTP]-Default HOST END Point: Fail !");
            ValidationFile->println(F("   Validation [HTTP]-Default HOST END Point: Fail !"));
        }
    }
    else
    {
        Serial.println(F(" - HTTP FLASH CONFIG FILE READ: Ok"));
        ValidationFile->println(F(" - HTTP FLASH CONFIG FILE READ: OK"));
    }    
}

bool Conductor::STM_FFT_Validation(File *ValidationFile)
{
    Serial.print(F(" - FFT DEFAULT SAMPLING RATE:"));
    Serial.println(FFTConfiguration::DEFAULT_SAMPLING_RATE); 
    ValidationFile->print(F(" - FFT DEFAULT SAMPLING RATE:"));
    ValidationFile->println(FFTConfiguration::DEFAULT_SAMPLING_RATE);
    if(FFTConfiguration::DEFAULT_SAMPLING_RATE != 3330)
    {
        Serial.println("   Validation [FFT]-Default Sampling Rate: Fail !");
        ValidationFile->println(F("   Validation [FFT]-Default Sampling Rate: Fail !"));
    }
    Serial.print(F(" - FFT DEFAULT BLOCK SIZE:"));
    Serial.println(FFTConfiguration::DEFAULT_BLOCK_SIZE);
    ValidationFile->print(F(" - FFT DEFAULT BLOCK SIZE:"));
    ValidationFile->println(FFTConfiguration::DEFAULT_BLOCK_SIZE);
    if(FFTConfiguration::DEFAULT_BLOCK_SIZE != 512)
    {
        Serial.println("   Validation [FFT]-Default Block Size: Fail !");
        ValidationFile->println(F("   Validation [FFT]-Default Block Size: Fail !"));
    }

    // Update the configuration of FFT computers from fft.conf
    if(setFFTParams() == false)
    {
        Serial.println("   Validation [FFT]-Read Config File: Fail !");
        ValidationFile->print(F("   Validation [FFT]-Read Config File: Fail !"));
    }
    Serial.println(F(" - FFT FLASH CONFIG FILE READ: OK"));
    ValidationFile->println(F(" - FFT FLASH CONFIG FILE READ: OK"));
}

bool Conductor::Get_Device_Details(File *ValidationFile)
{
    iuI2C.scanDevices();
    Serial.println(F("DEVICE LSM COMM CHECK:-"));
    ValidationFile->println(F("DEVICE LSM COMM CHECK:-"));
    if (!iuI2C.checkComponentWhoAmI("LSM6DSM ACC", iuAccelerometer.ADDRESS,iuAccelerometer.WHO_AM_I,iuAccelerometer.I_AM))
    {
        Serial.println("   Validation [LSM]-Read Add: Fail !");
        ValidationFile->println(F("   Validation [LSM]-Read Add: Fail !"));
    }
    else
    {        // iuI2C.releaseReadLock();
        if(iuI2C.i2c_dev[0] == iuAccelerometer.I_AM || iuI2C.i2c_dev[1] == iuAccelerometer.I_AM)
        {
            Serial.print("LSM6DSM I2C Add:0x");
            Serial.println(iuAccelerometer.I_AM,HEX);
            ValidationFile->print(F("LSM6DSM I2C Add:0x"));
            ValidationFile->println(iuAccelerometer.I_AM,HEX);
            Serial.println("   Validation [LSM]-Read Add: OK");
            ValidationFile->println(F("   Validation [LSM]-Read Add: Ok"));
        }
        else
        {
            Serial.println("   Validation [LSM]-Read Add: Fail !");
            ValidationFile->println(F("   Validation [LSM]-Read Add: Fail !"));
        }
    }

    if(iuI2C.i2c_dev[0] == iuTemp.I_AM || iuI2C.i2c_dev[1] == iuTemp.I_AM)
    {
        Serial.print("TMP116 I2C Add:0x");
        Serial.println(iuTemp.I_AM,HEX);
        ValidationFile->print(F("TMP116 I2C Add:0x"));
        ValidationFile->println(iuTemp.I_AM,HEX);
        Serial.println("   Validation [TMP]-Read Add: OK");
        ValidationFile->println(F("   Validation [TMP]-Read Add: Ok"));
    }
    else
    {
        Serial.println("   Validation [TMP]-Read Add: Fail !");
        ValidationFile->println(F("   Validation [TMP]-Read Add: Fail !"));
    }

    SPIClass *m_SPI;
    m_SPI->begin();
    /* Device communication check with Keonics Sensor usign SPI */
    /* Device communication check with External SRAM using QSPI ?? */
}
#if 0
bool Conductor::Get_Device_FeatureData(File *ValidationFile)
{
    char rawData[50];
    float aucostic =0;
    float* acceleration = NULL;

    Serial.println(F("DEVICE READ FEATURE RAW DATA:-"));
    ValidationFile->println(F("DEVICE READ FEATURE RAW DATA:-"));
    acceleration = iuAccelerometer.getData(iuUSB.port);
    Serial.print(F("AX : "));
    Serial.print(acceleration[0]);
    Serial.print(F(" g, AY : "));
    Serial.print(acceleration[1]);
    Serial.print(F(" g, AZ : "));
    Serial.print(acceleration[2]);
    Serial.println(F(" g"));
    Serial.print("Temperature : ");
    Serial.print(iuAccelerometer.m_LSMtemperature);
    Serial.println(" C");

    ValidationFile->print(F("AX : "));
    ValidationFile->print(acceleration[0]);
    ValidationFile->print(F(" g, AY : "));
    ValidationFile->print(acceleration[1]);
    ValidationFile->print(F(" g, AZ : "));
    ValidationFile->print(acceleration[2]);
    ValidationFile->println(F(" g"));
    ValidationFile->print("Temperature : ");
    ValidationFile->print(iuAccelerometer.m_LSMtemperature);
    ValidationFile->println(" C");

    aucostic = iuI2S.getData();
    snprintf(rawData,50,"%04.3f,%04.3f,%04.3f,%.3f",acceleration[0],acceleration[1],acceleration[2],aucostic);
    Serial.println(rawData);
    ValidationFile->print("Feature raw data: ");
    ValidationFile->println(rawData);

    Serial.print(F("DEVICE READ Acoustic Data: "));
    Serial.print(audioDB4096Computer.dBresult);
    Serial.println(F(" dB"));
    ValidationFile->print(F("DEVICE READ Acoustic Data: "));
    ValidationFile->print(audioDB4096Computer.dBresult);
    ValidationFile->println(F(" dB"));

    if ( (audioDB4096Computer.dBresult < 58.0) && (audioDB4096Computer.dBresult > 160.0) ){
        Serial.println("   Validation [LSM]-Read Acoustic: Fail !");
        ValidationFile->println(F("   Validation [FFT]-Read Acoustic: Fail !"));
    }

    double fingerprint_timestamp = getDatetime();
    int messageLength = strlen(fingerprintData); 
    char FingerPrintResult[150 + messageLength];
    DynamicJsonBuffer jBuffer;
    JsonObject& object =jBuffer.parseObject(fingerprintData);
    Serial.println("********* FINGERDATA **********");
    float  f1 = object["100"];
    Serial.println(f1);
     f1 = object["101"];
    Serial.println(f1);
     f1 = object["102"];
    Serial.println(f1);        
     f1 = object["103"];
    Serial.println(f1);
     f1 = object["104"];
    Serial.println(f1);
     f1 = object["105"];
    Serial.println(f1);
    Serial.println("*******************************");
    Serial.println(F("DEVICE READ FINGERPRINT DATA:-"));
    ValidationFile->println(F("DEVICE READ FINGERPRINT DATA:-"));

    snprintf(FingerPrintResult, 150 + messageLength, "{\"macID\":\"%s\",\"timestamp\": %lf,\"state\":\"%d\",\"accountId\":\"%s\",\"fingerprints\": %s }", m_macAddress.toString().c_str(),fingerprint_timestamp,ledManager.getOperationState(),"XXXAdmin",fingerprintData);
    Serial.println(FingerPrintResult);
    ValidationFile->println(FingerPrintResult);

 //   Serial.println("FW Validation Sending Raw Data:");
 //   CheckRawData(ValidationFile);
}

bool Conductor::STM_FINGERPRINT_Validation(File *ValidationFile)
{
    const char* fingerprintsKey[13];
    int i = 0; 
  //  JsonObject& config = configureJsonFromFlash("fingerprints.conf",1);
    JsonObject& config = iuDiagnosticEngine.configureFingerPrintsFromFlash("finterprints.conf",true);
    JsonObject& root2 = config["fingerprints"];
    String messageId = config["messageId"];
    Serial.print("messageId:");
    Serial.println(messageId);
    for (auto fingerprintsKeyValues : root2) {              // main for loop        
        
        JsonObject &root3 = root2[fingerprintsKeyValues.key];       // get the nested Keys
    
        fingerprintsKey[i] = fingerprintsKeyValues.key;
         
   //  if(m_direction == 0 && root3["dir"] =="VX" || root3["dir"] == "AX"){                             // seperate all the directions 
          // VX
      //   float  speedX = root3["speed"]; 
      //   float multiplierX = root3["mult"];
      //   float bandX = root3["band"];
      //   float frequencyX = root3["freq"];
        
        Serial.print("FingerPrint Key:");
        Serial.println(fingerprintsKeyValues.key); 
        i++;
    }

}

bool Conductor::CheckRawData(File *ValidationFile)
{
    if (m_streamingMode == StreamingMode::WIFI || m_streamingMode == StreamingMode::WIFI_AND_BLE) {
        rawDataRequest();
    }
    resetDataAcquisition();
//  delay(100);
//   manageRawDataSending();
}

#endif
