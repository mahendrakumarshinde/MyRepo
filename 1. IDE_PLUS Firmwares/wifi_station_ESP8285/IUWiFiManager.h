#ifndef IUWIFIMANAGER_H
#define IUWIFIMANAGER_H

// Local DNS Server used for redirecting all requests to the configuration portal
#include <DNSServer.h>
// Local WebServer used to serve the configuration portal
#include <ESP8266WebServer.h>
// https://github.com/tzapu/WiFiManager WiFi Configuration Magic
//#include <WiFiManager.h>/

#include "Utilities.h"

/**
 *
 */
class IUWiFiManager
{
    public:
        static const uint8_t wifiCredentialLength = 64;
        static const uint32_t newWiFiCredentialsTimeout = 1000;  // ms
        static const uint32_t reconnectionInterval = 5000;  // ms
        static const uint32_t heartbeatInterval = 300000;  // ms
        /***** Core *****/
        IUWiFiManager(uint32_t connectTimeout, uint32_t timeout,
                      int minSignalQuality);
        virtual ~IUWiFiManager() {}
        bool hasTimedOut(bool resetTimer);
        /***** User set SSID and password *****/;
        void resetUserSSID();
        void resetUserPassword();
        void saveCurrentCredentials();
        void addUserSSID(char *newSSID, uint8_t length);
        void addUserPassword(char *newPassword, uint8_t length);
        bool haveCredentialsTimeout();
        bool hasSavedCredentials();
        /***** Wifi connection and status *****/
//        void manageWifi(const char *apName, const char *apPassword,
//                        void (*onConnectionCallback)()=NULL);
//        bool reconnect(const char *apName, const char *apPassword,
//                       void (*onConnectionCallback)()=NULL);
        bool reconnect(bool forceNewCredentials=false);
        uint8_t waitForConnectResult();
        bool isTimeToSendWifiInfo();
        /***** Static IP settings *****/
        void setStaticIpConfig(char *staticIp, char *staticGateway,
                               char *staticSubnet);
        char *getStaticIp() { return m_staticIp; }
        char *getStaticGateway() { return m_staticGateway; }
        char *getStaticSubnet() { return m_staticSubnet; }
        /***** Debugging *****/
        void getWifiInfo(char *destination, bool mqttOn);
        void debugPrintWifiInfo();

    protected:
        int m_minSignalQuality;
        char m_staticIp[16];
        char m_staticGateway[16];
        char m_staticSubnet[16];
        /***** Timers *****/
        uint32_t m_connectTimeout;
        uint32_t m_timeout;
        uint32_t m_lastReconnectionAttempt;
        uint32_t m_lastSentHeartbeat;
        uint32_t m_lastConnected;
        /***** User set SSID and password *****/
        char m_userSSID[wifiCredentialLength];
        char m_userPassword[wifiCredentialLength];
        bool m_newSSID;
        bool m_newPassword;
        uint32_t m_newCredentialStart;
        
};


/**
// * Callback called when starting the AP + captive web portal.
// *
// * Must be passed to WiFiManager.setAPCallback.
// */
//inline void webPortalStartedCallback(WiFiManager *myWiFiManager)
//{
//    #ifdef DEBUGMODE
//    debugPrint("Entered config mode");
//    debugPrint("  Network SSID: ", false);
//    debugPrint(myWiFiManager->getConfigPortalSSID());
//    debugPrint("  Web protal IP: ", false);
//    debugPrint(WiFi.softAPIP());
//    #endif
//}


/***** Instanciation *****/

extern IUWiFiManager iuWifiManager;

#endif // IUWIFIMANAGER_H
