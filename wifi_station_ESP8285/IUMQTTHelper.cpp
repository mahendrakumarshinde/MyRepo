#include "IUMQTTHelper.h"


/* =============================================================================
    Preset values and default settings
============================================================================= */

IPAddress IUMQTTHelper::SERVER_HOST(192, 168, 43, 175);

char IUMQTTHelper::SUB_TEST_TOPIC[8] = "inTopic";
char IUMQTTHelper::PUB_MQTT_EVENT[19] = "ideplus/mqtt_event";
char IUMQTTHelper::PUB_NETWORK[16] = "ideplus/network";


/* =============================================================================
    Core
============================================================================= */

IUMQTTHelper::IUMQTTHelper(uint8_t placeholder) :
    m_wifiClient(),
    client(m_wifiClient)
{
    client.setServer(SERVER_HOST, SERVER_PORT);
    client.setCallback(mqttNewMessageCallback);
}

/**
 * Connect or reconnect to the MQTT server and (re)set subscriptions.
 *
 * Should be called repeatedly.
 * Note that this function is blocking, but if needed the library doc mentions
 * non-blocking ways to do the same.
 */
void IUMQTTHelper::reconnect(const char* clientID)
{
    while (!client.connected())
    {
        debugPrint("Attempting MQTT connection...");
        // Attempt to connect
        if (client.connect(clientID))
        {
            if (debugMode)
            {
                debugPrint("Connected as ", false);
                debugPrint(clientID);
            }
            // Resubscribe to all subscriptions here
            client.subscribe(SUB_TEST_TOPIC);
        }
        else
        {
            if (debugMode)
            {
                debugPrint("failed, rc=", false);
                debugPrint(client.state(), false);
                debugPrint(", try again in 5 seconds");
            }
            delay(5000);
        }
    }
}

/**
 * Main MQTT processing function - should called in main loop.
 *
 * Handles the reconnection to he MQTT server if needed and the publications.
 * NB: Makes use of IUMQTT::reconnect function, which may block the execution.
 */
void IUMQTTHelper::loop(const char* clientID)
{
    if (!client.connected()) {
        reconnect(clientID);
    }
    client.loop();
    // TODO implement publications
    publishEvent("alive");
}


/* =============================================================================
    Admin publications
============================================================================= */

/**
 * 
 */
bool IUMQTTHelper::publishEvent(char *msg)
{
    return client.publish(PUB_MQTT_EVENT, msg);
}

/**
 * 
 */
bool IUMQTTHelper::publishNetworkInfo()
{
    // TODO implement
}

/* =============================================================================
    Instanciation
============================================================================= */

IUMQTTHelper iuMQTTHelper(0);
