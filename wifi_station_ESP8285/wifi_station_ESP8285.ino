/*
 To install the ESP8266 board, (using Arduino 1.8.1+):
  - Add the following 3rd party board manager under "File -> Preferences -> Additional Boards Manager URLs":
       http://arduino.esp8266.com/stable/package_esp8266com_index.json
  - Open the "Tools -> Board -> Board Manager" and click install for the ESP8266"
  - Select your ESP8266 in "Tools -> Board"
*/

#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>

#include "IUMQTTHelper.h"
#include "IUWiFiManager.h"
#include "IUSerial.h"


/* =============================================================================
    Global Variables
============================================================================= */

// BLE MAC Address (used as device MAC address)
char BLE_MAC_ADDRESS[18] = "";
bool unknownBleMacAddress = true;
// Wifi MAC 60:01:94:80:00:BA

/***** Testing *****/
// const char* mqtt_server = "broker.mqtt-dashboard.com";
long lastMsg = 0;
char msg[50];
int value = 0;


/* =============================================================================
    MQTT functions
============================================================================= */

/**
 *
 */
void mqttPublishNetworkInfo()
{
    // TODO Implement
    iuMQTTHelper.client.publish(iuMQTTHelper.PUB_NETWORK, "");
}


/* =============================================================================
    HTTP functions
============================================================================= */

/**
 * 
 */
int requestHttpGet(const char *url, char* responseBody,
                   uint16_t maxResponseLength, const char *httpsFingerprint=NULL)
{
    if (WiFi.status() != WL_CONNECTED)
    {
        if (debugMode)
        {
            debugPrint("WiFi disconnected: GET request failed");
        }
        return 0;
    }
    HTTPClient http;
    if (httpsFingerprint)
    {
        http.begin(String(url));
    }
    else
    {
        http.begin(String(url), String(httpsFingerprint));
    }
    int httpCode = http.GET();
    if (httpCode > 0)
    {
        http.getString().toCharArray(responseBody, maxResponseLength);
        if (debugMode)
        {
            debugPrint(responseBody);
        }
    }
    http.end();
    return httpCode;
}


/* =============================================================================
    OTA functions
============================================================================= */



/* =============================================================================
    Communication to main board functions
============================================================================= */

void askBleMacAddress()
{
    hostSerial.port->print("1;");
}

void readAllMessagesFromHost()
{
    while(true)
    {
        hostSerial->readToBuffer();
        if (!hostSerial->hasNewMessage())
        {
            break;
        }
        processMessageFromHost(hostSerial.getBuffer());
        hostSerial.resetBuffer();
    }
}

void processMessageFromHost(char *buff)
{
    // Check if it is the BLE MAC in format "BLEMAC-XX:XX:XX:XX:XX:XX"
    if (strlen(buff) == 25 && ( strncmp("BLEMAC-", buff, 7) == 0 ))
    {
        strncpy(BLE_MAC_ADDRESS, &buff[7], 18);
        unknownBleMacAddress = false;
        if (debugMode)
        {
            debugPrint("Received BLE MAC from host: ", false);
            debugPrint(BLE_MAC_ADDRESS);
        }
        return;
    }
    // Else, just try to post the message to MQTT topic
    
}


/* =============================================================================
    Main setup and loop
============================================================================= */

void setup()
{
    Serial.begin(115200);
    delay(100);
    iuWifiManager.manageWifi();
    iuWifiManager.printWifiInfo();
}

void loop()
{
    if (unknownBleMacAddress)
    {
        // Don't run normal operation loop until BLE MAC Address is known.
        askBleMacAddress();
        readAllMessagesFromHost();
        return;
    }
    iuMQTTHelper.loop(BLE_MAC_ADDRESS);
    long now = millis();
    if (now - lastMsg > 2000) {
        lastMsg = now;
        ++value;
        snprintf (msg, 75, "hello world #%ld", value);
        Serial.print("Publish message: ");
        Serial.println(msg);
        iuMQTTHelper.client.publish("outTopic", msg);
    }
}
