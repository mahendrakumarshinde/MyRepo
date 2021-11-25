/*
  Infinite Uptime WiFi Module Firmware
  Update 25-11-2021
*/

#include "Conductor.h"
#include <Ticker.h>
#include <rom/rtc.h>
#include "soc/soc.h"
#include "soc/rtc_cntl_reg.h"
#include "IUESPFlash.h"
//#include "SPIFFS.h"
Conductor conductor;
uint32_t lastDone = 0;
uint32_t lastReqHash = 0;
/* =============================================================================
    MQTT callbacks
============================================================================= */

/**
 * Callback called when a new MQTT message is received from MQTT server.
 *
 * See conductor.processMessageFromMQTT
 */
void mqttNewMessageCallback(char* topic, byte* payload, unsigned int length) // ESP32_PORT_TRUE
{   
    conductor.processMessageFromMQTT(topic, (char*) payload, length);
}

/**
 * Callback for MQTT (re)connections.
 */
void onMQTTConnection()
{
    mqttHelper.onConnection();
    conductor.publishDiagnostic("connected", 10);
    hostSerial.sendMSPCommand(MSPCommand::ASK_HOST_FIRMWARE_VERSION);
    hostSerial.sendMSPCommand(MSPCommand::ASK_HOST_SAMPLING_RATE);
    hostSerial.sendMSPCommand(MSPCommand::ASK_HOST_BLOCK_SIZE);
}

void getAllConfig()
{   
    mqttHelper.onConnection();
    conductor.publishDiagnostic("connected", 10);
    hostSerial.sendMSPCommand(MSPCommand::GET_DEVICE_CONFIG);
    
}
/* =============================================================================
    Host message callback
============================================================================= */

void onNewHostMessageFromHost(IUSerial *iuSerial) {
    conductor.processHostMessage(iuSerial);
}

/* =============================================================================
    Main setup and loop
============================================================================= */

void setup()
{   
    setCpuFrequencyMhz(240);
    hostSerial.begin();
    hostSerial.setOnNewMessageCallback(onNewHostMessageFromHost);
    if (debugMode) {
        delay(5000);
    }
    #if IUDEBUG_ANY == 1
        conductor.forceWiFiConfig(testSSID, testPSK, testStaticIP,
                                  testGateway, testSubnet);
        conductor.setBleMAC(hostMacAddress);
    #else
        conductor.getConfigFromMainBoard();
    #endif
    // If this point is reached, tell host that WiFi is waking up
    hostSerial.sendMSPCommand(MSPCommand::WIFI_ALERT_AWAKE);
    iuWiFiFlash.begin();

    byte mqttConnectionMode = iuWiFiFlash.readMemory(CONNECTION_MODE);
    //Serial.print("connection Mode Before : ");Serial.println(mqttConnectionMode);
    if(mqttConnectionMode != UNSECURED && mqttConnectionMode != SECURED){
        iuWiFiFlash.updateValue(CONNECTION_MODE,UNSECURED); // Default Un-Secured connectionMode
        //Serial.println("Using Default, Connection CONNECTION_MODE");
        if(debugMode){
            debugPrint("Using Default , if invalid connectionMode found");
        }
        conductor.m_secure = false;
    }

    mqttConnectionMode = iuWiFiFlash.readMemory(CONNECTION_MODE);
    //Serial.print("connection Mode After : ");Serial.println(mqttConnectionMode);
    conductor.m_secure = mqttConnectionMode;
    
    // Prepare to receive MQTT messages
    if(mqttConnectionMode == UNSECURED){
        mqttHelper.nonSecureClient.setCallback(mqttNewMessageCallback);
    }else if (mqttConnectionMode == SECURED){
        mqttHelper.client.setCallback(mqttNewMessageCallback);
    }
    // mqttHelper.setOnConnectionCallback(onMQTTConnection);
    mqttHelper.setOnConnectionCallback(getAllConfig);           // Once the MQTT client connected 
    delay(100);
    #if IUDEBUG_ANY == 1
        conductor.reconnect(true);
    #endif
     
    //iuWiFiFlash.begin();
    // Set the common url json if file not present
    conductor.setMQTTConfig();
    conductor.setHTTPConfig();
    // TODO : Use unsecure flag here condition check required based on EEPROM flag
    if(iuWiFiFlash.readMemory(CONNECTION_MODE) == SECURED) {
        conductor.setCertificateManagerHttpEndpoint();
        //Configure the Diagnostic HTTP/HTTPS Endpoint
        conductor.configureDiagnosticEndpointFromFlash(IUESPFlash::CFG_DIAGNOSTIC_ENDPOINT);
        conductor.activeCertificates = iuWiFiFlash.readMemory(CERT_ADDRESS);
        if((conductor.activeCertificates == 0 && iuWiFiFlash.isFilePresent(IUESPFlash::CFG_HTTPS_OEM_ROOTCA0)) ||
        (conductor.activeCertificates == 1 && iuWiFiFlash.isFilePresent(IUESPFlash::CFG_HTTPS_OEM_ROOTCA1))){
                conductor.oemRootCAPresent = true;
        }
        conductor.espResetCount = iuWiFiFlash.readMemory(ESP_RESET_ADDRESS);
        conductor.certificateDownloadStatus = iuWiFiFlash.readMemory(CERT_DOWNLOAD_STATUS);
    }
    conductor.setWiFiConfig();
    conductor.sendWiFiConfig();
    conductor.wifiConnectTryFlag = true;
}
/**
 * Unless there is a connection attempt going on, the loop typically takes
 * ~ 1ms to complete, without accounting for the delay at the end.
 * The delay is there to allow the WiFi to light-sleep.
 */
void loop()
{
    if(!conductor.configStatus){
        delay(100);
        getAllConfig();
    }
    hostSerial.readMessages();  
    if (conductor.reconnect()) {  
        if (!timeHelper.active()) {
            timeHelper.begin();
        }
        // Update time reference from NTP server if not yet received
        // from IU server
        timeHelper.updateTimeReferenceFromNTP();
        // TODO : MQTT connection / message reception loop
        if(iuWiFiFlash.readMemory(CONNECTION_MODE) == UNSECURED){
            conductor.loopMQTT(); // use for non-secure version
        }
        conductor.publishWifiInfoCycle();
    }
    // TODO : use the conection Type flag 
    if(iuWiFiFlash.readMemory(CONNECTION_MODE) == SECURED){
        conductor.mqttSecureConnect();
    }
    conductor.updateWiFiStatusCycle();
    conductor.checkWiFiDisconnectionTimeout();
    conductor.checkMqttDisconnectionTimeout();
    conductor.checkOtaPacketTimeout();
    /* Commented autoreconnect, as STM periodically sends User defined WIFI config and Administrator NW config,
       so need to have this at ESP. Also because this autoreconnect(), existing wifi connection req. from STM
       which is in progress is retried with this function, causing delay in connection */
    // if(WiFi.isConnected() == false)         
    // {   
    //     conductor.autoReconncetWifi();
    // } 
    uint32_t now = millis();
    static uint8_t rssiPublishedCounter;
    if (now - lastDone > 5000 )
    {   
        //iuWiFiFlash.listAllAvailableFiles(IUESPFlash::CONFIG_SUBDIR);
        //Serial.print("EEPROM Value :");
        //Serial.println(iuWiFiFlash.readMemory(CERT_ADDRESS));
        rssiPublishedCounter++ ;
        if (rssiPublishedCounter >= 6)
        {
            conductor.publishRSSI();
            rssiPublishedCounter = 0;
        }
        conductor.resetDownloadInitTimer(10,5000);
        lastDone = now;
        if(uint64_t(conductor.getBleMAC() ) == 0) { 
            hostSerial.sendMSPCommand(MSPCommand::ASK_BLE_MAC);
        }
        
    }

    if (now - lastReqHash > 30000 &&  (conductor.otaInProgress == false && conductor.certificateDownloadInProgress == false )  )
    {
        if(iuWiFiFlash.readMemory(CONNECTION_MODE) == SECURED) {
            if(!iuWiFiFlash.isFilePresent(IUESPFlash::CFG_STATIC_CERT_ENDPOINT) || (conductor.certHashReceived == true && strcmp(conductor.certHash,conductor.getConfigChecksum(IUESPFlash::CFG_STATIC_CERT_ENDPOINT)) != 0)) {
                hostSerial.sendMSPCommand(MSPCommand::GET_CERT_CONNECTION_INFO);
            }
            if(!iuWiFiFlash.isFilePresent(IUESPFlash::CFG_DIAGNOSTIC_ENDPOINT) || (conductor.diagCertHashReceived == true && strcmp(conductor.diagCertHash,conductor.getConfigChecksum(IUESPFlash::CFG_DIAGNOSTIC_ENDPOINT)) != 0)) {
                hostSerial.sendMSPCommand(MSPCommand::GET_DIAG_CERT_CONNECTION_INFO);
            }
        }
        if(!iuWiFiFlash.isFilePresent(IUESPFlash::CFG_MQTT) || (conductor.mqttHashReceived == true  && strcmp(conductor.mqttHash,conductor.getConfigChecksum(IUESPFlash::CFG_MQTT)) != 0)) {
            hostSerial.sendMSPCommand(MSPCommand::GET_MQTT_CONNECTION_INFO);
        }
        if(!iuWiFiFlash.isFilePresent(IUESPFlash::CFG_HTTP) || (conductor.httpHashReceived == true && strcmp(conductor.httpHash,conductor.getConfigChecksum(IUESPFlash::CFG_HTTP)) != 0)) {
            hostSerial.sendMSPCommand(MSPCommand::GET_RAW_DATA_ENDPOINT_INFO);
        }
         if(!iuWiFiFlash.isFilePresent(IUESPFlash::CFG_WIFI) || (conductor.wifiHashReceived == true && strcmp(conductor.wifiHash,conductor.getConfigChecksum(IUESPFlash::CFG_WIFI)) != 0)) {
            hostSerial.sendMSPCommand(MSPCommand::ASK_WIFI_CONFIG);
        }
        lastReqHash = now;
    }
    delay(1);
}
