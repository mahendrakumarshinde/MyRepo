#ifndef IUMQTTHELPER_H
#define IUMQTTHELPER_H

#include <ESP8266WiFi.h>
#include <PubSubClient.h>

#include "Logger.h"

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
 */
class IUMQTTHelper
{
    public:
        /***** Preset values and default settings *****/
        static IPAddress SERVER_HOST;
        static const uint16_t SERVER_PORT = 1883;
        // Topics for subscriptions and publications
        // IMPORTANT - subscribe to each subscription in mqttReconnect functions
        static char SUB_TEST_TOPIC[8];
        static char PUB_MQTT_EVENT[19];
        static char PUB_NETWORK[16];
        uint16_t HEARTBEAT_DELAY = 45;  // Seconds
        uint16_t NETWORK_DELAY = 300;  // Seconds
        /***** Core *****/
        IUMQTTHelper(uint8_t placeholder);
        virtual ~IUMQTTHelper() { }
        void loop(const char* clientID);
        /***** Admin publications *****/
        bool publishEvent(char *msg);
        bool publishNetworkInfo();
        /***** Public Client for convenience *****/
        PubSubClient client;

    protected:
        WiFiClient m_wifiClient;
        /***** Core functions *****/
        void reconnect(const char* clientID);
};

/**
 * Callback called when a new MQTT message is received from MQTT server.
 *
 * The message can come from any previously subscribed topic (see
 * PubSubClient.subscribe).
 * @param topic The Pubsub topic the message came from
 * @param payload The message it self, as an array of bytes
 * @param length The number of byte in payload
 */
inline void mqttNewMessageCallback(char* topic, byte* payload,
                                   unsigned int length)
{
    if (debugMode)
    {
        debugPrint("Message arrived [", false);
        debugPrint(topic, false);
        debugPrint("] ", false);
        for (int i = 0; i < length; i++)
        {
            debugPrint((char)payload[i], false);
        }
        debugPrint("");
    }
    // TODO: Send message to main board for processing
}


/***** Instanciation *****/

extern IUMQTTHelper iuMQTTHelper;

#endif // IUMQTTHELPER_H
