#ifndef IUWIFIMANAGER_H
#define IUWIFIMANAGER_H

#include <ESP8266WiFi.h>
// Local DNS Server used for redirecting all requests to the configuration portal
#include <DNSServer.h>
// Local WebServer used to serve the configuration portal
#include <ESP8266WebServer.h>
// https://github.com/tzapu/WiFiManager WiFi Configuration Magic
#include <WiFiManager.h>

#include "Logger.h"

/**
 *
 */
class IUWiFiManager
{
    public:
        /***** Core *****/
        IUWiFiManager(uint32_t timeout, int minSignalQuality);
        virtual ~IUWiFiManager() {}
        void manageWifi(char const *apName=NULL, char const *apPassword=NULL,
                        bool printInfo=true);
        /***** Static IP settings *****/
        void setStaticIpConfig(char *staticIp, char *staticGateway,
                               char *staticSubnet);
        char *getStaticIp() { return m_staticIp; }
        char *getStaticGateway() { return m_staticGateway; }
        char *getStaticSubnet() { return m_staticSubnet; }
        /***** Debugging *****/
        void printWifiInfo();

    protected:
        uint32_t m_timeout;
        int m_minSignalQuality;
        char m_staticIp[16];
        char m_staticGateway[16];
        char m_staticSubnet[16];
};


/**
 * Callback called when starting the AP + captive web portal.
 *
 * Must be passed to WiFiManager.setAPCallback.
 */
inline void webPortalStartedCallback(WiFiManager *myWiFiManager)
{
    #ifdef DEBUGMODE
    debugPrint("Entered config mode");
    debugPrint("  Network SSID: ", false);
    debugPrint(myWiFiManager->getConfigPortalSSID());
    debugPrint("  Web protal IP: ", false);
    debugPrint(WiFi.softAPIP());
    #endif
}


/***** Instanciation *****/

extern IUWiFiManager iuWifiManager;

#endif // IUWIFIMANAGER_H
