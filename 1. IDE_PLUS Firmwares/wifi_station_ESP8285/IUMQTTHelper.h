#ifndef IUMQTTHELPER_H
#define IUMQTTHELPER_H

#include <PubSubClient.h>

#include "Utilities.h"

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
        static const uint8_t willMessageMaxLength = 50;
        static const uint32_t connectionTimeout = 5000;  // ms
        static const uint32_t connectionRetryDelay = 300;  // ms
        // MQTT server access
        static IPAddress SERVER_HOST;
        static const uint16_t SERVER_PORT = 1883;
        static char USER_NAME[9];
        static char PASSWORD[13];
        // Will definition
        static const uint8_t WILL_QOS = 0;
        static const bool WILL_RETAIN = false;
        // Topics for subscriptions and publications
        // IMPORTANT - subscribe to each subscription in mqttReconnect functions
        static char PUB_MQTT_EVENT[19];
        static char PUB_NETWORK[16];
        uint16_t HEARTBEAT_DELAY = 45;  // Seconds
        uint16_t NETWORK_DELAY = 300;  // Seconds
        /***** Core *****/
        IUMQTTHelper();
        virtual ~IUMQTTHelper() { }
        void setDeviceMacAddress(const char *deviceMacAddress);
        void setOnConnectionCallback(void (*callback)())
            { m_onConnectionCallback = callback; }
        void reconnect();
        void loop();
        bool publish(const char* topic, const char* payload);
        bool subscribe(const char* topic);
        /***** Infinite Uptime standard publications *****/
        bool publishDiagnostic(const char *rawMsg, const uint16_t msgLength,
                               time_t datetime, const char *topicExtension=NULL,
                               const uint16_t extensionLength=0);
        bool publishFeature(const char *rawMsg, const uint16_t msgLength,
                            const char *topicExtension=NULL,
                            const uint16_t extensionLength=0);
        void onConnection(time_t datetime);
        /***** Faster disconnection detection *****/
        void extendLifetime(uint16_t durationSec);
        bool keepAlive();
        /***** Public Client for convenience *****/
        PubSubClient client;

    protected:
        WiFiClient m_wifiClient;
        char m_deviceMacAddress[18];
        char m_willMessage[willMessageMaxLength];
        uint32_t m_enfOfLife;
        void (*m_onConnectionCallback)() = NULL;
        /***** Utility functions *****/
        void getFullSubscriptionName(char *destination,
                                     const char *commandName);
};

#endif // IUMQTTHELPER_H
