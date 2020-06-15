#ifndef IUMQTTHELPER_H
#define IUMQTTHELPER_H

#include <WiFi.h>
#include <PubSubClient.h>
#include <time.h>

#include <IUDebugger.h>
#include <MacAddress.h>
#include "BoardDefinition.h"
#include <WiFiClientSecure.h>


/* =============================================================================
    MQTT Helper
============================================================================= */

/**
 *
 *
 * It connects to an MQTT server then:
 *  - publishes "hello world" to the topic "outTopic" every two seconds
 *  - subscribes to the topic "inTopic", printing out any messages
 *    it receives. NB - it assumes the received payloads are strings not binary
 *  - If the first character of the topic "inTopic" is an 1, do something.
 *
 * It will reconnect to the server if the connection is lost using a blocking
 * reconnect function. See the 'mqtt_reconnect_nonblocking' example for how to
 * achieve the same result without blocking the main loop.
 *
 * After each reconnection, subscriptions are automatically resubscribed - see
 * private method "makeAllSubscriptions". => This function should be edited when
 * new subscriptions are required for the device.
 */
class IUMQTTHelper
{
    public:
        /***** Preset values and default settings *****/
        static const uint8_t credentialMaxLength = 45;
        static const uint8_t willMessageMaxLength = 50;
        static const uint32_t connectionTimeout = 5000;  // ms
        static const uint32_t connectionRetryDelay = 300;  // ms
        static char DEFAULT_WILL_MESSAGE[44];
        // Will definition
        static const uint8_t WILL_QOS = 0;
        static const bool WILL_RETAIN = false;
        // bool TLS_ENABLE = false;
        uint8_t mqttConnected = 0;
        /***** Core *****/
        IUMQTTHelper(/*const char * serverIP, uint16_t serverPort,
                     const char *username, const char *password*/);
        // IUMQTTHelper() : IUMQTTHelper("mqtt.infinite-uptime.com", 8883, "", "") {}
        virtual ~IUMQTTHelper() { }
        void setServer(const char * serverIP, uint16_t serverPort);
        void setCredentials(const char *username, const char *password);
        void setDeviceMAC(MacAddress deviceMAC);
        void setOnConnectionCallback(void (*callback)())
            { m_onConnectionCallback = callback; }
        bool hasConnectionInformations();
        void reconnect();
        bool publish(const char* topic, const char* payload);
        bool subscribe(const char* topic, bool deviceSpecific);
        /***** Infinite Uptime standard publications *****/
        bool publishDiagnostic(const char *payload,
                               const char *topicExtension=NULL,
                               const uint16_t extensionLength=0);
        bool publishFeature(const char *payload,
                            const char *topicExtension=NULL,
                            const uint16_t extensionLength=0);
        bool publishLog(const char *payload,
                        const char *topicExtension=NULL,
                        const uint16_t extensionLength=0);
        bool publishToFingerprints(const char *payload,
                              const char *topicExtension,
                              const uint16_t extensionLength);
        void onConnection();
        /***** Public Client for convenience *****/
        PubSubClient client;

    protected:
        /***** Core *****/
        //WiFiClient m_wifiClient;
        WiFiClientSecure m_wifiClient;

        MacAddress m_deviceMAC;
        /***** MQTT server address and credentials *****/
        /***** Settable parameters (addresses, credentials, etc) *****/
        char m_serverIP[credentialMaxLength];
        uint16_t m_serverPort;
        char m_username[credentialMaxLength];
        char m_password[credentialMaxLength];
        /***** Disconnection handling *****/
        char m_willMessage[willMessageMaxLength];
        void (*m_onConnectionCallback)() = NULL;
};

#endif // IUMQTTHELPER_H
