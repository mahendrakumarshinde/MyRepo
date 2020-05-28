#include "Conductor.h"
#include "Utilities.h"
// #include <ArduinoTrace.h>
#include <base64.h>

#define UART_TX_FIFO_SIZE 0x80
#define OTA_PACKET_SIZE 1024
/* =============================================================================
    Instanciation
============================================================================= */

char hostSerialBuffer[8500];

IUSerial hostSerial(&Serial, hostSerialBuffer, 8500, IUSerial::MS_PROTOCOL,
                    115200, ';', 1000);

IURawDataHelper accelRawDataHelper(10000,  // 10s timeout to input all keys
                                   300000,  // 5min timeout to succefully post data
                                   DATA_DEFAULT_ENDPOINT_HOST,
                                   RAW_DATA_DEFAULT_ENDPOINT_ROUTE,
                                   DATA_DEFAULT_ENDPOINT_PORT);

IUMQTTHelper mqttHelper = IUMQTTHelper();

IUTimeHelper timeHelper(2390, "time.google.com");

IUESPFlash iuWiFiFlash = IUESPFlash();

IUESPFlash::storedConfig Conductor::CONFIG_TYPES[Conductor::CONFIG_TYPE_COUNT] = {
    IUESPFlash::CFG_EAP_CLIENT0,
    IUESPFlash::CFG_EAP_KEY0,
    IUESPFlash::CFG_MQTT_CLIENT0,
    IUESPFlash::CFG_MQTT_KEY0,
    IUESPFlash::CFG_HTTPS_ROOTCA0,
    // Used only during rollback/upgrade
    IUESPFlash::CFG_EAP_CLIENT1,
    IUESPFlash::CFG_EAP_KEY1,
    IUESPFlash::CFG_MQTT_CLIENT1,
    IUESPFlash::CFG_MQTT_KEY1,
    IUESPFlash::CFG_HTTPS_ROOTCA1
    
    };

const char* CERT_TYPES[5] = { "EAP-TLS-CERT",
                              "EAP-TLS-KEY",
                              "MQTT-TLS-CERT",
                              "MQTT-TLS-KEY",
                              "SSL"};
/* =============================================================================
    Conductor
============================================================================= */

Conductor::Conductor() :
    m_useMQTT(true),
    m_lastMQTTInfoRequest(0),
    m_lastConnectionAttempt(0),
    m_disconnectionTimerStart(0),
    m_disconnectionMqttTimerStart(0),
    m_lastWifiStatusUpdate(0),
    m_lastWifiStatusCheck(0),
    m_lastWifiInfoPublication(0),
    m_mqttServerIP()
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
            if(otaInProgress == true) {
                mqttHelper.publish(OTA_TOPIC,buffer);
            }
            otaInitTimeoutFlag = false;
            break;
        case MSPCommand::OTA_FDW_START:
            if(otaInProgress == true) {    
                mqttHelper.publish(OTA_TOPIC,buffer);
                strncpy(ota_uri, otaStm_uri, 512);
                delay(100);
                otaDnldFw(false); //false => Start of FW Dnld
            }
            otaInitTimeoutFlag = false;
            break;
        case MSPCommand::OTA_FDW_ABORT:
            mqttHelper.publish(OTA_TOPIC,buffer);
            http_ota.end();
            otaInProgress = false;
            waitingForPktAck = false;
            otaInitTimeoutFlag = false;
            break;
        case MSPCommand::OTA_FUG_START:
            mqttHelper.publish(OTA_TOPIC,buffer);
            otaInitTimeoutFlag = false;
            break;
        case MSPCommand::OTA_FUG_SUCCESS:
            mqttHelper.publish(OTA_TOPIC,buffer);
            otaInProgress = false;
            waitingForPktAck = false;
            otaInitTimeoutFlag = false;
            delay(100);
            break;
        case MSPCommand::OTA_FUG_ABORT:
            mqttHelper.publish(OTA_TOPIC,buffer);
            otaInProgress = false;
            waitingForPktAck = false;
            otaInitTimeoutFlag = false;
            delay(100);
            break;
        case MSPCommand::OTA_PACKET_ACK:
            waitingForPktAck = false;
            otaInitTimeoutFlag = false;
            otaDnldFw(true);   //true => Cont. of FW Dnld
            break;
        case MSPCommand::OTA_STM_DNLD_OK:
            strncpy(ota_uri, otaEsp_uri, 512);
            otaDnldFw(false); //false => Start of FW Dnld
            break;
        case MSPCommand::OTA_ESP_DNLD_OK:
            break;
        case MSPCommand::OTA_FDW_SUCCESS:
            mqttHelper.publish(OTA_TOPIC,buffer);
//            otaInProgress = false; // Temp. Only after Download+Upgrade+Validation this shall be set to false
            waitingForPktAck = false;
            otaInitTimeoutFlag = false;
            break;
        case MSPCommand::SET_OTA_STM_URI:
            strcpy(otaStm_uri,buffer);
            delay(1);
            break;
        case MSPCommand::SET_OTA_ESP_URI:
            strcpy(otaEsp_uri,buffer);
            delay(1);
            break;
        case MSPCommand::SET_TLS_CERT_URI:
            strcpy(otaStm_uri,buffer);      // using the ota buffer for cert uri 
            delay(1);
            break;
        case MSPCommand::SET_TLS_KEY_URI:
            strcpy(otaEsp_uri,buffer);
            delay(1);
            break;
        case MSPCommand::TLS_INIT_ACK:
            {
                // if(otaInProgress == true) {
                //     mqttHelper.publish(OTA_TOPIC,buffer);
                // }
                // otaInitTimeoutFlag = false;
                // memset(client_cert,0x00,2048);
                // memset(client_key,0x00,2048);
                // bool certDownloadDone = false;
                // bool keyDownloadDone = false;
                //certDownloadDone = getDeviceCertificates(otaStm_uri,"/tls/client.crt");           
                // delay(1);
                // keyDownloadDone = getDeviceCertificates(otaEsp_uri,"/tls/client.key");
                // delay(1);
                
                // readCertificatesFromFlash(SPIFFS,"/tls/client.crt");
                // //delay(1);
                // //readCertificatesFromFlash(SPIFFS,"/tls/client.key");
                // delay(1);
                // Serial.println("CERT Reading completed....");
                
                // hostSerial.sendLongMSPCommand(MSPCommand::ESP_DEBUG_TO_STM_HOST,3000000,client_cert,sizeof(client_cert));
                // delay(10);
                // hostSerial.sendLongMSPCommand(MSPCommand::ESP_DEBUG_TO_STM_HOST,3000000,client_key,sizeof(client_key));
                // delay(10);
                // memset(client_cert,0x00,2048);
                // memset(client_key,0x00,2048);
                // ReadBinFile(SPIFFS,"/eapTlsCert.crt");
                // ReadBinFile(SPIFFS,"/eapTlsKey.key");
                // delay(1);
                // hostSerial.sendLongMSPCommand(MSPCommand::ESP_DEBUG_TO_STM_HOST,3000000,client_cert,1200);
                // delay(100);
                // hostSerial.sendLongMSPCommand(MSPCommand::ESP_DEBUG_TO_STM_HOST,3000000,client_key,1700);
                // delay(100);
                break;
            }
        case MSPCommand::SET_DIAGNOSTIC_URL:
            updateDiagnosticEndpoint(buffer,bufferLength);
            break;
        case MSPCommand::SEND_WIFI_CONFIG:
            updateWiFiConfig(buffer,bufferLength);
            setWiFiConfig();
            break;
        case MSPCommand::CERT_DOWNLOAD_INIT_ACK:
            {
                //Serial.print("CERT DOWNLOAD INIT ACK :");Serial.println(buffer);
                publishedDiagnosticMessage(buffer,bufferLength);
                certDownloadInitAck = true;
                downloadInitTimer = false;
                // trigger the config downloading can send the messageID 
                hostSerial.sendMSPCommand(MSPCommand::GET_CERT_DOWNLOAD_CONFIG);
            break;
            }
        case MSPCommand::CERT_UPGRADE_INIT:
            //Serial.print("CERT UPGRADE INIT :");Serial.println(buffer);
            publishedDiagnosticMessage(buffer,bufferLength);          
            break;
            
        case MSPCommand::CERT_UPGRADE_SUCCESS:
            //Serial.print("CERT UPGRADE Success :");Serial.println(buffer);
            //mqttHelper.publish(CERT_STATUS_TOPIC,buffer);
            upgradeReceived = false;
            downloadInitTimer = true;
            publishedDiagnosticMessage(buffer,bufferLength);
            break;
        case MSPCommand::CERT_UPGRADE_ABORT:
            //Serial.print("CERT UPGRADE Failed/ABORTED :");Serial.println(buffer);
            publishedDiagnosticMessage(buffer,bufferLength);
            if(newDownloadConnectonAttempt > maxMqttCertificateDownloadCount){ 
                downloadInitTimer = false;
                upgradeReceived = false; }else{
                     downloadInitTimer = true;
                     upgradeReceived = false;
           }
            break;
        case MSPCommand::CERT_DOWNLOAD_ABORT:
            //Serial.print("CERT DOWNLOAD ABORTED :");Serial.println(buffer);
            otaInProgress = false;
            waitingForPktAck = false;
            otaInitTimeoutFlag = false;
            publishedDiagnosticMessage(buffer,bufferLength);
            //delay(10);
            upgradeReceived = false;
            downloadInitTimer = true;
            break;
                    
        case MSPCommand::CERT_DOWNLOAD_SUCCESS:
             //Serial.print("CERT DOWNLOAD Success : ");Serial.println(buffer);
             otaInProgress = false;
             waitingForPktAck = false;
             otaInitTimeoutFlag = false;
             downloadInitTimer = true;
             publishedDiagnosticMessage(buffer,bufferLength);
             delay(10);
             
             break;
            
        case MSPCommand::CERT_NO_DOWNLOAD_AVAILABLE:
            //Serial.print("NO DOWNLOAD AVAILABLE :");Serial.println(buffer);
            publishedDiagnosticMessage(buffer,bufferLength);
            break;
            
        case MSPCommand::ALL_MQTT_CONNECT_ATTEMPT_FAILED:
            //Serial.print("All Connection Attempt Failed : ");Serial.println(buffer);
            // TODO :Send to Diagnostic endpoint if mqtt not connected
            publishedDiagnosticMessage(buffer,bufferLength);
            break;
        case MSPCommand::CERT_UPGRADE_TRIGGER:
            //Serial.println("RECEIVED THE CERT UPGRADE COMMAND, Init Download process");
            upgradeReceived = true;
            downloadInitTimer = false;
            iuSerial->sendMSPCommand(MSPCommand::CERT_DOWNLOAD_INIT,CERT_DOWNLOAD_DEFAULT_MESSAGEID);
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
        case MSPCommand::GET_DEVICE_CONFIG: 
             if(strlen(buffer)>0){
                char *str;
                int i=0;
                while((str = strtok_r(buffer,"-",&buffer)) != NULL)
                {
                    if(i==0)
                    {
                        strcpy(HOST_FIRMWARE_VERSION,str);
                        i++;
                    }else if(i == 1){
                        HOST_SAMPLING_RATE=atoi(str);
                        i++;
                    }else if(i == 2){
                        HOST_BLOCK_SIZE=atoi(str);
                        i++;
                    }
                
                }
                configStatus = true;
            }
            else{
                configStatus = false;
            }
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
        case MSPCommand::WIFI_RECEIVE_AUTH_TYPE:       
            strcpy(m_wifiAuthType,buffer);
            hostSerial.sendMSPCommand(MSPCommand::ESP_DEBUG_TO_STM_HOST, m_wifiAuthType);
            delay(1);
            break;
        case MSPCommand::WIFI_FORGET_CREDENTIALS:
            forgetWiFiCredentials();
            disconnectWifi();
            break;
        case MSPCommand::WIFI_RECEIVE_STATIC_IP:
        case MSPCommand::WIFI_RECEIVE_GATEWAY:
        case MSPCommand::WIFI_RECEIVE_SUBNET:
        case MSPCommand::WIFI_RECEIVE_DNS1:
        case MSPCommand::WIFI_RECEIVE_DNS2:
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
        case MSPCommand::MQTT_CONNECT:
            // Reset disconnection timer
            m_disconnectionMqttTimerStart = millis();
            loopMQTT();
            break;
        case MSPCommand::MQTT_DISCONNECT:
            disconnectMQTT();
            break;
        
        // case MSPCommand::GET_ESP_RSSI:
        //     hostSerial.sendMSPCommand(MSPCommand::GET_ESP_RSSI,String(WiFi.RSSI()).c_str() );
        //     break;
        /****** WiFI Radio Control *****/
        case MSPCommand::WIFI_GET_TX_POWER:
            wifi_power_t txpower;
            txpower  =  WiFi.getTxPower();
            char esp_radio[50];
            snprintf(esp_radio,50,"ESP32 Current TX PWR : %d",txpower);
            hostSerial.sendMSPCommand(MSPCommand::ESP_DEBUG_TO_STM_HOST, esp_radio);
            mqttHelper.publish(DIAGNOSTIC_TOPIC,esp_radio);
            break;
        case MSPCommand::WIFI_SET_TX_POWER:
            {
            int cmd(0), radioMode(0);
            int modeSet,currentTXpower;
            sscanf(buffer, "%d-%d", &cmd, &radioMode);
            char esp_radio[50];
            switch (radioMode)
            {
                case 0:
                    modeSet = WiFi.setTxPower(WIFI_POWER_19_5dBm);
                    break;
                case 1:
                    modeSet = WiFi.setTxPower(WIFI_POWER_19dBm);
                    break;
                case 2:
                    modeSet = WiFi.setTxPower(WIFI_POWER_18_5dBm);
                    break;
                case 3:
                    modeSet = WiFi.setTxPower(WIFI_POWER_17dBm);
                    break;
                case 4:
                    modeSet = WiFi.setTxPower(WIFI_POWER_15dBm);
                    break;
                case 5:
                    modeSet = WiFi.setTxPower(WIFI_POWER_13dBm);
                    break;
                case 6:
                    modeSet = WiFi.setTxPower(WIFI_POWER_11dBm);
                    break;
                case 7:
                    modeSet = WiFi.setTxPower(WIFI_POWER_8_5dBm);
                    break;
                case 8:
                    modeSet = WiFi.setTxPower(WIFI_POWER_7dBm);
                    break;
                case 9:
                    modeSet = WiFi.setTxPower(WIFI_POWER_5dBm);
                    break;
                case 10:
                    modeSet = WiFi.setTxPower(WIFI_POWER_2dBm);
                    break;
                case 11:
                    modeSet = WiFi.setTxPower(WIFI_POWER_MINUS_1dBm);
                    break;
    
                default:
                    break;
            }
            // currentTXpower = WiFi.getTxPower();
            // snprintf(esp_radio,50,"ESP32 TX PWR : %d,%d,%d,%d",cmd,radioMode,modeSet,currentTXpower);
            // hostSerial.sendMSPCommand(MSPCommand::ESP_DEBUG_TO_STM_HOST, esp_radio);
            
            break;
            }

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

            if (m_mqttServerValidator.hasTimedOut()) {
                m_mqttServerValidator.reset();
            }
            strncpy(m_mqttServerIP, buffer, IUMQTTHelper::credentialMaxLength);

            m_mqttServerValidator.receivedMessage(0);
            if (m_mqttServerValidator.completed()) {
                mqttHelper.setServer(m_mqttServerIP, m_mqttServerPort);
                //hostSerial.write("RECEIVED MQTT SERVER IP FROM DEVICE ");
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
        case MSPCommand::SET_MQTT_TLS_FLAG:
             m_tls_enabled = bool(strtol(buffer, NULL, 0));
            //  hostSerial.write("TLS Status :");hostSerial.write(m_tls_enabled);
            //  hostSerial.write("\n");
             mqttHelper.TLS_ENABLE = m_tls_enabled;
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
            memset(ssl_rootca_cert,0x00,2048);
            //Apply the rootCA cert
            if(activeCertificates == 0){
                //Serial.println("\nUsing ROOTCA 0");
                iuWiFiFlash.readFile(IUESPFlash::CFG_HTTPS_ROOTCA0,ssl_rootca_cert,sizeof(ssl_rootca_cert));
            }else
            {   //Serial.println("\nUsing rootCA 1");
                iuWiFiFlash.readFile(IUESPFlash::CFG_HTTPS_ROOTCA1,ssl_rootca_cert,sizeof(ssl_rootca_cert));
            }
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
            
            int httpStatusCode = httpsPostBigRequest(accelRawDataHelper.m_endpointHost, accelRawDataHelper.m_endpointRoute,
                                            accelRawDataHelper.m_endpointPort, (uint8_t*) &httpBuffer, 
                                            httpBufferPointer,"", ssl_rootca_cert, HttpContentType::octetStream);            

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
        case MSPCommand::SEND_SENSOR_STATUS:
          // Send the Ack to Topic
            mqttHelper.publish(COMMAND_RESPONSE_TOPIC,buffer);
            break;
        case MSPCommand::DOWNLOAD_TLS_SSL_START:
                certificateDownloadInitInProgress = true;
                certificateDownloadInProgress = true;
                if(downloadAborted  == false && (newEapCertificateAvailable == true || newEapPrivateKeyAvailable == true || 
                    newMqttcertificateAvailable == true || newMqttPrivateKeyAvailable == true || newRootCACertificateAvailable == true || upgradeReceived) ){
                    // Note : upgradeReceived flage is optional here, in upgrade new cert are having different checksum above all  flags will alreay set to true
                    // added temp for testing upgrade    
                    //Serial.println("\nESP32 DEBUG : DOWNLOADING STARTED ....");
                    publishedDiagnosticMessage(buffer,bufferLength);
                    certificateDownloadStatus = download_tls_ssl_certificates();
                    if(certificateDownloadStatus == 1){
                        hostSerial.sendMSPCommand(MSPCommand::CERT_DOWNLOAD_SUCCESS, String(getRca(CERT_DOWNLOAD_COMPLETE)).c_str());
                        if(activeCertificates == 1){
                            if(initialFileDownload){
                                //Serial.println("Initial File Download , Updated Value ");
                                initialFileDownload = false;
                            }
                           activeCertificates = iuWiFiFlash.updateValue(ADDRESS,0);
                        }else{
                           activeCertificates = iuWiFiFlash.updateValue(ADDRESS,1);
                        }
                        //Serial.println("\nESP32 DEBUG : DOWNLOADING SUCCESSFULLY COMPLETED....");
                    }
                }else
                {
                    //Serial.println("\nESP32 DEBUG : No Need to Update the Certificates");
                    hostSerial.sendMSPCommand(MSPCommand::CERT_NO_DOWNLOAD_AVAILABLE, String(getRca(CERT_NEW_CERT_NOT_AVAILBLE)).c_str());
                }
                   
                delay(200);
                certDownloadInitAck = false;
                certificateDownloadInProgress = false;
                certificateDownloadInitInProgress = false;
                newMqttcertificateAvailable = false;
                newEapPrivateKeyAvailable = false;
                newMqttcertificateAvailable = false;  
                newMqttPrivateKeyAvailable = false;
                newRootCACertificateAvailable = false;
                //Serial.println("\nESP32 DEBUG : ALL FLAGS RESET ....");    
                
            break;
        case MSPCommand::UPGRADE_TLS_SSL_START:
             // TODO : Upgrade the Certificate read from the file and Upgrade

            break;
        case MSPCommand::SET_CERT_CONFIG_URL:
          {
            // Store the URL into .conf file
            bool written = iuWiFiFlash.writeFile(IUESPFlash::CFG_STATIC_CERT_ENDPOINT,buffer,bufferLength);  
            if(written && buffer != NULL){
                // Serial.println("\nESP32 DEBUG : static url config type success"); 
                 //downloadInitTimer = true;  
                 if(debugMode){
                     debugPrint("HTTP Common Endpoint configured successfully");
                 }
            }else
            {
                // Failed to write to Flash File system, or Empty Buffer received 
                // TODO : Publish status to diagnostic endpoint
                if(!written){
                    hostSerial.sendMSPCommand(MSPCommand::ESP32_FLASH_FILE_WRITE_FAILED);
                    //Serial.println("\nESP32 DEBUG : ESP32_FLASH_FILE_WRITE_FAILED");
                }else if(buffer == NULL ) {
                    //Serial.println("\nESP32 DEBUG : Empty URL received");
                }
                hostSerial.sendMSPCommand(MSPCommand::CERT_INVALID_STATIC_URL);               
                    
            }
            break;
          }
        case MSPCommand::GET_CERT_DOWNLOAD_CONFIG:
          {
            StaticJsonBuffer<512> jsonBuffer;
            JsonVariant config = JsonVariant(iuWiFiFlash.loadConfigJson(IUESPFlash::CFG_STATIC_CERT_ENDPOINT,jsonBuffer));
            bool success = config.success();
            // Serial.print("COMMON URL JSON :");
            // config.prettyPrintTo(Serial);
            
            if( success && WiFi.isConnected() ){
                
                const char* url = config["certUrl"]["url"] ;  
                String commonUrl = url ;
                commonUrl +=  m_bleMAC.toString();  // append mac id
                const char* staticURL = commonUrl.c_str();
                //Serial.println("\nESP32 DEBUG: GET the Certificates Download Config");
                String auth = setBasicHTTPAutherization();
                int httpCode = httpGetRequest(staticURL,certDownloadResponse,sizeof(certDownloadResponse),auth); 
                int16_t responseLength = sizeof(certDownloadResponse);
                //Serial.print("\nHTTP CODE : ");Serial.println(httpCode);

                if( responseLength >0 && httpCode == HTTP_CODE_OK  ) {
                    char jsonResponse[responseLength+1];
                    strcpy(jsonResponse,certDownloadResponse);
                        // Checksum valication to check for new cert download
                        messageValidation(jsonResponse);            
                        if( newEapCertificateAvailable || newEapPrivateKeyAvailable ||  newRootCACertificateAvailable || 
                            newMqttcertificateAvailable || newMqttPrivateKeyAvailable || upgradeReceived )  // Note : upgradeReceived flasg is optional here
                            { 
                                // Store the response message in esp32 flash file system
                                bool configWritten = iuWiFiFlash.writeFile(IUESPFlash::CFG_CERT_UPGRADE_CONFIG,jsonResponse, sizeof(jsonResponse) );
                                if(!configWritten){
                                    // Failed to write to Flash File system 
                                    // TODO : Publish status to diagnostic endpoint
                                    hostSerial.sendMSPCommand(MSPCommand::ESP32_FLASH_FILE_WRITE_FAILED);
                                    //Serial.println("\nESP32 DEBUG : ESP32_FLASH_FILE_WRITE_FAILED");
                                    delay(1); 
                                    //hostSerial.sendMSPCommand(MSPCommand::CERT_DOWNLOAD_ABORT);
                                    hostSerial.sendMSPCommand(MSPCommand::CERT_DOWNLOAD_ABORT,String(getRca(CERT_FILESYSTEM_READ_WRITE_FAILED)).c_str());
                                    downloadAborted = true;
                                    upgradeReceived = false;
                                }else
                                {
                                    //Serial.println("\nESP32 DEBUG : Config JOSN File  Upgrade completed ---------");
                                    hostSerial.sendMSPCommand(MSPCommand::DOWNLOAD_TLS_SSL_START);
                                    downloadAborted = false;
                                    
                                }
                            }
                
                }else
                {
                    // https GET request failed
                    // Send the ACK / NACK to Diagnostic endpoints and inform the STM32 and stop the OTA/Certificate Download mode
                    // TODO : Publish status to diagnostic endpoints
                    // TODO : Get the http error code based on above httpCode 
                    if(httpCode != HTTP_CODE_OK) {
                        // Send the HTTP Faliure Code with RCA
                        //Serial.print("\nESP32 DEBUG : HTTP Failed : "); Serial.println(httpCode);
                        certificateDownloadInProgress =true;    // Set to send the RCA
                        hostSerial.sendMSPCommand(MSPCommand::CERT_DOWNLOAD_ABORT,String(getRca(httpCode)).c_str());
                        certificateDownloadInProgress = false;
                        downloadAborted = true;
                        
                    }
                    downloadAborted = true;
                    upgradeReceived = false;
                    //hostSerial.sendMSPCommand(MSPCommand::CERT_DOWNLOAD_ABORT);
                    //Serial.println("\nESP32 DEBUG :STOP CERT Download Mode");

                }
                
            }else
            {
                // Invalid Response Json or WiFI client not connected 
                // TODO : Publish status to diagnostic endpoint
                if(! WiFi.isConnected() || !success ) {
                    
                    if(!success) {
                        //Serial.println("\nESP32 DEBUG : Invalid Static URL Config JSON Response or File Not available");
                        hostSerial.sendMSPCommand(MSPCommand::CERT_DOWNLOAD_ABORT,String(getRca(CERT_STATIC_URL_FILE_NOT_PRESENT)).c_str() );
                    }else{ 
                        //Serial.println("\nESP32 DEBUG : Wifi client disconnected ");
                    }

                }
                downloadAborted = true;
                upgradeReceived = false;                    
            }
            break;
          }
        
        case MSPCommand::DELETE_CERT_FILES:
            for (size_t i = 0; i < CONFIG_TYPE_COUNT; i++)
            {            
                iuWiFiFlash.removeFile(CONFIG_TYPES[i]);
                delay(1);
            }
            //iuWiFiFlash.removeFile(IUESPFlash::CFG_STATIC_CERT_ENDPOINT);
            iuWiFiFlash.updateValue(ADDRESS,0);
            hostSerial.sendMSPCommand(MSPCommand::DELETE_CERT_FILES,"succefully Deleted");
            break;
        case MSPCommand::READ_CERTS:
            {
                int size1 = iuWiFiFlash.getFileSize(IUESPFlash::CFG_MQTT_CLIENT0);
                int size2 = iuWiFiFlash.getFileSize(IUESPFlash::CFG_MQTT_KEY0);
                int size3 = iuWiFiFlash.getFileSize(IUESPFlash::CFG_CERT_UPGRADE_CONFIG);
                int size4 = iuWiFiFlash.getFileSize(IUESPFlash::CFG_EAP_CLIENT0);
                int size5 = iuWiFiFlash.getFileSize(IUESPFlash::CFG_EAP_KEY0);
                //int size6 = iuWiFiFlash.getConfigFilename(IUESPFlash::CFG_STATIC_CERT_ENDPOINT);
                char cert[size1],eapcert[size3];
                char key[size2] ,eapkey[size5] ;
                char url[200];
                char configdata[size3];
                iuWiFiFlash.readFile(IUESPFlash::CFG_STATIC_CERT_ENDPOINT,url,200);
            
                Serial.println("Static CONFIG :");
                for (size_t i = 0; i < 200; i++)
                {
                    Serial.print(url[i]);
                }
                Serial.println("static config READ COMPLETED");
                break;
            }
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
        m_username[i] = 0;
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
        m_disconnectionMqttTimerStart = millis();
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
    m_dns1 = IPAddress();
    m_dns2 = IPAddress();
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
    else if (idx == 3) { m_dns1 = hostSerial.mspReadIPAddress(); }
    else if (idx == 4) { m_dns2 = hostSerial.mspReadIPAddress(); }
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
    while (uint64_t(m_bleMAC) == 0 && (millis() - startTime) < 10000 )
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
        delay(10);
        hostSerial.sendMSPCommand(MSPCommand::GET_MQTT_CONNECTION_INFO);    //Need to Change the call of this
        delay(10);
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
String Conductor :: IpAddress2String(const IPAddress& ipAddress)
{
  return String(ipAddress[0]) + String(".") +\
  String(ipAddress[1]) + String(".") +\
  String(ipAddress[2]) + String(".") +\
  String(ipAddress[3])  ; 
}
/**
 * Disconnect the clients 
 *
 * @param .
 */
void Conductor::disconnectMQTT()
{
    /***** End NTP server connection *****/
    timeHelper.end();
    /***** Disconnect MQTT client *****/
    if (mqttHelper.client.connected())
    {
        mqttHelper.client.disconnect();
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

        connectToWiFi();

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

void Conductor::connectToWiFi(){

    if (WiFi.getMode() != WIFI_STA)
    {
        WiFi.mode(WIFI_STA);
    }

    if(strncmp(m_wifiAuthType, "NONE", 4)==0)
    {
        WiFi.begin(m_userSSID);
    }
    else if(strncmp(m_wifiAuthType, "WPA-PSK", 7) == 0)
    {
        WiFi.begin(m_userSSID, m_userPassword);
    }
    else if(strncmp(m_wifiAuthType, "STATIC-NONE", 11) == 0 )
    {
        WiFi.config(m_staticIp, m_gateway, m_subnetMask, m_dns1, m_dns2);
        delay(200);
        WiFi.begin(m_userSSID);
    }
    else if(strncmp(m_wifiAuthType, "STATIC-WPA-PSK", 14) == 0)
    {
        WiFi.config(m_staticIp, m_gateway, m_subnetMask, m_dns1, m_dns2);
        delay(200);
        WiFi.begin(m_userSSID, m_userPassword);
    }
    else if(strncmp(m_wifiAuthType, "EAP-PEAP", 8) == 0)
    {
        // TODO Implemenet
    }
    else if(strncmp(m_wifiAuthType, "EAP-TLS", 7) == 0)
    {
        // TODO Implement
    }
    else if(strncmp(m_wifiAuthType, "STATIC-EAP-PEAP", 15) == 0)
    {
        // TODO Implement
    }
    else if(strncmp(m_wifiAuthType, "STATIC-EAP-TLS", 14) == 0)
    {
        // TODO Implement
    }
    else
    {
        WiFi.begin();   // Connects with Internal storage credentials
    }
    
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
            hostSerial.sendMSPCommand(MSPCommand::OTA_DNLD_FAIL, String(getRca(OTA_STM_PKT_ACK_TMOUT)).c_str());//"STM_PKT_ACK_TMOUT");
            if (debugMode)
            {
                debugPrint("Exceeded OTA packet wait time-out");
            }
        }
    }
    if(otaInitTimeoutFlag == true)
    {
        if((millis() - otaInitTimeout) > 3000) {
            otaInitTimeoutFlag = false;
            otaInProgress = false;
            if (debugMode)
            {
                debugPrint("Exceeded OTA Init wait time-out");
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
    if (WiFi.isConnected() /*&& mqttHelper.client.connected()*/)
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

/**
 * @brief 
 * 
 */
void Conductor::checkMqttDisconnectionTimeout()
{
    uint32_t now = millis();
    if ( mqttHelper.client.connected())
    {
        m_disconnectionMqttTimerStart = now;
        //Serial.println("\nSet the m_disconnectionTimerStart");
    }
    else if (now - m_disconnectionMqttTimerStart > disconnectionTimeout)
    {
        if (debugMode)
        {
            debugPrint("Exceeded mqtt disconnection time-out");
        }
       certificateDownloadInProgress = false;    
       certDownloadInitAck = false;
       downloadInitTimer = true;

       newMqttcertificateAvailable = false;
       newEapPrivateKeyAvailable = false;
       newMqttcertificateAvailable = false;
       newMqttPrivateKeyAvailable = false;
       newRootCACertificateAvailable = false;
       m_disconnectionMqttTimerStart = now;
       //Serial.println("Exceeded mqtt disconnection time-out");
       hostSerial.sendMSPCommand(MSPCommand::ESP_DEBUG_TO_STM_HOST,"ESP32 DEBUG : Exceeded MQTT disconnction timeout");
        // reconnect mqtt client
        //loopMQTT();
    }
    mqttHelper.client.loop();
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
        if(mqttHelper.client.connected()){
          m_disconnectionMqttTimerStart = millis();  
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
        if(otaInProgress == false) {
            if (length > UART_TX_FIFO_SIZE)
            {   /* FOr OTA Req. MSP commands are getting corrupted, so special handling is added
                   for sending OTA Init Request MQTT message to STM 
                   a. First Init Request with no payload is send to STM. STM then switches to 
                      OTA Usage Mode and stops all data streaming etc, to reduce MSP message traffic  
                   b. After some delay Actual MQTT message with OTA req. is sent to STM, to 
                      start OTA process.  
                      Copying this payload to other buffer and sending in another context
                      doesnt work, so payload is sent with some delay   */
                bool otaReqReceived = false;
                char *Ptr = NULL;
                char *cert = NULL;
       
                Ptr = strstr(payload,"fwBinaries");
                
                if(Ptr != NULL ) {
                    Ptr = strstr(payload,"initiateota");
                    if(Ptr != NULL )
                        otaReqReceived = true;
                }
                if(otaReqReceived)
                {     
                    otaInProgress = true;
                    hostSerial.sendMSPCommand(MSPCommand::OTA_INIT_REQUEST);
                    delay(1500);
                    hostSerial.sendLongMSPCommand(
                    MSPCommand::CONFIG_FORWARD_CONFIG, 3000000, payload, length);
                    otaInitTimeout = millis();
                    otaInitTimeoutFlag = true;
                }                
                else
                {
                        // Command is longer than TX buffer, send it while taking care to not
                    // overflow the buffer. Timeout parameter is in microseconds.
                    hostSerial.sendLongMSPCommand(
                        MSPCommand::CONFIG_FORWARD_CONFIG, 3000000, payload, length);
                }
            }
            else
            {
                hostSerial.sendMSPCommand(MSPCommand::CONFIG_FORWARD_CONFIG, payload,
                                        length);
            }
        }
    }
    else if (strncmp(&subTopic[1], "command", 7) == 0)
    {
        if(otaInProgress == false) {
                
            hostSerial.sendMSPCommand(MSPCommand::CONFIG_FORWARD_CMD,
                                    payload, length);
        }
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
            if(otaInProgress == false) {
                hostSerial.sendMSPCommand(MSPCommand::CONFIG_FORWARD_LEGACY_CMD,
                                        payload, length);
            }
        }
    }
    else if (strncmp(&subTopic[1], "post_url", 8) == 0)
    {
        if(otaInProgress == false) {
            //TODO Improve url management
            if (length < MAX_HOST_LENGTH)
            {
                strncpy(m_featurePostHost, payload, length);
                strncpy(m_diagnosticPostHost, payload, length);
            }
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
    if (WiFi.isConnected() /*&& mqttHelper.client.connected()*/)
    {
        hostSerial.sendMSPCommand(MSPCommand::WIFI_ALERT_CONNECTED);
        //Serial.println("\nWIFI CONNECTED");
    }
    else
    {
        hostSerial.sendMSPCommand(MSPCommand::WIFI_ALERT_DISCONNECTED);
        //Serial.println("WIFI DISCONECTED");
    }
}
/**
 * @brief Send a message to host MCU to inform about the MQTT connection status
 * 
 */
void Conductor::updateMQTTStatus()
{
    if (mqttHelper.client.connected())
    {
        hostSerial.sendMSPCommand(MSPCommand::MQTT_ALERT_CONNECTED);
        //Serial.println("MQTT CONNECTED");
    }
    else
    {
        hostSerial.sendMSPCommand(MSPCommand::MQTT_ALERT_DISCONNECTED);
        //Serial.println("MQTT DISCONNECTED");
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
        updateMQTTStatus();
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
        connectToWiFi();
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
    strcat(destination, "\",\"wifi-firmware-version\":\"");
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
 //       otaInProgress = true;
        if(WiFi.status() == WL_CONNECTED) 
        {
            http_ota.end(); //  in case of error... need to close handle as it defined global
            http_ota.setTimeout(otaHttpTimeout);
            http_ota.setConnectTimeout(otaHttpTimeout);
           // hostSerial.sendLongMSPCommand(MSPCommand::ESP_DEBUG_TO_STM_HOST,300000,ota_uri,512);
            totlen = 0;
            contentLen = 0;
            otaStsDataSent = false;
            uint8_t http_init_retry = 0;
            while(http_init_retry < MAX_HTTP_INIT_RETRY) 
            {
                if(http_ota.begin(ota_uri)) 
                {
                    http_init_retry = MAX_HTTP_INIT_RETRY;
                    int httpCode = http_ota.GET();
                    // httpCode will be negative on error
                    if(httpCode > 0)
                    {
                        // file found at server
                        if(httpCode == HTTP_CODE_OK)
                        {
                            contentLen = http_ota.getSize();
                            if(!strcmp(ota_uri,otaStm_uri))
                            {
                                if(contentLen == 0 || contentLen > MAX_MAIN_FW_SIZE)
                                {
                                    hostSerial.sendMSPCommand(MSPCommand::OTA_DNLD_FAIL,String(getRca(OTA_INVALID_MAIN_FW_SIZE)).c_str());
                                    waitingForPktAck = false;
                                    http_ota.end();
                                    return false;                           
                                }
                            }
                            else if(!strcmp(ota_uri,otaEsp_uri))
                            {
                                if(contentLen == 0 || contentLen > MAX_WIFI_FW_SIZE)
                                {
                                    hostSerial.sendMSPCommand(MSPCommand::OTA_DNLD_FAIL,String(getRca(OTA_INVALID_WIFI_FW_SIZE)).c_str());
                                    waitingForPktAck = false;
                                    http_ota.end();
                                    return false;                             
                                }                            
                            }
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
                                hostSerial.sendMSPCommand(MSPCommand::OTA_DNLD_FAIL,String(getRca(OTA_DATA_READ_TIMOUT)).c_str());//"DATA_READ_TIMOUT");
                                waitingForPktAck = false;
                            }
                            delay(1);
                        }
                        else
                        {
                        // sprintf(TestStr,"HTTP FAIL:%d",httpCode);
                            hostSerial.sendMSPCommand(MSPCommand::OTA_DNLD_FAIL,String(getRca(httpCode)).c_str());
                            waitingForPktAck = false;
                            http_ota.end();
                        }
                    } 
                    else
                    {            
                        hostSerial.sendMSPCommand(MSPCommand::OTA_DNLD_FAIL,String(getRca(httpCode)).c_str());
                        waitingForPktAck = false;
                        http_ota.end();                    
                    }
                }
                else
                {               
                    http_init_retry++;
                    waitingForPktAck = false;
                    http_ota.end();
                    delay(2000);
                    if(http_init_retry == MAX_HTTP_INIT_RETRY)
                    { 
                        hostSerial.sendMSPCommand(MSPCommand::OTA_DNLD_FAIL,String(getRca(OTA_HTTP_INIT_FAIL)).c_str());//"HTTP_INIT_FAIL");
                        delay(1);
                        return false;
                    }      
                }
            } //  while(http_init == false && http_init_retry < MAX_HTTP_INIT_RETRY)
        }
        else
        {
            // FW Download was initiated... but failed due to WiFi disconnect
            hostSerial.sendMSPCommand(MSPCommand::OTA_DNLD_FAIL,String(getRca(OTA_WIFI_DISCONNECT)).c_str());//"WIFI_DISCONNECT");
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
                uint8_t otaMod10Per = ((100 - (((fwdnldLen-contentLen)*100)/fwdnldLen))%10);
                //if((((100 - (((fwdnldLen-contentLen)*100)/fwdnldLen))%10) == 0) && (otaStsDataSent == false))
                if(otaMod10Per == 0 && otaStsDataSent == false) 
                { /* Send OTA Status on every 10 % completion on MQTT Diag.(iu_err) topic */     
                    char percent[24];
                    otaStsDataSent = true;
                    memset(percent,0x00, 24);
                    if(!strcmp(ota_uri,otaStm_uri))
                    {
                        sprintf(percent,"Main FW DNLD: %d %% ",(((((fwdnldLen-contentLen)*100)/fwdnldLen))));
                    }
                    else if(!strcmp(ota_uri,otaEsp_uri))
                    {
                        sprintf(percent,"WiFi FW DNLD: %d %% ",(((((fwdnldLen-contentLen)*100)/fwdnldLen))));
                    }
                    //hostSerial.sendMSPCommand(MSPCommand::ESP_DEBUG_TO_STM_HOST,percent,20);
                    //sprintf(percent,"Main_FW:%03d %%",(((((fwdnldLen-contentLen)*100)/fwdnldLen))));
                    percent[20] = '\0';
                    char otaSts[128];
                    snprintf(otaSts, 128, "{\"deviceId\":\"%s\",\"type\":\"%s\",\"status\":\"%s\"}",m_bleMAC.toString().c_str(), "vEdge",percent);
                    mqttHelper.publish(OTA_P_TOPIC,otaSts);
                    delay(1);
                }
                else if(otaMod10Per != 0 && otaStsDataSent == true)
                {
                    otaStsDataSent = false;
                }

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
                    hostSerial.sendMSPCommand(MSPCommand::OTA_DNLD_FAIL,String(getRca(OTA_DATA_READ_TIMOUT)).c_str());//"DATA_READ_TIMOUT");
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
                        delay(5);
                        char percent[24];
                        char otaSts[128];
                        strcpy(percent,"Main FW DNLD: 100 %");
                        snprintf(otaSts, 128, "{\"deviceId\":\"%s\",\"type\":\"%s\",\"status\":\"%s\"}",m_bleMAC.toString().c_str(), "vEdge",percent);
                        mqttHelper.publish(OTA_P_TOPIC,otaSts);
                        fwdnldLen = 0;
                        totlen = 0;
                        waitingForPktAck = false;
                        http_ota.end();
                    }
                    if(strcmp(ota_uri,otaEsp_uri) == 0)
                    {
                        hostSerial.sendMSPCommand(MSPCommand::OTA_ESP_DNLD_STATUS);
                        delay(5);
                        char percent[24];
                        char otaSts[128];
                        strcpy(percent,"WiFi FW DNLD: 100 %");
                        snprintf(otaSts, 128, "{\"deviceId\":\"%s\",\"type\":\"%s\",\"status\":\"%s\"}",m_bleMAC.toString().c_str(), "vEdge",percent);
                        mqttHelper.publish(OTA_P_TOPIC,otaSts);
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
                hostSerial.sendMSPCommand(MSPCommand::OTA_DNLD_FAIL,String(getRca(OTA_WIFI_DISCONNECT)).c_str());//"WIFI_DISCONNECT");
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
String Conductor::getRca(int error)
{   
    switch(error) {
        case OTA_WIFI_DISCONNECT:
            /* WiFi Disconnection error handled both at STM and ESP, with same RCA Code = 3  */
            if(certificateDownloadInProgress){ return F("CERT-RCA-0003");}else{ return F("OTA-RCA-0003"); }
        case OTA_HTTP_INIT_FAIL:
            return F("OTA-RCA-0011");
        case OTA_DATA_READ_TIMOUT:
            return F("OTA-RCA-0012");
        case OTA_STM_PKT_ACK_TMOUT:
            return F("OTA-RCA-0013");
        case HTTPC_ERROR_CONNECTION_REFUSED:
            if(certificateDownloadInProgress){ return F("CERT-RCA-0014");}else{ return F("OTA-RCA-0014"); }
        case HTTPC_ERROR_SEND_PAYLOAD_FAILED:
            if(certificateDownloadInProgress){ return F("CERT-RCA-0015");}else{ return F("OTA-RCA-0015"); }
        case HTTPC_ERROR_NOT_CONNECTED:
            if(certificateDownloadInProgress){ return F("CERT-RCA-0016");}else{ return F("OTA-RCA-0016"); }
        case HTTPC_ERROR_CONNECTION_LOST:
            if(certificateDownloadInProgress){ return F("CERT-RCA-0017");}else{ return F("OTA-RCA-0017"); }
        case HTTPC_ERROR_NO_STREAM:
            if(certificateDownloadInProgress){ return F("CERT-RCA-0018");}else{ return F("OTA-RCA-0018"); }
        case HTTPC_ERROR_NO_HTTP_SERVER:
            if(certificateDownloadInProgress){ return F("CERT-RCA-0019");}else{ return F("OTA-RCA-0019"); }
        case HTTPC_ERROR_TOO_LESS_RAM:
             if(certificateDownloadInProgress){ return F("CERT-RCA-0020");}else{ return F("OTA-RCA-0020"); }
        case HTTPC_ERROR_ENCODING:
             if(certificateDownloadInProgress){ return F("CERT-RCA-0021");}else{ return F("OTA-RCA-0021"); }
        case HTTPC_ERROR_STREAM_WRITE:
             if(certificateDownloadInProgress){ return F("CERT-RCA-0022");}else{ return F("OTA-RCA-0022"); }
        case HTTPC_ERROR_READ_TIMEOUT:
            if(certificateDownloadInProgress){ return F("CERT-RCA-0023");}else{ return F("OTA-RCA-0023"); }
        case HTTP_CODE_BAD_REQUEST:
            if(certificateDownloadInProgress){ return F("CERT-RCA-0024");}else{ return F("OTA-RCA-0024"); }
        case HTTP_CODE_FORBIDDEN:
            if(certificateDownloadInProgress){ return F("CERT-RCA-0025");}else{ return F("OTA-RCA-0025"); }
        case HTTP_CODE_NOT_FOUND:
            if(certificateDownloadInProgress){ return F("CERT-RCA-0026");}else{ return F("OTA-RCA-0026"); }
        case HTTP_CODE_METHOD_NOT_ALLOWED:
            if(certificateDownloadInProgress){ return F("CERT-RCA-0027");}else{ return F("OTA-RCA-0027"); }
        case HTTP_CODE_NOT_ACCEPTABLE:
            if(certificateDownloadInProgress){ return F("CERT-RCA-0028");}else{ return F("OTA-RCA-0028"); }
        case HTTP_CODE_PROXY_AUTHENTICATION_REQUIRED:
            if(certificateDownloadInProgress){ return F("CERT-RCA-0029");}else{ return F("OTA-RCA-0029"); }
        case HTTP_CODE_REQUEST_TIMEOUT:
            if(certificateDownloadInProgress){ return F("CERT-RCA-0030");}else{ return F("OTA-RCA-0030"); }
        case HTTP_CODE_INTERNAL_SERVER_ERROR:
            if(certificateDownloadInProgress){ return F("CERT-RCA-0031");}else{ return F("OTA-RCA-0031"); }
        case HTTP_CODE_NOT_IMPLEMENTED:
            if(certificateDownloadInProgress){ return F("CERT-RCA-0032");}else{ return F("OTA-RCA-0032"); }
        case HTTP_CODE_BAD_GATEWAY:
            if(certificateDownloadInProgress){ return F("CERT-RCA-0033");}else{ return F("OTA-RCA-0033"); }
        case HTTP_CODE_SERVICE_UNAVAILABLE:
            if(certificateDownloadInProgress){ return F("CERT-RCA-0034");}else{ return F("OTA-RCA-0034"); }
        case HTTP_CODE_GATEWAY_TIMEOUT:
            if(certificateDownloadInProgress){ return F("CERT-RCA-0035");}else{ return F("OTA-RCA-0035"); }
        case HTTP_CODE_HTTP_VERSION_NOT_SUPPORTED:
            if(certificateDownloadInProgress){ return F("CERT-RCA-0036");}else{ return F("OTA-RCA-0036"); }
        case HTTP_CODE_VARIANT_ALSO_NEGOTIATES:
            if(certificateDownloadInProgress){ return F("CERT-RCA-0037");}else{ return F("OTA-RCA-0037"); }
        case HTTP_CODE_INSUFFICIENT_STORAGE:
            if(certificateDownloadInProgress){ return F("CERT-RCA-0038");}else{ return F("OTA-RCA-0038"); }
        case HTTP_CODE_LOOP_DETECTED:
            if(certificateDownloadInProgress){ return F("CERT-RCA-0039");}else{ return F("OTA-RCA-0039"); }
        case HTTP_CODE_NOT_EXTENDED:
            if(certificateDownloadInProgress){ return F("CERT-RCA-0040");}else{ return F("OTA-RCA-0040"); }
        case HTTP_CODE_NETWORK_AUTHENTICATION_REQUIRED:
            if(certificateDownloadInProgress){ return F("CERT-RCA-0041");}else{ return F("OTA-RCA-0041"); }
        case OTA_INVALID_MAIN_FW_SIZE:
            return F("OTA-RCA-0042");
        case OTA_INVALID_WIFI_FW_SIZE:
            return F("OTA-RCA-0043");
        case CERT_DOWNLOAD_COMPLETE:
        case CERT_UPGRADE_COMPLETE:
            return F("CERT-RCA-0000");
        case CERT_DOWNLOAD_FAILED:
            return F("CERT-RCA-0001");
        case CERT_DOWNLOAD_KEY_FAILED:
            return F("CERT-RCA-0002");
        case CERT_DOWNLOAD_CRT_FAILED:
            return F("CERT-RCA-0003");
        case CERT_DOWNLOAD_ROOTCA_FAILED:
            return F("CERT-RCA-0004");
        case CERT_INVALID_CRT_CHECKSUM:
            return F("CERT-RCA-0005");
        case CERT_INVALID_KEY_CHECKSUM:
            return F("CERT-RCA-0006");
        case CERT_INVALID_ROOTCA_CHECKSUM:
            return F("CERT-RCA-0007");
        case CERT_FILES_NOT_PRESENT:
            return F("CERT-RCA-0008");
        case CERT_INVALID_CONFIG_JSON:
            return F("CERT-RCA-0009");
        case CERT_FILESYSTEM_READ_WRITE_FAILED:
            return F("CERT-RCA-0010");
        case CERT_UPGRADE_START:
            return F("CERT-RCA-0011");
        case MQTT_CONNECTION_ATTEMPT_FAILED: 
            return F("CERT-RCA-0012");  
        case CERT_UPGRADE_FAILED:
            return F("CERT-RCA-0013");
        case CERT_NEW_CERT_NOT_AVAILBLE:
            return F("CERT-RCA-0044"); 
        case CERT_SAME_UPGRADE_CONFIG_RECEIVED:
            return F("CERT-RCA-0045"); 
        case CERT_STATIC_URL_FILE_NOT_PRESENT:
            return F("CERT-RCA-0046");
        case HTTP_CODE_UNAUTHORIZED:
            return F("CERT-RCA-0047");
        default:
            if(certificateDownloadInProgress){ return F("CERT-RCA-2222");}else{ return F("OTA-RCA-1111"); }
    }
}

void Conductor::mqttSecureConnect(){
   
    //static uint8_t newDownloadConnectonAttempt;
    uint32_t now = millis();
    if(!mqttHelper.client.connected() && (now - m_disconnectionMqttTimerStart > 3000 ) && WiFi.isConnected() || upgradeReceived == true){
          if( certificateDownloadInProgress == false && certDownloadInitAck == false && downloadInitTimer == true ){
             if (!mqttHelper.hasConnectionInformations())
             {  
                 hostSerial.sendMSPCommand(MSPCommand::GET_MQTT_CONNECTION_INFO);    //Need to Change the call of this
                 return;
             }else
             {
                 //try connecting with available credentials
                 if (iuWiFiFlash.isFilePresent(IUESPFlash::CFG_MQTT_CLIENT0) && iuWiFiFlash.isFilePresent(IUESPFlash::CFG_MQTT_KEY0) ||
                     iuWiFiFlash.isFilePresent(IUESPFlash::CFG_MQTT_CLIENT1) && iuWiFiFlash.isFilePresent(IUESPFlash::CFG_MQTT_KEY1) )
                 {
                   //Serial.println("ESP32 DEBUG : APPYING THE CERTIFICATES*********");
                   hostSerial.sendMSPCommand(MSPCommand::ESP_DEBUG_TO_STM_HOST,"ESP32 DEBUG : Applying MQTT-TLS Certificates");
                   int certSize,keySize;
                   if( upgradeReceived  && (activeCertificates == 1 && initialFileDownload == false) || activeCertificates == 1 ){
                        //Serial.println("Using Client 1 Certificates");
                        hostSerial.sendMSPCommand(MSPCommand::ESP_DEBUG_TO_STM_HOST,"ESP32 DEBUG : Applying Client1 Certificates");
                        certSize = iuWiFiFlash.getFileSize(IUESPFlash::CFG_MQTT_CLIENT1);
                        keySize  = iuWiFiFlash.getFileSize(IUESPFlash::CFG_MQTT_KEY1);
                        iuWiFiFlash.readFile(IUESPFlash::CFG_MQTT_CLIENT1,mqtt_client_cert,certSize);
                        delay(1);
                        iuWiFiFlash.readFile(IUESPFlash::CFG_MQTT_KEY1,mqtt_client_key,keySize);
                   }else{
                        //Serial.println("Using Client 0 Certificates");
                        hostSerial.sendMSPCommand(MSPCommand::ESP_DEBUG_TO_STM_HOST,"ESP32 DEBUG : Applying Client0 Certificates");
                        certSize = iuWiFiFlash.getFileSize(IUESPFlash::CFG_MQTT_CLIENT0);
                        keySize  = iuWiFiFlash.getFileSize(IUESPFlash::CFG_MQTT_KEY0);
                        iuWiFiFlash.readFile(IUESPFlash::CFG_MQTT_CLIENT0,mqtt_client_cert,certSize);
                        delay(1);
                        iuWiFiFlash.readFile(IUESPFlash::CFG_MQTT_KEY0,mqtt_client_key,keySize);
                   }
                   
                    if ((certificateDownloadStatus == 1 && ( mqttHelper.mqttConnected == 0 && !mqttHelper.client.connected())) || upgradeReceived == true )
                    {
                        // first connection attempt,upgrade Init
                        hostSerial.sendMSPCommand(MSPCommand::ESP_DEBUG_TO_STM_HOST,"ESP32 DEBUG : Certificate Upgrade Init");
                    }
                    // Retry 3 time connection then download new certificates
                    mqttHelper.reconnect();  
                    char reconnectMessage[45];
                    sprintf(reconnectMessage,"ESP32 DEBUG : MQTT Reconnect Attemp :%d",mqttHelper.mqttConnected);
                    hostSerial.sendMSPCommand(MSPCommand::ESP_DEBUG_TO_STM_HOST,reconnectMessage);
                    upgradeSuccess();
                    upgradeFailed();
                    //Retry attempt overflow
                    if (mqttHelper.mqttConnected >= maxMqttClientConnectionCount )
                    {   
                        newDownloadConnectonAttempt++;
                        mqttHelper.mqttConnected = 0;
                        // Serial.print("\nESP32 DEBUG : ClientConnnection Attempt :");
                        // Serial.println(newDownloadConnectonAttempt);
                        // re-initiate the certifiates download process
                        if (newDownloadConnectonAttempt > maxMqttCertificateDownloadCount)
                        {
                            // TODO : All Retry failed , might be http link broken cannot download certs
                            // Send status to http diagnostic endpoint and show Visuals
                            //Serial.println("\nESP32 DEBUG : ALL MQTT Connection Attemps FAILED ************");
                            hostSerial.sendMSPCommand(MSPCommand::ALL_MQTT_CONNECT_ATTEMPT_FAILED,String(getRca(MQTT_CONNECTION_ATTEMPT_FAILED)).c_str());
                            downloadInitLastTimeSync = millis();
                            upgradeReceived = false;
                            downloadInitTimer  = false;
                            
                        }else {
                            //Serial.println("\nESP32 DEBUG : Download Init trigger.......");
                            upgradeReceived = false;
                            downloadInitTimer = false;
                            hostSerial.sendMSPCommand(MSPCommand::CERT_DOWNLOAD_INIT,CERT_DOWNLOAD_DEFAULT_MESSAGEID);
                        }    
                    }
                    
                 }else
                 {
                     // TODO : .crt and .key files not Present
                     // Show Visuals
                     //Serial.println("\nESP32 DEBUG : Files not available to Initiate Certificates Download");
                     hostSerial.sendMSPCommand(MSPCommand::ESP32_FLASH_FILE_READ_FAILED);
                     delay(10);
                     upgradeReceived = false;
                     downloadInitTimer = false;
                     initialFileDownload = true;
                     activeCertificates = iuWiFiFlash.updateValue(ADDRESS,1);   // Update for first attempt only
                     hostSerial.sendMSPCommand(MSPCommand::CERT_DOWNLOAD_INIT,CERT_DOWNLOAD_DEFAULT_MESSAGEID);
                }
                 
                 
             }
          }else
          { 
              // Certificate Download In-progress or all certificates download and connection attempt failed
            //  if (newDownloadConnectonAttempt >= maxMqttCertificateDownloadCount)
            //  {  Serial.println("\nESP32 DEBUG : CERT DOWNLOAD ALL Attempt failed, re-downloading the certificates");
            //      hostSerial.sendMSPCommand(MSPCommand::ALL_MQTT_CONNECT_ATTEMPT_FAILED,String(getRca(MQTT_CONNECTION_ATTEMPT_FAILED)).c_str());
            //  }
             
          }
           
        }
        
        
}

void Conductor::upgradeSuccess(){
    if ((certificateDownloadStatus == 1 && ( mqttHelper.mqttConnected <= 5 && mqttHelper.client.connected())) && upgradeReceived ==true)
    {
        // Rollback downloadCertificates
        if(upgradeReceived && activeCertificates == 1){
            //Serial.println("Client 1 Upgrade Success....");
            // backup the older  certificates and use the latest.
            // raname the files or overwrite it. make sure after devicereset it should use new certs
            activeCertificates = iuWiFiFlash.updateValue(ADDRESS,1);  // client1 in Use after Reset
            hostSerial.sendMSPCommand(MSPCommand::ESP_DEBUG_TO_STM_HOST,"ESP32 DEBUG : Upgrade Success,Updated Flag with Value : 1");
        }else {
            //Serial.println("\nClient 0 Upgrade Success.....");
            activeCertificates = iuWiFiFlash.updateValue(ADDRESS,0); // Clinet 0 in use 
            hostSerial.sendMSPCommand(MSPCommand::ESP_DEBUG_TO_STM_HOST,"ESP32 DEBUG : Upgrade Success,Updated Flag with Value : 0");
        }
        // Send Upgrade Status 
        hostSerial.sendMSPCommand(MSPCommand::CERT_UPGRADE_SUCCESS,String(getRca(CERT_UPGRADE_COMPLETE)).c_str());
        upgradeReceived = false;
        certificateDownloadStatus = 0;
    }
}

void Conductor::upgradeFailed(){
    if ((certificateDownloadStatus == 1 && ( mqttHelper.mqttConnected >= 5 && !mqttHelper.client.connected())) && upgradeReceived == true)
    {
        // Rollback downloadCertificates
        if(activeCertificates == 1){
            //Serial.println("Client 1 Upgrade Failed Use previous Client 0 Certificates....");
            // backup the older  certificates and use the latest.
            activeCertificates = iuWiFiFlash.updateValue(ADDRESS,0);  // client1 in Use after Reset
            hostSerial.sendMSPCommand(MSPCommand::ESP_DEBUG_TO_STM_HOST,"ESP32 DEBUG : Upgrade Failed,Updated Flag with Value : 0");
        }else {
            //Serial.println("\nClient 0 Upgrade Failed Use previous Client1 Certificates.....");
            activeCertificates = iuWiFiFlash.updateValue(ADDRESS,1); // Clinet 0 in use 
            hostSerial.sendMSPCommand(MSPCommand::ESP_DEBUG_TO_STM_HOST,"ESP32 DEBUG : Upgrade Failed,Updated Flag with Value : 1");
        }
        // Send Upgrade Status 
        hostSerial.sendMSPCommand(MSPCommand::CERT_UPGRADE_ABORT,String(getRca(CERT_UPGRADE_FAILED)).c_str());
        upgradeReceived = false;
        certificateDownloadStatus = 0;
    }
}
// Certificate managment selection
//String filepath = "/eap_client.crt"; // bin file name with a slash in front.
/**
 * @brief 
 * 
 * @param fs 
 * @param FileSize 
 * @param path 
 * @return true 
 * @return false 
 */
bool Conductor:: writeCertificatesToFlash(IUESPFlash::storedConfig configType,long fileSize,const char* type)
{
    uint8_t buf[512];
    int totalblock = fileSize/512;
    size_t count = 0;
    int loopCnt = 0;
    size_t ReadBlock = 512;
    long len = fileSize;
    unsigned long time1 = 0;
    unsigned long time2 = 0;   

    char filePath[iuWiFiFlash.MAX_FULL_CONFIG_FPATH_LEN];
    iuWiFiFlash.getConfigFilename(configType,filePath,iuWiFiFlash.MAX_FULL_CONFIG_FPATH_LEN);

    if(fileSize%512 != 0)
      totalblock =  totalblock + 1;

    File file = iuWiFiFlash.openFile(configType,"w");
    if(!file){
        hostSerial.sendMSPCommand(MSPCommand::CERT_DOWNLOAD_ABORT,String(getRca(CERT_FILESYSTEM_READ_WRITE_FAILED)).c_str());
        downloadAborted = true;
        upgradeReceived = false;
        return false;
    }
    WiFiClient * stream = http_ota.getStreamPtr();
    while(1)
    {
      delay(5);
      count = stream->readBytes(buf,ReadBlock);
      if(count != 0) 
      {
        loopCnt++;
        file.write(buf, count);
        if(count < 512) {
          if(loopCnt == totalblock)
          {            
            break; // on last packet          
          }
          else
          {
            //sprintf(TestStr,"SPIFFS RD_Fail1:%s",filePath);
            //hostSerial.sendMSPCommand(MSPCommand::ESP_DEBUG_TO_STM_HOST,TestStr,32);
            return false;
          }          
        }
        if(len < 512)
          ReadBlock = len;
        else
          len = len - count;
      }
      else
      {
         // TODO : Empty File Size, No Stream available, update the Status message 
        file.close();
        hostSerial.sendMSPCommand(MSPCommand::CERT_DOWNLOAD_ABORT,String(getRca(HTTPC_ERROR_NO_STREAM)).c_str());
        downloadAborted =true;
        upgradeReceived = false;
        return false;
      }
      //file.close();
    }

    file.close();
    return true;
}

/**
 * @brief 
 * 
 * @param fs 
 * @param fwpath 
 */
void Conductor:: readCertificatesFromFlash(IUESPFlash::storedConfig configType,const char* type)
{
    char TestStr[64];
    static uint8_t buf[512];
    int totalblock = 0;
    size_t count = 0;
    size_t ReadBlock = 512;
    int loopCnt = 0;
    long len = 0;
    File file = iuWiFiFlash.openFile(configType,"r");

    char filePath[iuWiFiFlash.MAX_FULL_CONFIG_FPATH_LEN];
    iuWiFiFlash.getConfigFilename(configType,filePath,iuWiFiFlash.MAX_FULL_CONFIG_FPATH_LEN);

    if(!file){
        // sprintf(TestStr,"SPIFFS RD_Fail:%s",filePath);
        // hostSerial.sendMSPCommand(MSPCommand::ESP_DEBUG_TO_STM_HOST,TestStr,32);
        hostSerial.sendMSPCommand(MSPCommand::CERT_UPGRADE_ABORT,String(getRca(CERT_FILESYSTEM_READ_WRITE_FAILED)).c_str());
        return ;
    }

    if(file && !file.isDirectory()){
        len = file.size();
        totalblock = len/512;
        if(len % 512 != 0)
          totalblock = totalblock + 1;
        size_t flen = len;
        while(len){
            size_t toRead = len;
            if(toRead > 512){
                toRead = 512;
            }
            loopCnt++;
            file.read(buf, toRead);
            if(/*!(strcmp(filePath,"/iuConfig/certs/client0.crt")) && */!strcmp(type,"MQTT-TLS-CERT"))
            {
                strncpy(&mqtt_client_cert[(loopCnt-1)*512],(const char *)buf,count);    // Update the buffer
                //hostSerial.sendLongMSPCommand(MSPCommand::ESP_DEBUG_TO_STM_HOST,3000000,mqtt_client_cert,sizeof(mqtt_client_cert));
            }
            else if(!strcmp(type,"MQTT-TLS-KEY"))
            {
                strncpy(&mqtt_client_key[(loopCnt-1)*512],(const char *)buf,count);
                //hostSerial.sendLongMSPCommand(MSPCommand::ESP_DEBUG_TO_STM_HOST,3000000,mqtt_client_key,sizeof(mqtt_client_key));
            }else if(!strcmp(type,"SSL"))
            {
                strncpy(&ssl_rootca_cert[(loopCnt-1)*512],(const char *)buf,count);    // Update the buffer
                //hostSerial.sendLongMSPCommand(MSPCommand::ESP_DEBUG_TO_STM_HOST,3000000,ssl_rootca_cert,sizeof(ssl_rootca_cert));
            }
            len -= toRead;
        }
        file.close();
    } else {
            // TODO : Not define ????
            file.close();
    }
    file.close();
}

/**
 * @brief 
 * 
 * @param tls_uri 
 * @param fileType 
 * @return true 
 * @return false 
 */
bool Conductor::getDeviceCertificates(IUESPFlash::storedConfig configType, const char* type,const char* url)
{
    if(WiFi.status() == WL_CONNECTED) 
    {
        http_ota.end(); //  in case of error... need to close handle as it defined global
        http_ota.setTimeout(otaHttpTimeout);
        http_ota.setConnectTimeout(otaHttpTimeout);
        totlen = 0;
        contentLen = 0;
        otaStsDataSent = false;
        uint8_t http_init_retry = 0;
        while(http_init_retry < MAX_HTTP_INIT_RETRY) 
        {
            if(http_ota.begin(url)) 
            {
                http_init_retry = MAX_HTTP_INIT_RETRY;
                int httpCode = http_ota.GET();
                // httpCode will be negative on error
                if(httpCode > 0)
                {
                    // file found at server
                    if(httpCode == HTTP_CODE_OK)
                    {
                        contentLen = http_ota.getSize();
                        if(contentLen == 0 || contentLen > MAX_MAIN_FW_SIZE)
                        {
                            // TODO : Invalid File size , file is empty or greater than available memory
                            // TODO : Not applicable for certificates 
                            http_ota.end();
                            return false;                           
                        }
                        //fwdnldLen = contentLen;
                        //Write the content to files
                        //Serial.print("Content Len : ");Serial.println(contentLen);
                        writeCertificatesToFlash(configType,contentLen,type);
                    }
                    else
                    {
                        // sprintf(TestStr,"HTTP FAIL:%d",httpCode);
                        //Serial.println("ESP32 DEBUG : httpCode != 200 OK");
                        hostSerial.sendMSPCommand(MSPCommand::CERT_DOWNLOAD_ABORT,String(getRca(httpCode)).c_str());
                        downloadAborted = true;
                        upgradeReceived = false;
                        http_ota.end();
                    }
                } 
                else
                {   
                    // TODO : Handle http error code
                    //Serial.println("ESP32 DEBUG : httpCode < 0 1 --->");Serial.println(httpCode);
                    hostSerial.sendMSPCommand(MSPCommand::CERT_DOWNLOAD_ABORT,String(getRca(httpCode)).c_str());         
                    http_ota.end();
                    downloadAborted =true;
                    upgradeReceived = false;
                }
            }
            else
            {               
                http_init_retry++;
                waitingForPktAck = false;
                http_ota.end();
                delay(2000);
                //Serial.println("ESP32 DEBUG : HTTP URL BEGIN Failed");
                if(http_init_retry == MAX_HTTP_INIT_RETRY)
                { 
                    delay(1);
                    return false;
                }      
            }
        } //  while(http_init == false && http_init_retry < MAX_HTTP_INIT_RETRY)
    }
    else
    {
        // FW Download was initiated... but failed due to WiFi disconnect
        //Serial.println("getDeviceCertificates : Wifi Disconnected");
        hostSerial.sendMSPCommand(MSPCommand::CERT_DOWNLOAD_ABORT,String(getRca(OTA_WIFI_DISCONNECT)).c_str());
        http_ota.end();
        downloadAborted = true;
        upgradeReceived = false;
        return false;       
    }
}

/**
 * @brief 
 * 
 * @return int 
 * */  
int Conductor::download_tls_ssl_certificates(){
      // Read the JOSN from the file and start Download
            // Start Certificate Download Mode /OTA
            // Apply the new Certificates and try connecting
            // Update Success , Failed status codes over Diagnostic endpoint
            // Validate the checksum of the files

            // Assume the URL are available in File
            //if(otaInProgress == true) {
                if (iuWiFiFlash.isFilePresent(IUESPFlash::CFG_CERT_UPGRADE_CONFIG))
                {   
                    DynamicJsonBuffer jsonBuffer;
                    JsonObject& root = jsonBuffer.parseObject(certDownloadResponse);
                    JsonVariant config = root;
                    int configTypeCount = config["certificates"].size();
                    bool success = config.success();
                    //Serial.print("Arry Size : ");Serial.println(configTypeCount);
                    // Serial.print("CONFIG JSON :");
                    // config.prettyPrintTo(Serial);
                    uint8_t certToUpdate;
                    if(activeCertificates != 0){ 
                        // if(initialFileDownload){
                        //     Serial.println("Initial File Download , Updated Value ");
                        //     activeCertificates = iuWiFiFlash.updateValue(ADDRESS,0);
                        //     //initialFileDownload = false;
                        // }
                        certToUpdate = 0;
                    }else{
                        certToUpdate = 1;
                    }
                    if (success)
                    {   
                        //Serial.println("Valid JSON Present");
                        for (size_t index = 0; index < configTypeCount; index++)
                        {
                            // Download the certificates and store in to files
                            const char* type = config["certificates"][index]["type"];
                            const char* url = config["certificates"][index]["url"];
                            const char* hash = config["certificates"][index]["hash"];
                            //Serial.println(type);
                            bool downloadSuccess = false;
                            char* checksum ;
                            // Note : upgradeReceived flage is set only for dev testing , can ne removed later (only in this statments not in nested if)
                            int res = downloadCertificates(type,url,hash,index,certToUpdate);
                            //Serial.print("Output : ");Serial.println(res);
                            if(res != 1){  return 0 ; /*break;*/}   //exit from for loop
                            
                        }   // for loop
                        
                    }else
                    {
                        // TODO : send the status message
                        //Serial.println("Invalid JSON message configuration");
                        //hostSerial.sendMSPCommand(MSPCommand::CERT_DOWNLOAD_ABORT,"Invalid JOSN configuration");
                        hostSerial.sendMSPCommand(MSPCommand::CERT_DOWNLOAD_ABORT, String(getRca(CERT_INVALID_CONFIG_JSON)).c_str());
                        downloadAborted = true;
                        upgradeReceived = false;
                        return 0;
                    }
                       
                }else
                {   
                    // TODO : Handle the Error Code here
                    hostSerial.sendMSPCommand(MSPCommand::GET_CERT_COMMOM_URL,".cert and .Key files not Present show Visuals");
                    //Serial.println(" getCert.conf file not Present");
                    delay(1);    
                    //hostSerial.sendMSPCommand(MSPCommand::CERT_DOWNLOAD_ABORT,".cert and .key not Present, ABORTED");
                    hostSerial.sendMSPCommand(MSPCommand::CERT_DOWNLOAD_ABORT, String(getRca(CERT_CONFIG_JSON_FILE_NOT_PRESENT)).c_str());
                    downloadAborted = true;
                    upgradeReceived = false;
                    return 0;    
                }
        return 1;
                
}

/**
 * calculate the checksum of the file
 */
char* Conductor::getConfigChecksum(IUESPFlash::storedConfig configType)
{
    size_t filesize = iuWiFiFlash.getFileSize(configType);
    size_t configMaxLen = filesize;
    char config[configMaxLen];
    strcpy(config, "");
    size_t charCount = 0;
    if (iuWiFiFlash.available()) {
        charCount = iuWiFiFlash.readFile(configType, config, configMaxLen);
    }
    // Charcount = 0 if flash is unavailable or file not found => Checksum of
    // empty string will be sent, and that will trigger a config refresh
    unsigned char* md5hash = MD5::make_hash(config, charCount);
    char *md5str = MD5::make_digest(md5hash, 16);
    //free memory
    free(md5hash);
    return md5str;
}


void Conductor::resetDownloadInitTimer(uint16_t downloadTriggerTime,uint16_t loopTimeout){       
    static int tickCounter;
    downloadTriggerTime = (downloadTriggerTime*1000)/loopTimeout;           // 10*1000/5000     
    
    if (( downloadInitTimer == false && (newDownloadConnectonAttempt > maxMqttCertificateDownloadCount ) ) || !mqttHelper.client.connected()    )
          
    {
        // increment the timer tick
        tickCounter += 1;           // increment by every  5sec 
    
        if (tickCounter >= downloadTriggerTime)
        {
            downloadInitTimer = true;           // Re-Initiate the Certificate Download process 
            newDownloadConnectonAttempt = 0;
            tickCounter = 0;
            if (debugMode)
            {
                debugPrint("ESP32 DEBUG : Timer Overflow, Re-Initiating Certificate Downloading.");
            }
            
        }
        
    }
    
}

void Conductor::updateDiagnosticEndpoint(char* diagnosticEndpoint,int length){
    // Update the file with the latest Diagnostic http/https endpoint
    bool success = iuWiFiFlash.writeFile(IUESPFlash::CFG_DIAGNOSTIC_ENDPOINT,diagnosticEndpoint,length);
    if (success)
    {
        if (debugMode)
        {
           debugPrint("Diagnostic URL :");
           debugPrint(diagnosticEndpoint);
        }
        // Update the URL variable
        StaticJsonBuffer<512> JsonBuffer;
        JsonVariant config = JsonVariant(iuWiFiFlash.loadConfigJson(IUESPFlash::CFG_DIAGNOSTIC_ENDPOINT,JsonBuffer));
        bool validConfig = config.success();
        config.prettyPrintTo(Serial);
        if (validConfig)
        {
            const char* host = config["diagnosticUrl"]["host"].as<char*>();
            int port = config["diagnosticUrl"]["port"].as<int>();
            const char* path = config["diagnosticUrl"]["path"].as<char*>();

            Serial.print("Diagnostic URL :");
            
            strncpy(diagnosticEndpointHost, host, MAX_HOST_LENGTH); 
            strncpy(diagnosticEndpointRoute, path, MAX_ROUTE_LENGTH);
            diagnosticEndpointPort = port;


        }else
        {
            Serial.print("Invalid JOSN Format received for Diagnostic endpoint");
            strncpy(diagnosticEndpointHost, DIAGNOSTIC_DEFAULT_ENDPOINT_HOST, MAX_HOST_LENGTH); 
            strncpy(diagnosticEndpointRoute, DIAGNOSTIC_DEFAULT_ENDPOINT_PATH, MAX_ROUTE_LENGTH);
            diagnosticEndpointPort = DIAGNOSTIC_DEFAULT_ENDPOINT_PORT;
            if(debugMode){
                debugPrint("Using Default Diagnostic endpoint");
            }
        }
        
    }else
    {
       strncpy(diagnosticEndpointHost, DIAGNOSTIC_DEFAULT_ENDPOINT_HOST, MAX_HOST_LENGTH); 
       strncpy(diagnosticEndpointRoute, DIAGNOSTIC_DEFAULT_ENDPOINT_PATH, MAX_ROUTE_LENGTH);
       diagnosticEndpointPort = DIAGNOSTIC_DEFAULT_ENDPOINT_PORT;
       if (debugMode)
        {
            debugPrint("Invalid or Failed to write the configuration message");
            debugPrint("Applied Default Diagnostic Configs");
        }
        
    }
    
}

void Conductor::updateWiFiConfig(char* config,int length){
    // Update the file with the latest Diagnostic http/https endpoint
    bool success = iuWiFiFlash.writeFile(IUESPFlash::CFG_WIFI,config,length);
    if (success)
    {
        if (debugMode)
        {
           debugPrint("Config Json :");
           debugPrint(config);
        }
    }
}

void Conductor::setWiFiConfig(){
    StaticJsonBuffer<512> JsonBuffer;
    JsonObject& config = iuWiFiFlash.loadConfigJson(IUESPFlash::CFG_WIFI,JsonBuffer);
    bool validConfig = config.success();
    // config.prettyPrintTo(Serial);
    if (validConfig)
    {
        const char* tempAuthType = config["auth_type"];
        const char* tempSSID = config["ssid"];
        const char* tempPassword = config["password"];
        const char* tempUsername = config["username"];
        const char* tempStaticIP = config["static"];
        const char* tempGatewayIP = config["gateway"];
        const char* tempSubnetIP = config["subnet"];
        const char* tempdns1 = config["dns1"];
        const char* tempdns2 = config["dns2"];
        forgetWiFiCredentials();
        forgetWiFiStaticConfig();
        if(config.containsKey("auth_type")){strcpy(m_wifiAuthType, tempAuthType); }
        if(config.containsKey("ssid")){strcpy(m_userSSID, tempSSID); }
        if(config.containsKey("password")){strcpy(m_userPassword, tempPassword); }
        if(config.containsKey("username")){strcpy(m_username, tempUsername);}
        if(config.containsKey("static")){m_staticIp.fromString(tempStaticIP); }
        if(config.containsKey("gateway")){m_gateway.fromString(tempGatewayIP); }
        if(config.containsKey("subnet")){m_subnetMask.fromString(tempSubnetIP); }
        if(config.containsKey("dns1")){m_dns1.fromString(tempdns1); }
        if(config.containsKey("dns2")){m_dns2.fromString(tempdns2); }
    }
    connectToWiFi();   
}     
void Conductor::configureDiagnosticEndpointFromFlash(IUESPFlash::storedConfig configType){
        // Update the URL variable
        StaticJsonBuffer<512> JsonBuffer;
        JsonVariant config = JsonVariant(iuWiFiFlash.loadConfigJson(configType,JsonBuffer));
        bool validConfig = config.success();
        // config.prettyPrintTo(Serial);
        if (validConfig)
        {
            const char* host = config["diagnosticUrl"]["host"].as<char*>();
            int port = config["diagnosticUrl"]["port"].as<int>();
            const char* path = config["diagnosticUrl"]["path"].as<char*>();

            //Serial.print("Diagnostic URL Configured from Flash :");
            
            strncpy(diagnosticEndpointHost, host, MAX_HOST_LENGTH); 
            strncpy(diagnosticEndpointRoute, path, MAX_ROUTE_LENGTH);
            diagnosticEndpointPort = port;
            if(debugMode){
                debugPrint("Diagnostic Configs apply successfully from file");
            } 

        }else
        {
            //Serial.print("Invalid JOSN Format received for Diagnostic endpoint, use default");
            strncpy(diagnosticEndpointHost, DIAGNOSTIC_DEFAULT_ENDPOINT_HOST, MAX_HOST_LENGTH); 
            strncpy(diagnosticEndpointRoute, DIAGNOSTIC_DEFAULT_ENDPOINT_PATH, MAX_ROUTE_LENGTH);
            diagnosticEndpointPort = DIAGNOSTIC_DEFAULT_ENDPOINT_PORT;
            if(debugMode){
                debugPrint("Using Default Diagnostic endpoint");
            }
        }
        
}
    


void Conductor::publishedDiagnosticMessage(char* buffer,int bufferLength){

    char message[bufferLength];
    snprintf(message,bufferLength,"%s",buffer);
    String auth = setBasicHTTPAutherization();
    if(mqttHelper.client.connected()){
         mqttHelper.publish(CERT_STATUS_TOPIC,buffer);
     }else
     {
        int status =  httpPostBigRequest(diagnosticEndpointHost,diagnosticEndpointRoute,diagnosticEndpointPort,(uint8_t*) message,
                                            bufferLength,auth, NULL,HttpContentType::textPlain );
        //Serial.print("Diagnostic POST Status  : ");
        //Serial.println(status);
     }
}
/**
 * @brief Published the RSSI 
 * 
 * @param publishedTimeout - Sec
 */
void Conductor::publishRSSI(){
        hostSerial.sendMSPCommand(MSPCommand::GET_ESP_RSSI,String(WiFi.RSSI()).c_str() );
        // Serial.print("Published RSSI");
        // Serial.println(WiFi.RSSI());
}

/**
 * @brief This method takes device mac id as a username without colon and password wiil be reverse of macId
 * 
 * @param macId - bleMacID
 * @return const char* - return the base64 encoded string
 */
String Conductor::setBasicHTTPAutherization(){

    char username[20]; 
    snprintf(username,20,"%s",getBleMAC().toString().c_str());
    removeCharacterFromString(username,':');
    // reverse the string as password
    uint8_t length =strlen(username); 
    char password[length];
    char psw[length];
    strcpy(password,username);
    reverseString(password);
    String auth = base64::encode(String(username) + ":" + String(password));
    
    return auth;
}

void Conductor :: removeCharacterFromString(char* inputString, int charToRemove){
    size_t j,count = strlen(inputString);
    for (size_t i =j= 0; i < count; i++)
        if (inputString[i] != charToRemove)
            inputString[j++] = inputString[i];
        
    inputString[j] = '\0';
    
}
void Conductor :: reverseString(char* username){
    int count = strlen(username);
    int n=count-1;
    for(int i=0;i<(count/2);i++){
       //Using temp to store the char value at index i so 
        //you can swap it in later for char value at index n
        char temp = username[i];    
        username[i] = username[n];
        username[n] = temp;
        n = n-1;

    }
    if (debugMode)
    {
        debugPrint("PASSWORD  : ",false);
        debugPrint(username);
    }
}


void Conductor:: messageValidation(char* json){

     //StaticJsonBuffer<2048> jsonBuffer;
     DynamicJsonBuffer JsonBuffer;
     JsonVariant configJson = JsonVariant(JsonBuffer.parseObject(json));
     bool validJson = configJson.success();
     uint8_t configTypeCount = configJson["certificates"].size();
     const char* messageID = configJson["messageId"];
     // Note : below 4 files are mandatory except EAP
    if(validJson && WiFi.isConnected() ){       // JOSN for Checksum Validation
        //Validate the Response message
        if (iuWiFiFlash.isFilePresent(IUESPFlash::CFG_CERT_UPGRADE_CONFIG) && 
             iuWiFiFlash.isFilePresent(IUESPFlash::CFG_MQTT_CLIENT0) && 
             iuWiFiFlash.isFilePresent(IUESPFlash::CFG_MQTT_KEY0)   &&
             iuWiFiFlash.isFilePresent(IUESPFlash::CFG_HTTPS_ROOTCA0)  || 
             iuWiFiFlash.isFilePresent(IUESPFlash::CFG_EAP_CLIENT0) || 
             iuWiFiFlash.isFilePresent(IUESPFlash::CFG_EAP_KEY0) )
        {   
            // Update Certificates Download messageID
            hostSerial.sendMSPCommand(MSPCommand::SET_CERT_DOWNLOAD_MSGID,messageID);
            uint8_t result[CONFIG_TYPE_COUNT]; // 10 , max use 5
            for (size_t i = 0; i < configTypeCount; i++)
            {
                result[i] =  strcmp(getConfigChecksum(CONFIG_TYPES[i]), (const char* ) configJson["certificates"][i]["hash"] );
                // Serial.println(result[i]);
            }
            
            if (result[0] == 0 && result[1] == 0 && result[2] == 0 && result[3] == 0 && result[4] == 0)
            {   //0,1 - eap cert and key
                //2, 3,4 - mqtt cert,key,ssl ca  
                // same file available no need to write to file and no need to initiate the downloading
                // TODO : Send the status message Similar certificates already available
                // abort downloading
                /* code */
                //Serial.println("Similar Checksum of all the files, Do not initiate downloading.....");
                newEapCertificateAvailable = false;
                newEapPrivateKeyAvailable = false;
                newMqttcertificateAvailable = false;  // false
                newMqttPrivateKeyAvailable = false;
                newRootCACertificateAvailable = false;
                // Abort Download
                //Serial.println("\nABORTED Download and Sending Status message....");
                //Send the Status message
                //TODO : Temp Change 
                if(upgradeReceived){
                       //Serial.println("Ingnoreing the Checksum comparision during dev testing..."); 
                    hostSerial.sendMSPCommand(MSPCommand::ESP_DEBUG_TO_STM_HOST,"ESP32 DEBUG : Ignoring Checksum Comparision for dev. testing");
                       //TODO : Check availabel cert and new certs checksum 
                }else{ 
                 hostSerial.sendMSPCommand(MSPCommand::CERT_DOWNLOAD_ABORT, String(getRca(CERT_SAME_UPGRADE_CONFIG_RECEIVED)).c_str());
                 downloadAborted =true;   
                }
                
            }
            if (result[0] != 0 || result[1] != 0)
            {
                // New EAP Certificate available
                newEapCertificateAvailable = true;
                newEapPrivateKeyAvailable = true;
             }
            if (result[2] != 0 || result[3] != 0)
            {
                // New MQTTS Certificate available for downloading
                newMqttcertificateAvailable = true;
                newMqttPrivateKeyAvailable = true;
            }
            if(result[4] != 0){
                // new SSL rootCA.crt is available for downloading
                newRootCACertificateAvailable = true;
            }
            
        }
        else
        {
            // First time downloading the certificates , when files are not available, ignore the checksum comparision here
            bool configWritten = iuWiFiFlash.writeFile(IUESPFlash::CFG_CERT_UPGRADE_CONFIG,json, sizeof(json) );
            if(!configWritten){
                    // Failed to write to Flash File system 
                    // TODO : Publish status to diagnostic endpoint
                    hostSerial.sendMSPCommand(MSPCommand::ESP32_FLASH_FILE_WRITE_FAILED);
                    //Serial.println("ESP32 DEBUG : ESP32_FLASH_FILE_WRITE_FAILED");
                    delay(1);
                    hostSerial.sendMSPCommand(MSPCommand::CERT_DOWNLOAD_ABORT);
                    downloadAborted = true;
            }else
            {       if(configTypeCount > 3){
                        // EaP Certificates available for download
                        newMqttcertificateAvailable = true;
                        newEapPrivateKeyAvailable = true;
                    }    
                    newMqttcertificateAvailable = true;  
                    newMqttPrivateKeyAvailable  = true;
                    newRootCACertificateAvailable = true;
                    //Serial.println("succefully  written the config JOSN to File First time.");
            }
        }
    }else
    {
        //Serial.println("Received Invalid JSON");
        hostSerial.sendMSPCommand(MSPCommand::CERT_DOWNLOAD_ABORT, String(getRca(CERT_INVALID_CONFIG_JSON)).c_str());
        downloadAborted = true;
    }
    
}

int Conductor::downloadCertificates(const char* type,const char* url,const char* hash,uint8_t index,uint8_t certToUpdate){

    bool downloadSuccess = false;
    char* checksum ;
    //Serial.print("CERT TYPE : ");Serial.println(CERT_TYPES[index]);
    // Individual file download is disabled now , rebuilt it                         
    if (( strcmp(type,CERT_TYPES[index]) == 0 ||  upgradeReceived == true) )
    {
        // Download the rootCA.crt and store
        //Serial.println("\ndownloading initiated.");
        if(upgradeReceived && certToUpdate == 1){
            //Serial.println("Updating Client 1 Files.");
             downloadSuccess = getDeviceCertificates(CONFIG_TYPES[index+5],type,url); // Upgrade client 1
        }else{
            //Serial.println("Updating Client 0 Files.");
            downloadSuccess = getDeviceCertificates(CONFIG_TYPES[index],type,url);  // Upgrade Cleint 0
        }
        if (downloadSuccess)
        {
            // File Download and stored success
            // Read the certificates and print
            //Serial.println("downloadSuccess!!!");
            //readCertificatesFromFlash(IUESPFlash::CFG_HTTPS_ROOTCA0,type);
            //delay(10);
            if(upgradeReceived && certToUpdate == 1){
                checksum = getConfigChecksum(CONFIG_TYPES[index+5]);    //verify client 1
            }else{
               checksum = getConfigChecksum(CONFIG_TYPES[index]);       // verify client0
            }
            if (strcmp(hash,checksum) == 0)
            {
                // TODO : .CERT DOWNLOAD Success
                //Serial.println("VALIDATION Success");
                //hostSerial.sendMSPCommand(MSPCommand::CERT_DOWNLOAD_SUCCESS,".rootCA VALIDATED");
            }else
            {
                // TODO : Invalid Checksum abort Upgrade
                // Serial.print("INVALID CHECKSUM ");
                // Serial.println(checksum);
                hostSerial.sendMSPCommand(MSPCommand::CERT_DOWNLOAD_ABORT, String(getRca(CERT_INVALID_CRT_CHECKSUM)).c_str());
                downloadAborted = true;
                upgradeReceived = false;
                return 0;
            }
           
        }else
        {   
            // TODO :
            //Serial.println("File Download Failed");
            hostSerial.sendMSPCommand(MSPCommand::CERT_DOWNLOAD_ABORT, String(getRca(CERT_DOWNLOAD_FAILED)).c_str());
            downloadAborted = true;
            upgradeReceived = false;
            return 0;
        }
    }
    return 1;
}


bool Conductor:: setCommonHttpEndpoint(){

    bool configAvailable = iuWiFiFlash.isFilePresent(IUESPFlash::CFG_STATIC_CERT_ENDPOINT);
    if (configAvailable)
    {   
        if(debugMode){
            debugPrint("HTTP Common Endpoint present.");
        }
        //Serial.println("Common endpoint file is present");
        return true;
    }else
    {   // Construct the JSON 
        // {"certUrl":{"url":"http://13.235.210.250:8000/certificates?deviceIdentifier=94:54:93:4A:27:ED"},"messageId":"cEgxwaPKJRCRloSNYW0xk3GFp"}
        //Serial.println("Constructing STATIC URL JSON ");
        // StaticJsonBuffer<256> JsonBuffer;
        // JsonObject& root = JsonBuffer.createObject(); 
        // root["cert"]["url"] = CERT_CONFIG_DEFAULT_ENDPOINT_HOST + String(":") + CERT_CONFIG_DEFAULT_ENDPOINT_PORT + CERT_CONFIG_DEFAULT_ENDPOINT_PATH 
        //                         +  "94:54:93:4A:27:ED" ;
        // root["messageId"] = "123456789";
        // root.prettyPrintTo(Serial);
       // Serial.print("MAC ID :");Serial.println(getBleMAC().toString().c_str());
        //if(uint64_t(m_bleMAC) > 0) {
            String commonEndpoint = String(CERT_CONFIG_DEFAULT_ENDPOINT_HOST) + String(":") + 
                                    String(CERT_CONFIG_DEFAULT_ENDPOINT_PORT) +
                                    String(CERT_CONFIG_DEFAULT_ENDPOINT_PATH); //+
                                    //getBleMAC().toString().c_str();
            
            String  messageId = "123456789";
            hostSerial.sendMSPCommand(MSPCommand::SET_CERT_DOWNLOAD_MSGID,messageId.c_str());
            //Serial.print("COMMON ENDPOINT : ");Serial.println(commonEndpoint);
            char config[256];
            snprintf(config, 256, "{\"certUrl\":{\"url\":\"%s\"}, \"messageId\":\"%s\"}",commonEndpoint.c_str(),messageId );
            //Serial.print("COMMON JSON : ");
            //Serial.println(config);

            bool writeSuccess = iuWiFiFlash.writeFile(IUESPFlash::CFG_STATIC_CERT_ENDPOINT,config,sizeof(config));
        
            
            return true;
        }
    return false;
        
    
}
