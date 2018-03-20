#include "IUMQTTHelper.h"


/* =============================================================================
    Preset values and default settings
====================================c========================================= */

/** US-WEST1-A server **/
//IPAddress IUMQTTHelper::SERVER_HOST(35, 197, 32, 136);

/** ASIA-SOUTH1-A (Mumbai) server **/
IPAddress IUMQTTHelper::SERVER_HOST(35, 200, 183, 103);

char IUMQTTHelper::USER_NAME[9] = "ide_plus";
char IUMQTTHelper::PASSWORD[13] = "nW$Pg81o@EJD";


/* =============================================================================
    Core
============================================================================= */

IUMQTTHelper::IUMQTTHelper() :
    m_wifiClient(),
    client(SERVER_HOST, SERVER_PORT, m_wifiClient),
    m_enfOfLife(0)
{
    strcpy(m_deviceMacAddress, "00:00:00:00:00:00");
    strncpy(m_willMessage, DEFAULT_WILL_MESSAGE, willMessageMaxLength);
}

/**
 *
 */
void IUMQTTHelper::setDeviceMacAddress(const char *deviceMacAddress)
{
    // Replace MAC address in MQTT last will message
    char *pch;
    pch = strstr(m_willMessage, m_deviceMacAddress);  // Find current
    if (pch)
    {
        strncpy(pch, deviceMacAddress, 17);  // Replace by new
    }
    if (debugMode)
    {
        debugPrint("MQTT last will updated: ", false);
        debugPrint(m_willMessage);
    }
    // Copy new MAC address
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
void IUMQTTHelper::reconnect()
{
    uint32_t startTime = millis();
    uint32_t currentTime = startTime;
    while (!client.connected() && currentTime - startTime < connectionTimeout)
    {
        if (debugMode)
        {
            debugPrint("Attempting MQTT connection... ", false);
        }
        // Attempt to connect
        if (client.connect(m_deviceMacAddress, USER_NAME, PASSWORD,
                           DIAGNOSTIC_TOPIC, WILL_QOS, WILL_RETAIN,
                           m_willMessage))
        {
            if (m_onConnectionCallback)
            {
                m_onConnectionCallback();
            }
            if (debugMode)
            {
                debugPrint("Success");
            }
        }
        else
        {
            if (debugMode)
            {
                debugPrint("Failed");
            }
            delay(connectionRetryDelay);
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
void IUMQTTHelper::loop()
{
    if (!client.connected())
    {
        reconnect();
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
    Infinite Uptime standard publications
============================================================================= */

/**
 *
 */
bool IUMQTTHelper::publishDiagnostic(
    const char *rawMsg, const uint16_t msgLength, time_t datetime,
    const char *topicExtension, const uint16_t extensionLength)
{
    char message[msgLength + CUSTOMER_PLACEHOLDER_LENGTH + 51];
    strcpy(message, CUSTOMER_PLACEHOLDER);
    strcat(message, ";;;");
    strcat(message, m_deviceMacAddress);
    strcat(message, ";;;");
    strcat(message, ctime(&datetime));
    strcat(message, ";;;");
    strncat(message, rawMsg, msgLength);
    if (topicExtension && extensionLength > 0)
    {
        char topic[DIAGNOSTIC_TOPIC_LENGTH + extensionLength + 1];
        strcpy(topic, DIAGNOSTIC_TOPIC);
        strcat(topic, "/");
        strncat(topic, topicExtension, extensionLength);
        return publish(topic, message);
    }
    else
    {
        return publish(DIAGNOSTIC_TOPIC, message);
    }
}

/**
 *
 */
bool IUMQTTHelper::publishFeature(
    const char *rawMsg, const uint16_t msgLength,
    const char *topicExtension, const uint16_t extensionLength)
{
    char message[msgLength + CUSTOMER_PLACEHOLDER_LENGTH + 24];
    strcpy(message, CUSTOMER_PLACEHOLDER);
    strcat(message, ";;;");
    strcat(message, m_deviceMacAddress);
    strcat(message, ";;;");
    strncat(message, rawMsg, msgLength);
    if (topicExtension && extensionLength > 0)
    {
        char topic[FEATURE_TOPIC_LENGTH + extensionLength + 1];
        strcpy(topic, FEATURE_TOPIC);
        strcat(topic, "/");
        strncat(topic, topicExtension, extensionLength);
        return publish(topic, message);
    }
    else
    {
        return publish(DIAGNOSTIC_TOPIC, message);
    }
}

/**
* Subscribe to all the required device subscriptions
*
* Should be called after each reconnection.
* This function should be edited when new subscriptions are required for the
* device.
*/
void IUMQTTHelper::onConnection(time_t datetime)
{
    subscribe("config");  // Config subscription
    subscribe("time_sync");  // Time synchornisation subscription
    subscribe("legacy");  // Legacy command format subscription
    publishDiagnostic("connected", 9, datetime);
}

/* =============================================================================
    Faster disconnection detection
============================================================================= */

/**
 *
 */
void IUMQTTHelper::extendLifetime(uint16_t durationSec)
{
    m_enfOfLife = millis() + (durationSec * 1000);
}

/**
 *
 */
bool IUMQTTHelper::keepAlive()
{
    uint32_t now = millis();
    return (m_enfOfLife == 0 || now < m_enfOfLife);

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
    strcpy(destination, DEVICE_TYPE);
    strcat(destination, "/");
    strcat(destination, m_deviceMacAddress);
    strcat(destination, "/");
    strcat(destination, commandName);
}
