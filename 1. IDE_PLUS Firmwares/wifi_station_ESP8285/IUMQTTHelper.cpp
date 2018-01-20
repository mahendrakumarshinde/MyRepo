#include "IUMQTTHelper.h"


/* =============================================================================
    Preset values and default settings
====================================c========================================= */

IPAddress IUMQTTHelper::SERVER_HOST(35, 197, 32, 136);

char IUMQTTHelper::USER_NAME[9] = "ide_plus";
char IUMQTTHelper::PASSWORD[13] = "nW$Pg81o@EJD";

/* =============================================================================
    Core
============================================================================= */

IUMQTTHelper::IUMQTTHelper(uint8_t placeholder) :
    m_wifiClient(),
    client(SERVER_HOST, SERVER_PORT, m_wifiClient)
{
    strcpy(m_deviceMacAddress, "00:00:00:00:00:00");
}

/**
 * 
 */
void IUMQTTHelper::setDeviceInfo(const char *deviceType,
                                 const char *deviceMacAddress)
{
    strcpy(m_deviceType, deviceType);
    strcpy(m_deviceMacAddress, deviceMacAddress);
}

/**
 * Connect or reconnect to the MQTT server and (re)set subscriptions.
 *
 * Should be called repeatedly.
 * Note that this function is blocking, but if needed the library doc mentions
 * non-blocking ways to do the same.
 * 
 * @param willTopic MQTT topic name to publish message upon succesful connection,
 *  and to be used as will topic.
 * @param willMsg  MQTT message to publish upon disconnection from server.
 * @param onConnectionCallback  callback to be run upon connecting to MQTT server.
 * @param timeout  The duration after which the MQTT reconnection attempt is
 *  abandonned if not successful.
 */
void IUMQTTHelper::reconnect(const char *willTopic, const char *willMsg,
                             void (*onConnectionCallback)(), uint32_t timeout)
{
    uint32_t maxTime = millis() + timeout;
    while (!client.connected() && millis() < maxTime)
    {
        if (debugMode)
        {
            debugPrint("Attempting MQTT connection...");
        }
        // Attempt to connect
        if (client.connect(m_deviceMacAddress, USER_NAME, PASSWORD,
                           willTopic, WILL_QOS, WILL_RETAIN, willMsg))
        {
            if (debugMode)
            {
                debugPrint("Connected as ", false);
                debugPrint(m_deviceMacAddress);
            }
            onConnectionCallback();
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
 * 
 * @param willTopic MQTT topic name to publish message upon succesful connection,
 *  and to be used as will topic.
 * @param willMsg  MQTT message to publish upon disconnection from server.
 * @param onConnectionCallback  callback to be run upon connecting to MQTT server.
 * @param timeout  The duration after which the MQTT reconnection attempt is
 *  abandonned if not successful.
 */
void IUMQTTHelper::loop(const char *willTopic, const char *willMsg,
                        void (*onConnectionCallback)(), uint32_t timeout)
{
    if (!client.connected())
    {
        reconnect(willTopic, willMsg, onConnectionCallback, timeout);
    }
    client.loop();
}

/**
 * 
 * 
 * @param topic
 * @param payload
 */
bool IUMQTTHelper::publish(const char* topic, const char* payload)
{
    if (debugMode)
    {
        debugPrint("Published to '", false);
        debugPrint(topic, false);
        debugPrint("': ", false);
        debugPrint(payload);
    }
    return client.publish(topic, payload);
}

/**
 * 
 */
bool IUMQTTHelper::subscribe(const char* topic)
{
    char subscription[50];
    getFullSubscriptionName(subscription, topic);
    bool result = client.subscribe(subscription);
    if (result)
    {
        debugPrint("Subscribed to ", false);
        debugPrint(subscription);
    }
    return result;
}


/* =============================================================================
    Utility functions
============================================================================= */

/**
 * Get the full path of the topic to subscribe to, for the given command.
 *
 * The full topic name is:
 * cmd/<deviceType>/<deviceMacAddress>/<messageType>
 * eg: if messageType = "config", the topic could be
 * ide_plus/94:54:93:0F:67:01/config
 *
 * @param destination a char array buffer to hold the full topic name.
 * @param commandName The name of the command expected to be received from this
 *  subscription.
 */
void IUMQTTHelper::getFullSubscriptionName(char *destination,
                                           const char *commandName)
{
    strcpy(destination, m_deviceType);
    strcat(destination, "/");
    strcat(destination, m_deviceMacAddress);
    strcat(destination, "/");
    strcat(destination, commandName);
}


/* =============================================================================
    Instanciation
============================================================================= */

IUMQTTHelper iuMQTTHelper(0);
