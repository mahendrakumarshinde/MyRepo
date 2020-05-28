#include "IUMQTTHelper.h"
#include "IUClientCertificates.h"

#include "IUSerial.h" // ESP32_PORT_TRUE Debug changes
#include "MSPCommands.h" // ESP32_PORT_TRUE Debug changes
#include "Conductor.h"

extern Conductor conductor; 
extern IUSerial hostSerial; // ESP32_PORT_TRUE Debug changes

/* =============================================================================
    Preset values and default settings
============================================================================= */

char IUMQTTHelper::DEFAULT_WILL_MESSAGE[44] =
    "XXXAdmin;;;00:00:00:00:00:00;;;disconnected";


/* =============================================================================
    Core
============================================================================= */

IUMQTTHelper::IUMQTTHelper(const char * serverIP, uint16_t serverPort,
                           const char *username, const char *password) :
    m_wifiClient(),//m_wifiClientS(),
    client(m_wifiClient)//,client(m_wifiClientS)
{
    setServer(serverIP, serverPort);
    if (username != NULL && password != NULL) {
        setCredentials(username, password);
    }
    strncpy(m_willMessage, DEFAULT_WILL_MESSAGE, willMessageMaxLength);
}


/**
 * @brief 
 *  Not yet active
 * @param serverIP 
 * @param serverPort 
 */
void IUMQTTHelper::setServer(const char * serverIP, uint16_t serverPort)
{
    //(const char*)m_serverIP = serverIP;
    strcpy(m_serverIP,serverIP);
    m_serverPort = serverPort;
    if (serverIP != NULL) {
        client.setServer(serverIP, m_serverPort);
    }
}


/**
 *
 */
void IUMQTTHelper::setCredentials(const char *username, const char *password)
{
    strncpy(m_username, username, credentialMaxLength);
    strncpy(m_password, password, credentialMaxLength);
}

/**
 *
 */
void IUMQTTHelper::setDeviceMAC(MacAddress deviceMAC)
{
    // Replace MAC address in MQTT last will message
    char *pch;
    pch = strstr(m_willMessage, m_deviceMAC.toString().c_str()); // Find current
    if (pch)
    {
        strncpy(pch, deviceMAC.toString().c_str(), 17);  // Replace by new
    }
    // New MAC address
    m_deviceMAC = deviceMAC;
    if (debugMode)
    {
        debugPrint("MQTT last will updated: ", false);
        debugPrint(m_willMessage);
    }
}

/**
 * Return true if all connection info are available.
 *
 * Required connection informations are: host, port, username and password.
 */
bool IUMQTTHelper::hasConnectionInformations()
{
    return (uint32_t(m_serverIP) > 0 && m_username != NULL && m_password != NULL);
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
    if (!hasConnectionInformations()) {
        return;
    }
    uint32_t startTime = millis();
    uint32_t currentTime = startTime;
    while (!client.connected() && currentTime - startTime < connectionTimeout) {
        if (debugMode) {
            debugPrint("Attempting MQTT connection... ", false);
        }
        hostSerial.sendMSPCommand(MSPCommand::ESP_DEBUG_TO_STM_HOST,"ESP32 DEBUG : Attempting MQTT Connection");
        if((m_serverPort == 8883 || m_serverPort == 8884) && TLS_ENABLE == true ){
            // m_wifiClient.setCACert(client_ca);
            hostSerial.sendMSPCommand(MSPCommand::ESP_DEBUG_TO_STM_HOST,"ESP32 DEBUG : Setting the MQTT TLS Certificates");
            m_wifiClient.setCertificate(conductor.mqtt_client_cert );
            m_wifiClient.setPrivateKey(conductor.mqtt_client_key);
        }
        // Attempt to connect
        if (client.connect(m_deviceMAC.toString().c_str(), m_username,
                           m_password, DIAGNOSTIC_TOPIC, WILL_QOS, WILL_RETAIN,
                           m_willMessage)) {
            mqttConnected = 0;
            if (debugMode) {
                debugPrint("Success");
            }
            if (m_onConnectionCallback) {
                m_onConnectionCallback();
            }
        } else {
            mqttConnected++;
            if (debugMode) {
                debugPrint("Failed");
            }
            delay(connectionRetryDelay);
            break;
        }
    }
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
 * Format the topic name to IU standard and subscribe to the topic.
 *
 * NB: The topic name is modified as follow:
 * - if device specific, the full topic name is:
 *      cmd/<deviceType>/<deviceMacAddress>/<raw topic>
 *      eg: if raw topic = "config", the topic could be
 *          ide_plus/94:54:93:0F:67:01/config
 * - if not device specific, the full topic name is:
 *      cmd/<deviceType>/<raw topic>
 *      eg: if raw topic = "time_sync", the topic could be
 *          ide_plus/time_sync
 *   Device Diagnostic fingerprints Topics       
 *   getting all the configuration messages 
 *   ide_plus/<ble_macID>/iu_fingerprints/config 
 *   
 */
bool IUMQTTHelper::subscribe(const char* topic, bool deviceSpecific)
{
    uint16_t subsLength = strlen(topic) + DEVICE_TYPE_LENGTH + 19;
    char subscription[subsLength];
    if (deviceSpecific)
    {
        snprintf(subscription, subsLength, "%s/%s/%s", DEVICE_TYPE,
                 m_deviceMAC.toString().c_str(), topic);
    }
    else
    {
        snprintf(subscription, subsLength, "%s/%s", DEVICE_TYPE, topic);
    }
    bool result = client.subscribe(subscription);
    if (result && debugMode)
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
bool IUMQTTHelper::publishDiagnostic(const char *payload,
                                     const char *topicExtension,
                                     const uint16_t extensionLength)
{
    if (topicExtension && extensionLength > 0)
    {
        uint16_t topicLength = DIAGNOSTIC_TOPIC_LENGTH + extensionLength + 1;
        char topic[topicLength];
        snprintf(topic, topicLength, "%s/%s", DIAGNOSTIC_TOPIC, topicExtension);
        return publish(topic, payload);
    }
    else
    {
        return publish(DIAGNOSTIC_TOPIC, payload);
    }
}

/**
 *
 */
bool IUMQTTHelper::publishFeature(const char *payload,
                                  const char *topicExtension,
                                  const uint16_t extensionLength)
{
    if (topicExtension && extensionLength > 0)
    {
        uint16_t topicLength = FEATURE_TOPIC_LENGTH + extensionLength + 1;
        char topic[topicLength];
        snprintf(topic, topicLength, "%s/%s", FEATURE_TOPIC, topicExtension);
        return publish(topic, payload);
    }
    else
    {
        return publish(FEATURE_TOPIC, payload);
    }
}
/**
 *
 */
bool IUMQTTHelper::publishLog(const char *payload,
                              const char *topicExtension,
                              const uint16_t extensionLength)
{
    if (topicExtension && extensionLength > 0)
    {
        uint16_t topicLength = LOG_TOPIC_LENGTH + extensionLength + 1;
        char topic[topicLength];
        snprintf(topic, topicLength, "%s/%s", LOG_TOPIC, topicExtension);
        return publish(topic, payload);
    }
    else
    {
        return publish(LOG_TOPIC, payload);
    }
}

/**
 * Diagnostic finggerprints
 */
bool IUMQTTHelper::publishToFingerprints(const char *payload,
                              const char *topicExtension,
                              const uint16_t extensionLength)
{
    if (topicExtension && extensionLength > 0)
    {
        uint16_t topicLength = FINGERPRINT_TOPIC_LENGTH + extensionLength + 1;
        char topic[topicLength];
        snprintf(topic, topicLength, "%s/%s", FINGERPRINT_DATA_PUBLISH_TOPIC, topicExtension);
        return publish(topic, payload);
    }
    else
    {
        return publish(FINGERPRINT_DATA_PUBLISH_TOPIC, payload);
    }
}


/**
* Subscribe to all the required device subscriptions
*
* Should be called after each reconnection.
* This function should be edited when new subscriptions are required for the
* device.
*/
void IUMQTTHelper::onConnection()
{
    subscribe("config", true);  // Config subscription
    // In new version, do not subscribe to Legacy anymore
    // subscribe("legacy", true);  // Legacy command format subscription
    // In new version, subscribe to command topic (remote instruction)
    subscribe("command", true);  // Remote instruction subscription
    subscribe("post_url", false);
    subscribe("time_sync", false);  // Time synchornisation subscription
    subscribe("iu_fingerprints/config",true);  // Diagnostic Configuration subscription
}
