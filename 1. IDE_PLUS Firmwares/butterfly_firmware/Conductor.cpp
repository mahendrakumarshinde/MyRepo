#include<string.h>
#include "Conductor.h"
#include "rBase64.h"

const char* fingerprintData;
const char* fingerprints_X;
const char* fingerprints_Y;
const char* fingerprints_Z;

int sensorSamplingRate;
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
    
    if (!root.success()) {
        if (debugMode) {
            debugPrint("parseObject() failed");
        }
        return false;
    }
    // Device level configuration
    JsonVariant subConfig = root["main"];
    subConfig.printTo(jsonChar);
    
    if (subConfig.success()) {
        configureMainOptions(subConfig);
        if (saveToFlash) {
            iuFlash.saveConfigJson(IUFlash::CFG_DEVICE, subConfig);
        }
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
    m_httpHost = "http://13.232.122.10";
    m_httpPort = 8080;
    m_httpPath = "/iu-web/iu-infiniteuptime-api/postdatadump?mac=";
   
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
                sendAccelRawData(0);  // Axis X
                sendAccelRawData(1);  // Axis Y
                sendAccelRawData(2);  // Axis Z
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

void processJSONmessage(const char * buff) 
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
        // MSP Status messages
        case MSPCommand::MSP_INVALID_CHECKSUM:
            if (loopDebugMode) { debugPrint(F("MSP_INVALID_CHECKSUM")); }
            break;
        case MSPCommand::MSP_TOO_LONG:
            if (loopDebugMode) { debugPrint(F("MSP_TOO_LONG")); }
            break;

        case MSPCommand::ASK_BLE_MAC:
            if (loopDebugMode) { debugPrint(F("ASK_BLE_MAC")); }
            iuWiFi.sendBleMacAddress(m_macAddress);
            break;
	      case MSPCommand::ASK_HOST_FIRMWARE_VERSION:
            if(loopDebugMode){ debugPrint(F("ASK_HOST_FIRMWARE_VERSION")); }
            iuWiFi.sendHostFirmwareVersion(FIRMWARE_VERSION);               
            break;
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
            JsonObject& config = configureJsonFromFlash("httpConfig.conf",1);

            m_httpHost = config["httpConfig"]["host"];
            m_httpPort = config["httpConfig"]["port"];
            m_httpPath = config["httpConfig"]["path"];
            //Serial.print("File Content :");Serial.println(jsonChar);
            //Serial.print("http details :");Serial.print(m_httpHost);Serial.print(",");Serial.print(m_httpPort);Serial.print(",");Serial.print(m_httpPath);Serial.println("/****** SWITCH****/");
            if(m_httpHost == NULL && m_httpPort == 0 && m_httpPath == NULL ){
              //load default configurations
              m_httpHost = "http://13.232.122.10";                                       //"ideplus-dot-infinite-uptime-1232.appspot.com";
              m_httpPort =  8080;                                                        //80;
              m_httpPath = "/iu-web/iu-infiniteuptime-api/postdatadump?mac=";           //"/raw_data?mac="; 
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
            timerISRPeriod = 600;  // 1.6KHz
            sensorSamplingRate = 1660;
            //Serial.println("STEP - 2");
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
            timerISRPeriod = 300;
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
            
            iuWiFi.startLiveMSPCommand(MSPCommand::PUBLISH_FEATURE_WITH_CONFIRMATION, msgLen + 2);
            iuWiFi.streamLiveMSPMessage((char) nodeToSend->idx);
            iuWiFi.streamLiveMSPMessage(':');
            iuWiFi.streamLiveMSPMessage(nodeToSend->buffer, msgLen);
            iuWiFi.endLiveMSPCommand();
            sendingQueue.attemptingToSend(nodeToSend->idx);
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
    static char rawAccelerationX[15000];
    static char rawAccelerationY[15000];
    static char rawAccelerationZ[15000];
    
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
       
        uint16_t maxLen = 15000;   //3500
        char txBuffer[maxLen];
        for (uint16_t i =0; i < maxLen; i++) {
            txBuffer[i] = 0;
        }
        txBuffer[0] = axis[axisIdx];
        uint16_t idx = 1;
        idx += accelEnergy->sendToBuffer(txBuffer, idx, 4);   //4
        txBuffer[idx] = 0; // Terminate string (idx incremented in sendToBuffer)
        //iuWiFi.sendMSPCommand(MSPCommand::PUBLISH_RAW_DATA, txBuffer);
        iuWiFi.sendLongMSPCommand(MSPCommand::SEND_RAW_DATA, 1000000,
                                  txBuffer, strlen(txBuffer));

        delay(10);
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
            
            memmove(rawAccelerationX,txBuffer + 2, strlen(txBuffer) - 1); // sizeof(txBuffer)/sizeof(txBuffer[0]) -2);
            debugPrint("RawAcel X: ",false);debugPrint(rawAccelerationX);
        }else if(txBuffer[0]== 'Y' && axisIdx == 1)
        {
            memmove(rawAccelerationY,txBuffer + 2, strlen(txBuffer) - 1 ); //sizeof(txBuffer)/sizeof(txBuffer[0]) -2);
            debugPrint("RawAcel Y: ",false);debugPrint(rawAccelerationY);
        }else if(txBuffer[0] == 'Z' && axisIdx == 2){
            
            memmove(rawAccelerationZ,txBuffer + 2, strlen(txBuffer) - 1) ; //sizeof(txBuffer)/sizeof(txBuffer[0]) -2);    
            debugPrint("RawAcel Z: ",false);debugPrint(rawAccelerationZ);
            snprintf(rawAcceleration,maxLen,"{\"deviceId\":\"%s\",\"transport\":%d,\"messageType\":%d,\"payload\":\"{\\\"deviceId\\\":\\\"%s\\\",\\\"firmwareVersion\\\":\\\"%s\\\",\\\"samplingRate\\\":%d,\\\"blockSize\\\":%d,\\\"X\\\":\\\"%s\\\",\\\"Y\\\":\\\"%s\\\",\\\"Z\\\":\\\"%s\\\"}\"}",m_macAddress.toString().c_str(),1,0,m_macAddress.toString().c_str(),FIRMWARE_VERSION,IULSM6DSM::defaultSamplingRate,512,rawAccelerationX,rawAccelerationY,rawAccelerationZ);
            
            
            iuWiFi.write(rawAcceleration);           // send the rawAcceleration over UART 
            iuWiFi.write("\n");            
            if(loopDebugMode){
               debugPrint("Raw Acceleration:",true);
               debugPrint(rawAcceleration);     
            }
            //FREE ALLOCATED MEMORY 
            memset(rawAccelerationX,0,sizeof(rawAccelerationX));
            memset(rawAccelerationY,0,sizeof(rawAccelerationY));
            memset(rawAccelerationZ,0,sizeof(rawAccelerationZ));
        }

     }
}

/**
 * Handle periodical publication of accel raw data.
 */
void Conductor::periodicSendAccelRawData()
{
    uint32_t now = millis();
    if (now - m_rawDataPublicationStart > m_rawDataPublicationTimer) {
        delay(500);
        sendAccelRawData(0);
        sendAccelRawData(1);
        sendAccelRawData(2);
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
                    iuWiFi.sendMSPCommand(MSPCommand::SEND_DIAGNOSTIC_RESULTS,FingerPrintResult );    
                
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