#include "Conductor.h"
#include "Utilities.h"

#define UART_TX_FIFO_SIZE 0x80
#define OTA_PACKET_SIZE 1024
/* =============================================================================
    Instanciation
============================================================================= */

char hostSerialBuffer[8500];

IUSerial hostSerial(&Serial, hostSerialBuffer, 8500, IUSerial::MS_PROTOCOL,
                    115200, ';', 100);

IURawDataHelper accelRawDataHelper(10000,  // 10s timeout to input all keys
                                   300000,  // 5min timeout to succefully post data
                                   DATA_DEFAULT_ENDPOINT_HOST,
                                   RAW_DATA_DEFAULT_ENDPOINT_ROUTE,
                                   DATA_DEFAULT_ENDPOINT_PORT);

IUMQTTHelper mqttHelper = IUMQTTHelper();

IUTimeHelper timeHelper(2390, "time.google.com");


/* =============================================================================
    Conductor
============================================================================= */

Conductor::Conductor() :
    m_useMQTT(true),
    m_lastMQTTInfoRequest(0),
    m_lastConnectionAttempt(0),
    m_disconnectionTimerStart(0),
    m_lastWifiStatusUpdate(0),
    m_lastWifiStatusCheck(0),
    m_lastWifiInfoPublication(0),
    m_mqttServerIP(IPAddress())
{
    m_featurePostPort = DATA_DEFAULT_ENDPOINT_PORT;
    m_diagnosticPostPort = DATA_DEFAULT_ENDPOINT_PORT;
    m_credentialValidator.setTimeout(wifiConfigReceptionTimeout);
    m_staticConfigValidator.setTimeout(wifiConfigReceptionTimeout);
    m_mqttServerValidator.setTimeout(wifiConfigReceptionTimeout);
    m_mqttCredentialsValidator.setTimeout(wifiConfigReceptionTimeout);
    m_wifiMAC.fromString(WiFi.macAddress().c_str());
    // Default values for endpoints
    strncpy(m_featurePostHost, DATA_DEFAULT_ENDPOINT_HOST, MAX_HOST_LENGTH);
    strncpy(m_featurePostRoute, FEATURE_DEFAULT_ENDPOINT_ROUTE,
            MAX_ROUTE_LENGTH);
    strncpy(m_diagnosticPostHost, DATA_DEFAULT_ENDPOINT_HOST, MAX_HOST_LENGTH);
    strncpy(m_diagnosticPostRoute, DIAGNOSTIC_DEFAULT_ENDPOINT_ROUTE,
            MAX_ROUTE_LENGTH);
}

void Conductor::setBleMAC(MacAddress hostBLEMac)
{
    m_bleMAC = hostBLEMac;
    mqttHelper.setDeviceMAC(m_bleMAC);
}

void Conductor::setBleMAC(const char *hostBLEMac)
{
    m_bleMAC.fromString(hostBLEMac);
    mqttHelper.setDeviceMAC(m_bleMAC);
}

/**
 *
 *
 * @param duration_ms The sleep duration in microseconds. If duration_ms=0, will
 *  sleep for default Conductor::deepSleepDuration.
 */
void Conductor::deepsleep(uint32_t duration_ms)
{
    if (debugMode)
    {
        debugPrint("Going to deep-sleep");
    }
    hostSerial.sendMSPCommand(MSPCommand::WIFI_ALERT_DISCONNECTED);
    hostSerial.sendMSPCommand(MSPCommand::WIFI_ALERT_SLEEPING);
    delay(100); // Wait for serial messages to be sent
    ESP.deepSleep(duration_ms * 1000);
}


/* =============================================================================
    Communication with host
============================================================================= */

/**
 *
 */
void Conductor::processHostMessage(IUSerial *iuSerial)
{
    MSPCommand::command cmd = iuSerial->getMspCommand();
    char *buffer = iuSerial->getBuffer();
    uint16_t bufferLength = iuSerial->getCurrentBufferLength();
    char resp[2] = "";
    resp[1] = 0;
    char message[256];
    switch(cmd) {
        case MSPCommand::OTA_INIT_ACK:
            if(otaInProgress == false) {
                mqttHelper.publish(OTA_TOPIC,buffer);
            }
            break;
        case MSPCommand::OTA_FDW_START:
            if(otaInProgress == false) {    
                mqttHelper.publish(OTA_TOPIC,buffer);
                strncpy(ota_uri, otaStm_uri, 512);
                delay(100);
                otaDnldFw(false); //false => Start of FW Dnld
            }
            break;
        case MSPCommand::OTA_FDW_ABORT:
            mqttHelper.publish(OTA_TOPIC,buffer);
            http_ota.end();
            otaInProgress = false;
            waitingForPktAck = false;
            break;
        case MSPCommand::OTA_FUG_START:
            mqttHelper.publish(OTA_TOPIC,buffer);
            break;
        case MSPCommand::OTA_FUG_SUCCESS:
            mqttHelper.publish(OTA_TOPIC,buffer);
            otaInProgress = false;
            waitingForPktAck = false;
            delay(100);
            break;
        case MSPCommand::OTA_FUG_ABORT:
            mqttHelper.publish(OTA_TOPIC,buffer);
            otaInProgress = false;
            waitingForPktAck = false;
            delay(100);
            break;
        case MSPCommand::OTA_PACKET_ACK:
            waitingForPktAck = false;
            otaDnldFw(true);   //true => Cont. of FW Dnld
            break;
        case MSPCommand::OTA_STM_DNLD_OK:
            strncpy(ota_uri, otaEsp_uri, 512);
            otaDnldFw(false); //false => Start of FW Dnld
            break;
        case MSPCommand::OTA_ESP_DNLD_OK:
            break;
        case MSPCommand::OTA_FDW_SUCCESS:
            delay(10);
            mqttHelper.publish(OTA_TOPIC,buffer);
//            otaInProgress = false; // Temp. Only after Download+Upgrade+Validation this shall be set to false
            waitingForPktAck = false;
            break;
        case MSPCommand::SET_OTA_STM_URI:
            strcpy(otaStm_uri,buffer);
            delay(1);
            break;
        case MSPCommand::SET_OTA_ESP_URI:
            strcpy(otaEsp_uri,buffer);
            delay(1);
            break;
        case MSPCommand::ASK_WIFI_FV:
            iuSerial->sendMSPCommand(MSPCommand::RECEIVE_WIFI_FV, FIRMWARE_VERSION);
            break;
        case MSPCommand::CONFIG_ACK:
            // Send the fft configuration update acknowledgement to command response topic
            mqttHelper.publish(COMMAND_RESPONSE_TOPIC,buffer);
            break;

        /***** MAC addresses *****/
        case MSPCommand::RECEIVE_BLE_MAC:
            setBleMAC(iuSerial->mspReadMacAddress());
            break;
        case MSPCommand::ASK_WIFI_MAC:
            iuSerial->mspSendMacAddress(MSPCommand::RECEIVE_WIFI_MAC,
                                        m_wifiMAC);
            break;
        case MSPCommand::RECEIVE_HOST_FIRMWARE_VERSION: 
            getDeviceFirmwareVersion(message,buffer,FIRMWARE_VERSION);
            mqttHelper.publishDiagnostic(message);
            strncpy(HOST_FIRMWARE_VERSION, buffer, 8);
            break;
        case MSPCommand::RECEIVE_HOST_SAMPLING_RATE:
            HOST_SAMPLING_RATE = atoi(buffer);
            break;
        case MSPCommand::RECEIVE_HOST_BLOCK_SIZE:
            HOST_BLOCK_SIZE = atoi(buffer);
            break;
        /***** Logging *****/
        case MSPCommand::SEND_LOG_MSG:
            mqttHelper.publishLog(buffer);
            break;

        /***** Wifi Config *****/
        case MSPCommand::WIFI_RECEIVE_SSID:
            receiveNewCredentials(buffer, NULL);
            break;
        case MSPCommand::WIFI_RECEIVE_PASSWORD:
            receiveNewCredentials(NULL, buffer);
            break;
        case MSPCommand::WIFI_FORGET_CREDENTIALS:
            forgetWiFiCredentials();
            disconnectWifi();
            break;
        case MSPCommand::WIFI_RECEIVE_STATIC_IP:
        case MSPCommand::WIFI_RECEIVE_GATEWAY:
        case MSPCommand::WIFI_RECEIVE_SUBNET:
            receiveNewStaticConfig(
                iuSerial->mspReadIPAddress(),
                (uint8_t) cmd - (uint8_t) MSPCommand::WIFI_RECEIVE_STATIC_IP);
            break;
        case MSPCommand::WIFI_FORGET_STATIC_CONFIG:
            forgetWiFiStaticConfig();
            break;

        /***** Wifi commands *****/
        case MSPCommand::WIFI_SOFT_RESET:
            ESP.restart(); // ESP32_PORT_TRUE
            break;
        case MSPCommand::WIFI_CONNECT:
            // Reset disconnection timer
            m_disconnectionTimerStart = millis();
            reconnect();
            break;
        case MSPCommand::WIFI_DISCONNECT:
            disconnectWifi();
            break;

        /***** Data publication *****/
        // Implemented in Host Firmware v1.1.3
        case MSPCommand::PUBLISH_DEVICE_DETAILS_MQTT:
            mqttHelper.publish(COMMAND_RESPONSE_TOPIC, buffer);
            break;
        case MSPCommand::PUBLISH_RAW_DATA:      //not used in Host Firmware v1.1.2
           
            if (accelRawDataHelper.inputHasTimedOut()) {
                accelRawDataHelper.resetPayload();
            }
            accelRawDataHelper.addKeyValuePair(buffer[0], &buffer[2],
                                               strlen(buffer) - 2);
            iuSerial->sendMSPCommand(MSPCommand::WIFI_CONFIRM_ACTION, buffer, 1);
            accelRawDataHelper.publishIfReady(m_bleMAC);
                  
            break;
        case MSPCommand::PUBLISH_FEATURE:
            if (publishFeature(&buffer[7], bufferLength - 7, buffer, 6)) {
                iuSerial->sendMSPCommand(MSPCommand::WIFI_CONFIRM_PUBLICATION);
            }
            break;
        case MSPCommand::PUBLISH_FEATURE_WITH_CONFIRMATION:
            // Buffer structure:
            // "[confirmation_id (1 char)]:[feature_group (6 chars)],[feature msg]"
            if (publishFeature(&buffer[9], bufferLength - 9, &buffer[2], 6)) {
                iuSerial->sendMSPCommand(MSPCommand::WIFI_CONFIRM_PUBLICATION,
                                         &buffer[0], 1);
            }
            break;
        case MSPCommand::PUBLISH_DIAGNOSTIC:
            publishDiagnostic(buffer, bufferLength);
            break;
        case MSPCommand::PUBLISH_CONFIG_CHECKSUM:
            mqttHelper.publish(CHECKSUM_TOPIC, buffer);
            break;

        /***** Cloud command reception and transmission *****/
        case MSPCommand::HOST_CONFIRM_RECEPTION:
            // TODO Implement
            break;
        case MSPCommand::HOST_FORWARD_CMD:
            // TODO Implement
            break;

        /***** Publication protocol selection *****/
        case MSPCommand::PUBLICATION_USE_MQTT:
            m_useMQTT = true;
            break;
        case MSPCommand::PUBLICATION_USE_HTTP:
            m_useMQTT = false;
            break;

        /***** Settable parameters (addresses, credentials, etc) *****/
        case MSPCommand::SET_RAW_DATA_ENDPOINT_HOST:
            accelRawDataHelper.setEndpointHost(buffer);
            //mqttHelper.publish(FINGERPRINT_DATA_PUBLISH_TOPIC, "RAW HOST...");
            break;
        case MSPCommand::SET_RAW_DATA_ENDPOINT_ROUTE:
            accelRawDataHelper.setEndpointRoute(buffer);
            //mqttHelper.publish(FINGERPRINT_DATA_PUBLISH_TOPIC, "RAW ROUTE...");
            break;
        case MSPCommand::SET_RAW_DATA_ENDPOINT_PORT:
            accelRawDataHelper.setEndpointPort(
                uint16_t(strtol(buffer, NULL, 0)));
                //mqttHelper.publish(FINGERPRINT_DATA_PUBLISH_TOPIC, "RAW PORT...");
            break;
        case MSPCommand::SET_MQTT_SERVER_IP:
            //hostSerial.write("FROM WIFI SET_MQTT_SERVER_IP :");
            //hostSerial.write(buffer);
            if (m_mqttServerValidator.hasTimedOut()) {
                m_mqttServerValidator.reset();
            }
            m_mqttServerIP = iuSerial->mspReadIPAddress();
            //hostSerial.write("RECEIVED IP :");hostSerial.write(m_mqttServerIP);
            
            m_mqttServerValidator.receivedMessage(0);
            if (m_mqttServerValidator.completed()) {
                mqttHelper.setServer(m_mqttServerIP, m_mqttServerPort);
                hostSerial.write("RECEIVED MQTT SERVER IP FROM DEVICE ");
           
            }
            break;
        case MSPCommand::SET_MQTT_SERVER_PORT:
           
            if (m_mqttServerValidator.hasTimedOut()) {
                m_mqttServerValidator.reset();
            }
            m_mqttServerPort = uint16_t(strtol(buffer, NULL, 0));
            m_mqttServerValidator.receivedMessage(1);
            if (m_mqttServerValidator.completed()) {
                mqttHelper.setServer(m_mqttServerIP, m_mqttServerPort);
            }
            break;
        case MSPCommand::SET_MQTT_USERNAME:
            if (m_mqttCredentialsValidator.hasTimedOut()) {
                m_mqttCredentialsValidator.reset();
            }
            strncpy(m_mqttUsername, buffer, IUMQTTHelper::credentialMaxLength);
            m_mqttCredentialsValidator.receivedMessage(0);
            if (m_mqttCredentialsValidator.completed()) {
                mqttHelper.setCredentials(m_mqttUsername, m_mqttPassword);
            }
            break;
        case MSPCommand::SET_MQTT_PASSWORD:
            if (m_mqttCredentialsValidator.hasTimedOut()) {
                m_mqttCredentialsValidator.reset();
            }
            strncpy(m_mqttPassword, buffer, IUMQTTHelper::credentialMaxLength);
            m_mqttCredentialsValidator.receivedMessage(1);
            if (m_mqttCredentialsValidator.completed()) {
                mqttHelper.setCredentials(m_mqttUsername, m_mqttPassword);
            }
            break;
        /********** Diagnostic Fingerprints Commands **************/    
        case MSPCommand::RECEIVE_DIAGNOSTIC_ACK:
          // Send the Ack to Topic
            mqttHelper.publish(COMMAND_RESPONSE_TOPIC,buffer);
                   
            break;
        case MSPCommand::SEND_DIAGNOSTIC_RESULTS:
              // publish the diagnostic fingerprints results
              mqttHelper.publish(FINGERPRINT_DATA_PUBLISH_TOPIC,buffer);
           break; 
        case MSPCommand::SEND_RAW_DATA:
          {
            IUMessageFormat::rawDataPacket* rawData = (IUMessageFormat::rawDataPacket*) buffer;
            char ack_config[150];

            if (accelRawDataHelper.inputHasTimedOut()) {
                accelRawDataHelper.resetPayload();
            }

            // Only X axis timestamp is recorded so that record times can be correlated on server
            if (rawData->axis == 'X') timestamp = rawData->timestamp;
          
            // This mechanism ensures the ESP sends out only blockSize data points in the POST payload
            // Tried implementing this using structures, but ran into problems. See commented code in 
            //  IUMessage.h for details.
            int rawValuesSize = HOST_BLOCK_SIZE * 2;                // since 1 q15_t occupies 2 bytes
            int httpBufferSize = macIdSize + hostFirmwareVersionSize + timestampSize 
                            + blockSizeSize + samplingRateSize + axisSize + rawValuesSize;      // metadata total size will be constant(43 bytes) + variable block size
            int httpBufferPointer = 0;                      // keeps track of how many bytes are filed in the HTTP buffer 

            memcpy(&httpBuffer[httpBufferPointer], m_bleMAC.toString().c_str(), macIdSize);
            httpBufferPointer += macIdSize;

            memcpy(&httpBuffer[httpBufferPointer], HOST_FIRMWARE_VERSION, hostFirmwareVersionSize);
            httpBufferPointer += hostFirmwareVersionSize;

            memcpy(&httpBuffer[httpBufferPointer], &timestamp, timestampSize);
            httpBufferPointer += timestampSize;

            memcpy(&httpBuffer[httpBufferPointer], &HOST_BLOCK_SIZE, blockSizeSize);            
            httpBufferPointer += blockSizeSize;

            memcpy(&httpBuffer[httpBufferPointer], &HOST_SAMPLING_RATE, samplingRateSize);
            httpBufferPointer += samplingRateSize;

            memcpy(&httpBuffer[httpBufferPointer], &rawData->axis, axisSize);
            httpBufferPointer += axisSize;
            
            memcpy(&httpBuffer[httpBufferPointer], rawData->txRawValues, rawValuesSize);
            httpBufferPointer += rawValuesSize;

            iuSerial->sendMSPCommand(MSPCommand::WIFI_CONFIRM_ACTION, &rawData->axis, 1);
            
            int httpStatusCode = httpPostBigRequest(accelRawDataHelper.m_endpointHost, accelRawDataHelper.m_endpointRoute,
                                            accelRawDataHelper.m_endpointPort, (uint8_t*) &httpBuffer, 
                                            httpBufferPointer, HttpContentType::octetStream);            

            // send HTTP status code back to the MCU
            char httpAckBuffer[1 + 4];      // axis + 3 digit HTTP status code + null terminator
            httpAckBuffer[0] = rawData->axis;
            itoa(httpStatusCode, &httpAckBuffer[1], 10);
            iuSerial->sendMSPCommand(MSPCommand::HTTP_ACK, httpAckBuffer);

            // send HTTP status code to MQTT
            snprintf(ack_config, 150, "{\"messageType\":\"raw-data-ack\",\"mac\":\"%s\",\"httpCode\":\"%d\",\"axis\":\"%c\",\"timestamp\":%.2f}",
            m_bleMAC.toString().c_str(),httpStatusCode, rawData->axis, timestamp);
            mqttHelper.publish(COMMAND_RESPONSE_TOPIC, ack_config);
           break;  
          }
        case MSPCommand::RECEIVE_HTTP_CONFIG_ACK:
          // Send the Ack to Topic
            mqttHelper.publish(COMMAND_RESPONSE_TOPIC,buffer);
                   
            break;
        case MSPCommand::SEND_FLASH_STATUS:
            mqttHelper.publish(DIAGNOSTIC_TOPIC,buffer);                 
            break;
       /* case MSPCommand::SEND_ACCOUNTID:
         {
             //accountID =  buffer;
           char* result = strstr(accountID,"XXXAdmin");
           
           if(result != NULL  ){    // true
              accountID = buffer;
          }else {
            
              accountID = "XXXAdmin";     // default accountID
           }
           iuSerial->sendMSPCommand(MSPCommand::RECEIVE_RAW_DATA_ACK,buffer);  
           mqttHelper.publish(COMMAND_RESPONSE_TOPIC, accountID);
          
           break;

         }
       */  
      case MSPCommand::GET_PENDING_HTTP_CONFIG:
          // Get all the pending configuration messages over http
          
          String response =  accelRawDataHelper.publishConfigMessage(m_bleMAC);
          
          // create the JSON objects 
        //  DynamicJsonBuffer jsonBuffer;
        //  JsonObject& pendingConfigObject = jsonBuffer.parseObject(response); 
          
       /*   String pendingJson;
          
           for (auto configKeyValues : pendingConfigObject) {
               pendingConfigObject["result"][configKeyValues.key];// = configKeyValues.value;
               
             }
           pendingConfigObject.printTo(pendingJson);
        */
         // size_t msgLen = strlen(response);
       //   char fingerprintAlarm[1500];
       //   char featuresThreshold[1500]; 
       //   char fingerprintFeatures[1500];   
          
        //  JsonVariant fingerprintAlarmConfig  = pendingConfigObject["result"]["fingerprintAlarm"];
        //  JsonVariant featuresThresholdConfig = pendingConfigObject["result"]["alarm"];
        //  JsonVariant fingerprintFeaturesConfig = pendingConfigObject["result"]["fingerprint"];
             
          //Serial.println(fingerprintFeaturesConfig.size());
             
       //   fingerprintAlarmConfig.prettyPrintTo(fingerprintAlarm);
       //   featuresThresholdConfig.prettyPrintTo(featuresThreshold);
       //   fingerprintFeaturesConfig.prettyPrintTo(fingerprintFeatures);
            
          
          
          iuSerial->sendLongMSPCommand(MSPCommand::SET_PENDING_HTTP_CONFIG,1000000,response.c_str(),strlen(response.c_str()));
          
          //mqttHelper.publish(COMMAND_RESPONSE_TOPIC, fingerprintAlarm);//response.c_str());
          //mqttHelper.publish(COMMAND_RESPONSE_TOPIC, featuresThreshold);
          //mqttHelper.publish(COMMAND_RESPONSE_TOPIC, fingerprintFeatures);
          //mqttHelper.publish(COMMAND_RESPONSE_TOPIC, response.c_str());
          
          
          break;
            
    }
}



/* =============================================================================
    WiFi credentials and config
============================================================================= */

void Conductor::forceWiFiConfig(const char *userSSID, const char *userPSK,
                                IPAddress staticIp, IPAddress gateway,
                                IPAddress subnetMask)
{
    // Credentials
    forgetWiFiCredentials();
    strncpy(m_userSSID, userSSID, wifiCredentialLength);
    strncpy(m_userPassword, userPSK, wifiCredentialLength);
    m_credentialValidator.receivedMessage(0);
    m_credentialValidator.receivedMessage(1);
    // Static IP
    forgetWiFiStaticConfig();
    m_staticIp = staticIp;
    m_gateway = gateway;
    m_subnetMask = subnetMask;
    m_staticConfigValidator.receivedMessage(0);
    m_staticConfigValidator.receivedMessage(1);
    m_staticConfigValidator.receivedMessage(2);
    m_disconnectionTimerStart = millis();  // Reset Disconnection timer
}

/**
 *
 */
void Conductor::forgetWiFiCredentials()
{
    m_credentialValidator.reset();
    for (uint8_t i = 0; i < wifiCredentialLength; ++i)
    {
        m_userSSID[i] = 0;
        m_userPassword[i] = 0;
    }
}

/**
 * Receive new WiFi credentials (SSID and / or PSK)
 */
void Conductor::receiveNewCredentials(char *newSSID, char *newPSK)
{
    if (m_credentialValidator.hasTimedOut())
    {
        forgetWiFiCredentials();
    }
    if (newSSID)
    {
        m_credentialValidator.reset();
        memset(m_userSSID,0x00,wifiCredentialLength);
        memset(m_userPassword,0x00,wifiCredentialLength);
        strncpy(m_userSSID, newSSID, wifiCredentialLength);
        m_credentialValidator.receivedMessage(0);
    }
    if (newPSK)
    {
        strncpy(m_userPassword, newPSK, wifiCredentialLength);
        m_credentialValidator.receivedMessage(1);
    }
    if (m_credentialValidator.completed())
    {
        String info = String(m_userSSID) + "_" + String(m_userPassword);
        hostSerial.sendMSPCommand(MSPCommand::WIFI_CONFIRM_NEW_CREDENTIALS,
                                  info.c_str());
        // Reset disconnection timer before reconnection attempt
        m_disconnectionTimerStart = millis();
        reconnect(true);
    }
}

/**
 *
 */
void Conductor::forgetWiFiStaticConfig()
{
    m_staticConfigValidator.reset();
    m_staticIp = IPAddress();
    m_gateway = IPAddress();
    m_subnetMask = IPAddress();
}

/**
 * Receive new WiFi static config (static IP, gateway or subnet mask).
 *
 * @param ip: The IP address of either the static IP, gateway or subnet mask.
 * @param idx: An index 0, 1 or 2 corresponding to the kind of IP (static IP,
    gateway or subnet mask)
 */
void Conductor::receiveNewStaticConfig(IPAddress ip, uint8_t idx)
{
    if (m_staticConfigValidator.hasTimedOut())
    {
        forgetWiFiStaticConfig();
    }
    if (idx == 0) { m_staticIp = hostSerial.mspReadIPAddress(); }
    else if (idx == 1) { m_gateway = hostSerial.mspReadIPAddress(); }
    else if (idx == 2) { m_subnetMask = hostSerial.mspReadIPAddress(); }
    else { return; }
    m_staticConfigValidator.receivedMessage(idx);
    if (m_staticConfigValidator.completed())
    {
        hostSerial.sendMSPCommand(MSPCommand::WIFI_CONFIRM_NEW_STATIC_CONFIG);
        // Reset disconnection timer before reconnection attempt
        m_disconnectionTimerStart = millis();
        reconnect(true);
    }
}

/**
 * Request host to know if should sleep & get BLE MAC address.
 *
 * If the host has not answered after hostResponseTimeout, the ESP82 will deep-
 * sleep for deepSleepDuration and then restart.
 */
bool Conductor::getConfigFromMainBoard()
{
    uint32_t startTime = millis();
    uint32_t current = startTime;
    while (uint64_t(m_bleMAC) == 0)
    {
        if (current - startTime > hostResponseTimeout)
        {
            if (debugMode)
            {
                debugPrint("Host didn't respond");
            }
            deepsleep();
        }
        delay(500);
        hostSerial.sendMSPCommand(MSPCommand::ASK_BLE_MAC);
        hostSerial.sendMSPCommand(MSPCommand::GET_MQTT_CONNECTION_INFO);
        hostSerial.sendMSPCommand(MSPCommand::GET_RAW_DATA_ENDPOINT_INFO); 
        
        // get the IDE Firmware Version from Host
        //hostSerial.sendMSPCommand(MSPCommand::ASK_HOST_FIRMWARE_VERSION);
        m_lastMQTTInfoRequest = current;
        delay(100);
        hostSerial.readMessages();
        current = millis();
    }
    return (uint64_t(m_bleMAC) > 0);
}


/* =============================================================================
    Wifi connection and status
============================================================================= */

/**
 * Disconnect the clients and the WiFi
 *
 * @param wifiOff If true, set WiFi mode to WIFI_OFF.
 */
void Conductor::disconnectWifi(bool wifiOff)
{
    /***** End NTP server connection *****/
    timeHelper.end();
    /***** Disconnect MQTT client *****/
    if (mqttHelper.client.connected())
    {
        mqttHelper.client.disconnect();
    }
    /***** Turn off Wifi *****/
    if (WiFi.isConnected())
    {
        WiFi.disconnect(wifiOff);
    }
}

/**
 * Attempt to connect or reconnect to the WiFi
 *
 * If the WiFi is already connected to the "right" network (see param
 * forceNewCredentials), this function will not affect the Wifi connection.
 *
 * @param forceNewCredentials If true, the function will connect to the
 * user-input SSID and password (m_userSSID and m_userPassword). If the WiFi is
 * already connected to a network with a different SSID and password, this
 * former connection will be interrupted to use the new credentials.
 */
bool Conductor::reconnect(bool forceNewCredentials)
{
    // Ensure that the WiFi is in STA mode (STA only, no AP)
    if (WiFi.getMode() != WIFI_STA)
    {
        WiFi.mode(WIFI_STA);
    }
    // Disconnect the WiFi if new credentials
    if (forceNewCredentials)
    {
        String currentSSID = WiFi.SSID();
        String currentPW = WiFi.psk();
        if (currentSSID.length() == 0 ||
            strcmp(currentSSID.c_str(), m_userSSID) > 0 ||
            currentPW.length() == 0 ||
            strcmp(currentPW.c_str(), m_userPassword) > 0 ||
            !(WiFi.localIP() == m_staticIp) ||
            !(WiFi.gatewayIP() == m_gateway) ||
            !(WiFi.subnetMask() == m_subnetMask))
        {
            // New and different user input for SSID and Password => disconnect
            // from current SSID then reconnect to new SSID
            // ESP.eraseConfig();  // ESP32_PORT_TRUE
            disconnectWifi();
            delay(1000);  // Wait for effective disconnection
        }
    }
    uint32_t current = millis();
    // Connect the WiFi if not connected
    bool wifiConnected = WiFi.isConnected();
    if (!wifiConnected) {
        if (current - m_lastConnectionAttempt < reconnectionInterval) {
            if (debugMode) {
                debugPrint("Not enough time since last connection attempt");
            }
            return false;
        }
        if (!m_credentialValidator.completed()) {
            if (debugMode) {
                debugPrint("Can't connect without credentials");
            }
            return false;
        }
        if (uint32_t(m_staticIp) > 0 && uint32_t(m_gateway) > 0 &&
            uint32_t(m_subnetMask) > 0)
        {
            WiFi.config(m_staticIp, m_gateway, m_subnetMask);
        }
        WiFi.begin(m_userSSID, m_userPassword);
        m_lastConnectionAttempt = current;
        wifiConnected = (waitForConnectResult() == WL_CONNECTED);
        if (debugMode && wifiConnected) {
            debugPrint("Connected to ", false);
            debugPrint(WiFi.SSID());
        }
    }
    // Set light sleep mode if not done // ESP32_PORT_TRUE
    if (wifiConnected && WiFi.getSleep() != WIFI_PS_MIN_MODEM) {
        WiFi.setSleep(WIFI_PS_MIN_MODEM);
    }
    return wifiConnected;
}

/**
 * Wait for WiFi connection to reach a result
 *
 * Unlike the ESP function WiFi.waitForConnectResult, this one handles a
 * connection time-out. If the timeout=0, then it just calls the ESP library
 * function.
 *
 * @return The status WL_CONNECTED, WL_CONNECT_FAILED or disconnect if
 *  STA is off.
 */
uint8_t Conductor::waitForConnectResult()
{
    if (connectionTimeout == 0)
    {
        return WiFi.waitForConnectResult();  // Use ESP function
    }
    else
    {
        uint32_t startT = millis();
        uint32_t current = startT;
        uint8_t status = WiFi.status();
        while(current - startT < connectionTimeout &&
              status != WL_CONNECTED && status != WL_CONNECT_FAILED)
        {
            delay(100);
            status = WiFi.status();
            current = millis();
        }
        if (debugMode && current - startT > connectionTimeout)
        {
            debugPrint("WiFi connection time-out");
        }
        return status;
    }
}
/**
 *
 */
void Conductor::checkOtaPacketTimeout()
{
    uint32_t now = millis();
    if (waitingForPktAck == true)
    {
        if (now - pktWaitTimeStr > otaPktAckTimeout)
        {
            waitingForPktAck = false;
            hostSerial.sendMSPCommand(MSPCommand::OTA_DNLD_FAIL, String(getOtaRca(OTA_STM_PKT_ACK_TMOUT)).c_str());//"STM_PKT_ACK_TMOUT");
            if (debugMode)
            {
                debugPrint("Exceeded OTA packet wait time-out");
            }
        }
    }
}
/**
 *
 */
void Conductor::checkWiFiDisconnectionTimeout()
{
    uint32_t now = millis();
    if (WiFi.isConnected() && mqttHelper.client.connected())
    {
        m_disconnectionTimerStart = now;
    }
    else if (now - m_disconnectionTimerStart > disconnectionTimeout)
    {
        if (debugMode)
        {
            debugPrint("Exceeded disconnection time-out");
        }
        ESP.restart(); // Resetting ESP32 if not connected to WiFi (every 100 Sec.) // ESP32_PORT_TRUE
    }
}


/* =============================================================================
    MQTT
============================================================================= */

/**
 * Main MQTT processing function - should called in main loop.
 *
 * Handles the reconnection to he MQTT server if needed and the publications.
 * NB: Makes use of IUMQTT::reconnect function, which may block the execution.
 */
void Conductor::loopMQTT()
{
    if (mqttHelper.hasConnectionInformations()) {
        if (!mqttHelper.client.connected())
        {
            mqttHelper.reconnect();
        }
        mqttHelper.client.loop();
    } else {
        uint32_t now = millis();
        if (now - m_lastMQTTInfoRequest > mqttInfoRequestDelay) {
            hostSerial.sendMSPCommand(MSPCommand::GET_MQTT_CONNECTION_INFO);
            m_lastMQTTInfoRequest = now;
        }
    }
}

/**
 * Callback called when a new MQTT message is received from MQTT server.
 *
 * The message can come from any previously subscribed topic (see
 * PubSubClient.subscribe).
 * @param topic The Pubsub topic the message came from
 * @param payload The message it self, as an array of bytes
 * @param length The number of byte in payload
 */
void Conductor::processMessageFromMQTT(const char* topic, const char* payload,
                                       uint16_t length)
{
    if (debugMode)
    {
        debugPrint("MQTT message arrived [", false);
        debugPrint(topic, false);
        debugPrint("] ", false);
        for (int i = 0; i < length; i++)
        {
            debugPrint(payload[i], false);
        }
        debugPrint("");
    }
    char *subTopic;
    subTopic = strrchr(topic, '/');
    if (subTopic == NULL || strlen(subTopic) < 2)
    {
        if (debugMode)
        {
            debugPrint("Last level of MQTT topic not found");
        }
        return;
    }
    if (debugMode)
    {
        debugPrint("Command type is: ", false);
        debugPrint(&subTopic[1]);
    }
    if (strncmp(&subTopic[1], "time_sync", 9) == 0)
    {
        timeHelper.updateTimeReferenceFromIU(payload);
        hostSerial.sendMSPCommand(MSPCommand::SET_DATETIME, payload, length);
    }
    else if (strncmp(&subTopic[1], "config", 6) == 0)
    {
        if (length > UART_TX_FIFO_SIZE)
        {
            // Command is longer than TX buffer, send it while taking care to not
            // overflow the buffer. Timeout parameter is in microseconds.
            hostSerial.sendLongMSPCommand(
                MSPCommand::CONFIG_FORWARD_CONFIG, 300000, payload, length);
        }
        else
        {
            hostSerial.sendMSPCommand(MSPCommand::CONFIG_FORWARD_CONFIG, payload,
                                      length);
        }

    }
    else if (strncmp(&subTopic[1], "command", 7) == 0)
    {
        hostSerial.sendMSPCommand(MSPCommand::CONFIG_FORWARD_CMD,
                                  payload, length);
    }
    else if (strncmp(&subTopic[1], "legacy", 6) == 0)
    {
        if (payload[0] == '1' && payload[1] == ':')
        {
            timeHelper.updateTimeReferenceFromIU(&payload[2]);
            hostSerial.sendMSPCommand(MSPCommand::SET_DATETIME, &payload[2],
                                      length);
        }
        else
        {
            hostSerial.sendMSPCommand(MSPCommand::CONFIG_FORWARD_LEGACY_CMD,
                                      payload, length);
        }
    }
    else if (strncmp(&subTopic[1], "post_url", 8) == 0)
    {
        //TODO Improve url management
        if (length < MAX_HOST_LENGTH)
        {
            strncpy(m_featurePostHost, payload, length);
            strncpy(m_diagnosticPostHost, payload, length);
        }
    }
}


/* =============================================================================
    Data posting / publication
============================================================================= */

/**
 *
 */
bool Conductor::publishDiagnostic(const char *rawMsg, const uint16_t msgLength,
                                  const char *diagnosticType,
                                  const uint16_t diagnosticTypeLength)
{
    char stringDT[25];  // Remove the newline at the end of ctime
    time_t datetime = timeHelper.getCurrentTime();
    strncpy(stringDT, ctime(&datetime), 25);
    stringDT[24] = 0;
    if (m_useMQTT)
    {
        uint16_t totalMsgLength = msgLength + CUSTOMER_PLACEHOLDER_LENGTH + 50;
        char message[totalMsgLength];
        snprintf(message, totalMsgLength, "%s;;;%s;;;%s;;;%s",
                 CUSTOMER_PLACEHOLDER, m_bleMAC.toString().c_str(),
                 stringDT, rawMsg);
        return mqttHelper.publishDiagnostic(message, diagnosticType,
                                            diagnosticTypeLength);
    } else {
        uint16_t totalMsgLength = msgLength + CUSTOMER_PLACEHOLDER_LENGTH + 98;
        char message[totalMsgLength];
        snprintf(message, totalMsgLength,
                 "{\"payload\":\"%s;;;%s;;;%s;;;%s\",\"time\":\"%s\"}",
                 // payload value
                 CUSTOMER_PLACEHOLDER, m_bleMAC.toString().c_str(), stringDT,
                 rawMsg,
                 // time value
                 stringDT);
        uint16_t routeLength = strlen(DIAGNOSTIC_DEFAULT_ENDPOINT_ROUTE) + 17;
        char route[routeLength];
        snprintf(route, routeLength, "%s%s", DIAGNOSTIC_DEFAULT_ENDPOINT_ROUTE,
                 m_bleMAC.toString().c_str());
        return httpPostBigRequest(
            m_diagnosticPostHost, route, m_diagnosticPostPort,
            (uint8_t*) message, totalMsgLength);
    }
}

/**
 *
 */
bool Conductor::publishFeature(const char *rawMsg, const uint16_t msgLength,
                               const char *featureType,
                               const uint16_t featureTypeLength)
{
    char stringDT[25];  // Remove the newline at the end of ctime
    time_t datetime = timeHelper.getCurrentTime();
    strncpy(stringDT, ctime(&datetime), 25);
    stringDT[24] = 0;
    if (m_useMQTT)
    {
        uint16_t totalMsgLength = msgLength + CUSTOMER_PLACEHOLDER_LENGTH + 24;
        char message[totalMsgLength];
        snprintf(message, totalMsgLength, "%s;;;%s;;;%s", CUSTOMER_PLACEHOLDER,
                 m_bleMAC.toString().c_str(), rawMsg);
        return mqttHelper.publishFeature(message, featureType,
                                         featureTypeLength);
    }
    else
    {
        uint16_t totalMsgLength = msgLength + CUSTOMER_PLACEHOLDER_LENGTH + 72;
        char message[totalMsgLength];
        snprintf(message, totalMsgLength,
                 "{\"payload\":\"%s;;;%s;;;%s\",\"time\":\"%s\"}",
                 // payload value
                 CUSTOMER_PLACEHOLDER, m_bleMAC.toString().c_str(), rawMsg,
                 // time value
                 stringDT);
        uint16_t routeLength = strlen(FEATURE_DEFAULT_ENDPOINT_ROUTE) + 17;
        char route[routeLength];
        snprintf(route, routeLength, "%s%s", FEATURE_DEFAULT_ENDPOINT_ROUTE,
                 m_bleMAC.toString().c_str());
        return httpPostBigRequest(
            m_featurePostHost, route, m_featurePostPort,
            (uint8_t*) message, totalMsgLength);

    }
}


/* =============================================================================
    Cyclic Update
============================================================================= */

/**
 * Format and output wifi info into a JSON-parsable char array.
 *
 * @param destination A char array to hold the function output. Must be at least
 *  250 char long.
 */
void Conductor::getWifiInfo(char *destination, uint16_t len, bool mqttOn)
{
    for (uint16_t i = 0; i < len; ++i)
    {
        destination[i] = 0;
    }
    if (len < 250)
    {
        if (debugMode)
        {
            debugPrint("destination char array is too short to "
                       "get WiFi Info");
        }
        return;
    }
    strcpy(destination, "{\"ssid\":\"");
    strcat(destination, WiFi.SSID().c_str());
    strcat(destination, "\",\"rssi\":");
    strcat(destination, String(WiFi.RSSI()).c_str());
    strcat(destination, ",\"local_ip\":\"");
    strcat(destination, WiFi.localIP().toString().c_str());
    strcat(destination, "\",\"subnet\":\"");
    strcat(destination, WiFi.subnetMask().toString().c_str());
    strcat(destination, "\",\"gateway\":\"");
    strcat(destination, WiFi.gatewayIP().toString().c_str());
    strcat(destination, "\",\"dns\":\"");
    strcat(destination, WiFi.dnsIP().toString().c_str());
    strcat(destination, "\",\"wifi_mac\":\"");
    strcat(destination, m_wifiMAC.toString().c_str());
    strcat(destination, "\",\"MQTT\":\"");
    if (mqttOn)
    {
        strcat(destination, "on");
    } else {
        strcat(destination, "off");
    }
    strcat(destination, "\"}");
}

/**
 * Publish a diagnostic message to the cloud about the WiFi connection & IDE & Wifi Firmware Version.
 */
void Conductor::publishWifiInfo()
{
    if (WiFi.isConnected() && mqttHelper.client.connected())
    {
        char message[256];
        getWifiInfo(message, 256, true);
        publishDiagnostic(message, strlen(message));
    }
}

/**
 * Periodically publish the WiFi info (see publishWifiInfo).
 */
void Conductor::publishWifiInfoCycle()
{
    if (WiFi.isConnected() && mqttHelper.client.connected())
    {
        uint32_t now = millis();
        if (now - m_lastWifiInfoPublication > wifiInfoPublicationDelay)
        {
            publishWifiInfo();
            m_lastWifiInfoPublication = now;
        }
    }
}

/**
 * Send a message to the host MCU to inform about the WiFi status.
 */
void Conductor::updateWiFiStatus()
{
    if (WiFi.isConnected() && mqttHelper.client.connected())
    {
        hostSerial.sendMSPCommand(MSPCommand::WIFI_ALERT_CONNECTED);
    }
    else
    {
        hostSerial.sendMSPCommand(MSPCommand::WIFI_ALERT_DISCONNECTED);
    }
}

/**
 * Periodically update the host MCU about the WiFi status (see updateWiFiStatus).
 */
void Conductor::updateWiFiStatusCycle()
{
    uint32_t now = millis();
    if (now - m_lastWifiStatusUpdate > wifiStatusUpdateDelay)
    {
        updateWiFiStatus();
        m_lastWifiStatusUpdate = now;
    }
}

/**
 * Periodically check WiFi status, if disconnected, try to connect using saved credentials
 */
void Conductor::autoReconncetWifi()
{
    uint32_t now = millis();
    if (now - m_lastWifiStatusCheck > (wifiStatusUpdateDelay))
    {
        WiFi.mode(WIFI_STA);
        WiFi.begin();
        m_lastWifiStatusCheck = now;
    }
}

/* =============================================================================
    Debugging
============================================================================= */

/**
 *
 */
void Conductor::debugPrintWifiInfo()
{
    #if IUDEBUG_ANY == 1
    WiFiMode_t currMode = WiFi.getMode();
    debugPrint("WiFi mode: ", false);
    debugPrint((uint8_t) currMode);
    if (currMode == WIFI_STA)
    {
        debugPrint("WiFi SSID: ", false);
        debugPrint(WiFi.SSID());
        debugPrint("WiFi psk: ", false);
        debugPrint(WiFi.psk());
        debugPrint("WiFi RSSI: ", false);
        debugPrint(WiFi.RSSI());
    }
    debugPrint("User SSID: ", false);
    debugPrint(m_userSSID);
    debugPrint("User psk: ", false);
    debugPrint(m_userPassword);
    #endif
}

/* =================================================================================
 *  get Device latest Firmware versions
 *  Function Name: sendDeviceFirmwareVersion 
 *  return : buffer with IDE and Wifi Firmware
 *  Output Format : JSON
 *  {"IDE-FIRMWARE-VR": "x.x.x", "WIFI-FIRMWARE-VR": "x.x.x"} 
 *==================================================================================*/

 void Conductor:: getDeviceFirmwareVersion(char* destination,char* HOST_VERSION, const char* WIFI_VERSION){
      //Ask Host firmware version
      //hostSerial.sendMSPCommand(MSPCommand::ASK_HOST_FIRMWARE_VERSION);
     
     uint8_t len = 255; //sizeof(destination)/sizeof(destination[0]); 
      
    for (uint16_t i = 0; i < len; ++i)
    {
        destination[i] = 0;
    }
    /*if (len < 250)  
    { 
      if (debugMode)
      {
          debugPrint("destination char array is too short to "
                         "get Version Info");
       }
          return;
    }
     */
    strcpy(destination, "{\"ide-firmware-version\":\"");
    strcat(destination, HOST_VERSION);
    strcat(destination, "\",\"wifi-firmware-version\":");
    strcat(destination, WIFI_VERSION);
    strcat(destination, "\",\"wifi_mac\":\"");
    strcat(destination, m_wifiMAC.toString().c_str());
    strcat(destination, "\",\"ble_mac\":\"");
    strcat(destination, m_bleMAC.toString().c_str());
    strcat(destination, "\",\"mqtt\":\"");
    strcat(destination, "on");
    /*if (mqttOn)
    {
        strcat(destination, "on");
    } else {
        strcat(destination, "off");
    }*/
    strcat(destination, "\"}");    
 }


bool Conductor:: otaDnldFw(bool otaDnldProgress)
{
  //  char TestStr[128];
    if(otaDnldProgress == false) 
    {
        otaInProgress = true;
        if(WiFi.status() == WL_CONNECTED) 
        {
            http_ota.end(); //  in case of error... need to close handle as it defined global
            http_ota.setTimeout(otaHttpTimeout);
            http_ota.setConnectTimeout(otaHttpTimeout);
           // hostSerial.sendLongMSPCommand(MSPCommand::ESP_DEBUG_TO_STM_HOST,300000,ota_uri,512);
            totlen = 0;
            contentLen = 0;
            if(http_ota.begin(ota_uri)) 
            {
                int httpCode = http_ota.GET();
                // httpCode will be negative on error
                if(httpCode > 0)
                {
                    // file found at server
                    if(httpCode == HTTP_CODE_OK)
                    {
                        contentLen = http_ota.getSize();
                        fwdnldLen = contentLen;
  //                      sprintf(TestStr,"contentLen:%d",contentLen);
  //                      hostSerial.sendMSPCommand(MSPCommand::ESP_DEBUG_TO_STM_HOST,TestStr,32);
                        delay(100);
                        // get tcp stream
                        WiFiClient * stream = http_ota.getStreamPtr();
                        // read all data from server
                        uint32_t otaStramStr = millis();
                        size_t size = 0;
                        do {
                            size = stream->available();
                            if(size > 0 || ((millis() - otaStramStr) > otaPktReadTimeout))
                                break;
                        } while(http_ota.connected());
                        if(size) {
                            // read up to 1024 byte
    //                        int c = stream->readBytes(ota_buff, ((size > sizeof(ota_buff)) ? sizeof(ota_buff) : size));
    //                        hostSerial.sendLongMSPCommand(MSPCommand::OTA_PACKET_DATA, 2000000, (const char*) ota_buff, OTA_PACKET_SIZE);

                            int c = stream->readBytes(httpBuffer, ((size > OTA_PACKET_SIZE) ? OTA_PACKET_SIZE : size));
                            hostSerial.sendLongMSPCommand(MSPCommand::OTA_PACKET_DATA, 5000000, (const char*) httpBuffer, c);
                            pktWaitTimeStr  = millis();
                            waitingForPktAck = true;
                            if(contentLen > 0) {
                                totlen = totlen + c;
                                contentLen -= c;
    //                            sprintf(TestStr,"Pkt:%d Read:%d Rem:%d",c,totlen,contentLen);
    //                            hostSerial.sendMSPCommand(MSPCommand::ESP_DEBUG_TO_STM_HOST,TestStr,32);
                            }                        
                        }
                        else if(size == 0 || ((millis() - otaStramStr) > otaPktReadTimeout))
                        { // Read timeout
                            hostSerial.sendMSPCommand(MSPCommand::OTA_DNLD_FAIL,String(getOtaRca(OTA_DATA_READ_TIMOUT)).c_str());//"DATA_READ_TIMOUT");
                            waitingForPktAck = false;
                        }
                        delay(1);
                    }
                    else
                    {
                       // sprintf(TestStr,"HTTP FAIL:%d",httpCode);
                        hostSerial.sendMSPCommand(MSPCommand::OTA_DNLD_FAIL,String(getOtaRca(httpCode)).c_str());
                        waitingForPktAck = false;
                        http_ota.end();
                    }
                } 
                else
                {            
                    hostSerial.sendMSPCommand(MSPCommand::OTA_DNLD_FAIL,String(getOtaRca(httpCode)).c_str());
                    waitingForPktAck = false;
                    http_ota.end();                    
                }
            }
            else
            {                    
                hostSerial.sendMSPCommand(MSPCommand::OTA_DNLD_FAIL,String(getOtaRca(OTA_HTTP_INIT_FAIL)).c_str());//"HTTP_INIT_FAIL");
                waitingForPktAck = false;
                http_ota.end();                       
            }
        }
        else
        {
            // FW Download was initiated... but failed due to WiFi disconnect
            hostSerial.sendMSPCommand(MSPCommand::OTA_DNLD_FAIL,String(getOtaRca(OTA_WIFI_DISCONNECT)).c_str());//"WIFI_DISCONNECT");
            http_ota.end();
            waitingForPktAck = false;
            return false;       
        }
    }
    else
    {
        if(WiFi.status() == WL_CONNECTED) 
        {
            if(contentLen > 0)
            {
                http_ota.setTimeout(otaHttpTimeout);
                http_ota.setConnectTimeout(otaHttpTimeout);
            // get tcp stream
                WiFiClient * stream = http_ota.getStreamPtr();
                // read all data from server
                // get available data size
                uint32_t otaStramStr = millis();
                size_t size = 0;
                do {
                    size = stream->available();
                    if(size > 0 || ((millis() - otaStramStr) > otaPktReadTimeout))
                        break;
                } while(http_ota.connected());

                if(size) {
                    // read up to 1024 byte
//                    int c = stream->readBytes(ota_buff, ((size > sizeof(ota_buff)) ? sizeof(ota_buff) : size));
//                    hostSerial.sendLongMSPCommand(MSPCommand::OTA_PACKET_DATA, 2000000, (const char*) ota_buff, OTA_PACKET_SIZE);

                    uint16_t c = stream->readBytes(httpBuffer, ((size > OTA_PACKET_SIZE) ? OTA_PACKET_SIZE : size));
                    hostSerial.sendLongMSPCommand(MSPCommand::OTA_PACKET_DATA, 5000000, (const char*) httpBuffer, c);
                    pktWaitTimeStr  = millis();
                    waitingForPktAck = true;
                    if(contentLen > 0) {
                        totlen = totlen + c;
                        contentLen -= c;
         //               sprintf(TestStr,"Pkt:%d Read:%d Rem:%d",c,totlen,contentLen);
         //               hostSerial.sendMSPCommand(MSPCommand::ESP_DEBUG_TO_STM_HOST,TestStr,32);
                    }
                }
                else if(size == 0 || ((millis() - otaStramStr) > otaPktReadTimeout))
                { // Read timeout
                    hostSerial.sendMSPCommand(MSPCommand::OTA_DNLD_FAIL,String(getOtaRca(OTA_DATA_READ_TIMOUT)).c_str());//"DATA_READ_TIMOUT");
                    waitingForPktAck = false;
                }
                delay(1);
            }
            else
            {
                if(fwdnldLen == totlen)
                {
                    if(strcmp(ota_uri,otaStm_uri) == 0)
                    {
                        hostSerial.sendMSPCommand(MSPCommand::OTA_STM_DNLD_STATUS);
                        fwdnldLen = 0;
                        totlen = 0;
                        waitingForPktAck = false;
                        http_ota.end();
                    }
                    if(strcmp(ota_uri,otaEsp_uri) == 0)
                    {
                        hostSerial.sendMSPCommand(MSPCommand::OTA_ESP_DNLD_STATUS);
                        fwdnldLen = 0;
                        totlen = 0;
                        waitingForPktAck = false;
                        http_ota.end();
                    }                                        
                }
            }            
        }
        else
        {
            if(fwdnldLen != totlen && contentLen > 0)
            { // FW Download is not completed... and was in progress
                hostSerial.sendMSPCommand(MSPCommand::OTA_DNLD_FAIL,String(getOtaRca(OTA_WIFI_DISCONNECT)).c_str());//"WIFI_DISCONNECT");
            }
            http_ota.end();
            waitingForPktAck = false;
            return false;  
        }        
    }
}

/**
 * Get RCA Reason code based on HTTP error code value
 * @param error int
 * @return String
 * OTA-RCA-0001 to OTA-RCA-0010 - Used at STM code for sending OTA Failure reason code
 * 
 */
String Conductor::getOtaRca(int error)
{
    switch(error) {
        case OTA_WIFI_DISCONNECT:
            /* WiFi Disconnection error handled both at STM and ESP, with same RCA Code = 3  */
            return F("OTA-RCA-0003");
        case OTA_HTTP_INIT_FAIL:
            return F("OTA-RCA-0011");
        case OTA_DATA_READ_TIMOUT:
            return F("OTA-RCA-0012");
        case OTA_STM_PKT_ACK_TMOUT:
            return F("OTA-RCA-0013");
        case HTTPC_ERROR_CONNECTION_REFUSED:
            return F("OTA-RCA-0014");
        case HTTPC_ERROR_SEND_PAYLOAD_FAILED:
            return F("OTA-RCA-0015");
        case HTTPC_ERROR_NOT_CONNECTED:
            return F("OTA-RCA-0016");
        case HTTPC_ERROR_CONNECTION_LOST:
            return F("OTA-RCA-0017");
        case HTTPC_ERROR_NO_STREAM:
            return F("OTA-RCA-0018");;
        case HTTPC_ERROR_NO_HTTP_SERVER:
            return F("OTA-RCA-0019");
        case HTTPC_ERROR_TOO_LESS_RAM:
            return F("OTA-RCA-0020");
        case HTTPC_ERROR_ENCODING:
            return F("OTA-RCA-0021");
        case HTTPC_ERROR_STREAM_WRITE:
            return F("OTA-RCA-0022");
        case HTTPC_ERROR_READ_TIMEOUT:
            return F("OTA-RCA-0023");
        case HTTP_CODE_BAD_REQUEST:
            return F("OTA-RCA-0024");
        case HTTP_CODE_FORBIDDEN:
            return F("OTA-RCA-0025");
        case HTTP_CODE_NOT_FOUND:
            return F("OTA-RCA-0026");
        case HTTP_CODE_METHOD_NOT_ALLOWED:
            return F("OTA-RCA-0027");
        case HTTP_CODE_NOT_ACCEPTABLE:
            return F("OTA-RCA-0028");
        case HTTP_CODE_PROXY_AUTHENTICATION_REQUIRED:
            return F("OTA-RCA-0029");
        case HTTP_CODE_REQUEST_TIMEOUT:
            return F("OTA-RCA-0030");
        case HTTP_CODE_INTERNAL_SERVER_ERROR:
            return F("OTA-RCA-0031");
        case HTTP_CODE_NOT_IMPLEMENTED:
            return F("OTA-RCA-0032");
        case HTTP_CODE_BAD_GATEWAY:
            return F("OTA-RCA-0033");
        case HTTP_CODE_SERVICE_UNAVAILABLE:
            return F("OTA-RCA-0034");
        case HTTP_CODE_GATEWAY_TIMEOUT:
            return F("OTA-RCA-0035");
        case HTTP_CODE_HTTP_VERSION_NOT_SUPPORTED:
            return F("OTA-RCA-0036");
        case HTTP_CODE_VARIANT_ALSO_NEGOTIATES:
            return F("OTA-RCA-0037");
        case HTTP_CODE_INSUFFICIENT_STORAGE:
            return F("OTA-RCA-0038");
        case HTTP_CODE_LOOP_DETECTED:
            return F("OTA-RCA-0039");
        case HTTP_CODE_NOT_EXTENDED:
            return F("OTA-RCA-0040");
        case HTTP_CODE_NETWORK_AUTHENTICATION_REQUIRED:
            return F("OTA-RCA-0041"); 
        default:
            return F("OTA-RCA-1111");
    }
}