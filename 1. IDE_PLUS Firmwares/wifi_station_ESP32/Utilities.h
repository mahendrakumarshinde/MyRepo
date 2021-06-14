#ifndef UTILITIES_H
#define UTILITIES_H

#include <Arduino.h>
#include <time.h>

#include <WiFi.h>
#include <HTTPClient.h>
#include <IUDebugger.h>
#include <WiFiClientSecure.h>
#include "IUSerial.h" // ESP32_PORT_TRUE Debug changes
#include "MSPCommands.h" // ESP32_PORT_TRUE Debug changes
extern IUSerial hostSerial; // ESP32_PORT_TRUE Debug changes

#define WIFICLIENT_MAX_PACKET_SIZE 1460

/* =============================================================================
    HTTP functions
============================================================================= */
namespace HttpContentType {
    static char* applicationJSON = "json\r\n";
    static char* octetStream = "octet-stream\r\n";
    static char* textPlain  = "plain\r\n";
}
#if 1 // ESP32_PORT_TRUE
/**
 * Sends an HTTP GET request - HTTPS is used if fingerprint is given.
 *
 * @param url
 * @param responseBody
 * @param maxResponseLength
 * @param httpsFingerprint
 */
inline int httpGetRequest(const char *url, char* responseBody,
                          uint16_t maxResponseLength,
                          String auth = "",
                          const char *httpsFingerprint=NULL)
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
        //http.begin(String(url), String(httpsFingerprint));
    }
    else
    {
        http.begin(String(url));
        http.addHeader("Authorization", "Basic " + auth);
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
#endif // ESP32_PORT_TRUE
// Get the configuration messages 


/**
 * Sends an HTTP GET request - HTTPS is used if fingerprint is given.
 *
 * @param url
 * @param maxResponseLength
 * @param httpsFingerprint
 * 
 * return - responseBody 
 */
inline String httpGET(const char *url,uint16_t maxResponseLength,
                          const char *httpsFingerprint=NULL)
{
   String responseBody;
   
    if (WiFi.status() != WL_CONNECTED)
    {
        if (debugMode)
        {
            debugPrint("WiFi disconnected: GET request failed");
        }
        return "WIFI-DISCONNECTED";
    }
    HTTPClient http;
    if (httpsFingerprint)
    {
      //  http.begin(String(url), String(httpsFingerprint)); // ESP32_PORT_TRUE  
    }
    else
    {
        http.begin(String(url));
    }
    int httpCode = http.GET();
    if (httpCode > 0)
    {
        responseBody = http.getString();//.toCharArray(responseBody, maxResponseLength);
        if (debugMode)
        {
            debugPrint(responseBody);
        }
    }
    http.end();
    if(httpCode > 0){
      return responseBody;
    }
   else {
    return String(httpCode);
   }
}

#if 0 // ESP32_PORT_TRUE
/**
 * Sends an HTTP POST request - HTTPS is used if fingerprint is given.
 *
 * @param url
 * @param payload
 * @param payloadLength
 * @param responseBody
 * @param maxResponseLength
 * @param httpsFingerprint
 */
inline int httpPostJsonRequest(const char *url, char *payload,
                               uint16_t payloadLength, char* responseBody,
                               uint16_t maxResponseLength,
                               const char *httpsFingerprint=NULL)
{
    if (WiFi.status() != WL_CONNECTED)
    {
        if (debugMode)
        {
            debugPrint("WiFi disconnected: POST request failed");
        }
        return 0;
    }
    HTTPClient http;
    if (httpsFingerprint)
    {
        http.begin(String(url), String(httpsFingerprint));
    }
    else
    {
        http.begin(String(url));
    }
    http.addHeader("Content-Type", "application/json");
    int httpCode = http.POST((uint8_t*) payload, (size_t) payloadLength);
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
#endif // ESP32_PORT_TRUE

/**
 * Sends an HTTP POST request with a "Big" payload
 *
 * "Big" means that its size is above the max packet size which is
 * around 3KB (2920 bytes in my experience).
 *
 * @param endpointHost
 * @param endpointURL
 * @param payload
 * @param payloadLength
 * @param chunkSize should at most WIFICLIENT_MAX_PACKET_SIZE
 *  (= HTTP_TCP_BUFFER_SIZE), currently 1460 bytes.
 * @param tcpTimeout
 * @return the HTTP code, or 0 if connection couldn't be established,
 * or a negative number for HTTPClient errors (see HTTPC_ERROR in
 * ESP8266HTTPClient.h)
 */
inline int httpPostBigRequest(
    const char *endpointHost, const char *endpointURL,
    uint16_t endpointPort, uint8_t *payload, uint16_t payloadLength,
    String auth ="",
    char* ssl_rootCA = NULL,
    char* contentType = HttpContentType::applicationJSON,
    size_t chunkSize=WIFICLIENT_MAX_PACKET_SIZE,
    uint16_t tcpTimeout=HTTPCLIENT_DEFAULT_TCP_TIMEOUT + 10000)
{
     
    if (WiFi.status() != WL_CONNECTED)
    {
        if (debugMode)
        {
            debugPrint("WiFi disconnected: POST request failed");
        }
         return 404; // 0
    }
    char type[12];
    if (strncmp(contentType,"plain",5) == 0){
        strncpy(type,"text",12);}
    else
    {
        strncpy(type,"application",12); 
    }
    
    // create the request and headers
    String request = "POST " + String(endpointURL) + " HTTP/1.1\r\n" +
        "Host: " + String(endpointHost) + "\r\n" +
        "Accept: application/json" + "\r\n" +
        "Content-Type:" + type + "/" + contentType +
        "Authorization: Basic " + auth +"\r\n" +
        "Content-Length: " + String(payloadLength) + "\r\n\r\n";
    // Use WiFiClient class to create TCP connections
    WiFiClient client;
// Runtime Polymorphism to chose between the object    
//     bool useHttp = ssl_rootCA == NULL ? 1 : 0;
//     Serial.print("HTTP ? : ");Serial.println(useHttp);
   
//     dynamic_cast< WiFiClientSecure > client = (useHttp) ? new WiFiClient() : new WiFiClientSecure();
   
//    if(!useHttp){
//        client.setCACert(ssl_rootCA);
//        Serial.println("rootCA SET");
//    }
    
    //Serial.println(request);
    int connectResult = client.connect(endpointHost, endpointPort);
    if (connectResult == 0)
    {
        if (debugMode)
        {
            debugPrint("Couldn't connect to host: ", false);
            debugPrint(endpointHost, false);
            debugPrint(":", false);
            debugPrint(endpointPort);
            debugPrint("\nHEADERS:");
            debugPrint(request);
        }
        return 505; //connectResult;  // 0 means no connection
    }
    // This will send the request and headers to the server
    client.print(request);
    // now we need to chunk the payload into 1000 byte chunks
    size_t cIndex;
    size_t retSize;
    if(payloadLength > chunkSize) {
    for (cIndex = 0; cIndex < payloadLength - chunkSize;
         cIndex = cIndex + chunkSize)
    {
        retSize = client.write(&payload[cIndex], chunkSize);
        if(retSize != chunkSize)
        {
            if (client.connected() || (client.available() > 0))
            {
                client.stop();
            }
            return HTTPC_ERROR_SEND_PAYLOAD_FAILED;  // -3
        }
    }
    retSize = client.write(&payload[cIndex], payloadLength - cIndex);
    if(retSize != payloadLength - cIndex)
    {
        if (client.connected() || (client.available() > 0))
        {
            client.stop();
        }
        return HTTPC_ERROR_SEND_PAYLOAD_FAILED;  // -3
    }
    }
    else if(payloadLength < chunkSize && payloadLength > 0)
    {
        retSize = client.write(&payload[0], payloadLength);
        if(retSize != payloadLength)
        {
            if (client.connected() || (client.available() > 0))
            {
                client.stop();
            }

            return HTTPC_ERROR_SEND_PAYLOAD_FAILED;  // -3
        }       
    }
    // Handle response
    uint32_t lastDataTime = millis();
    int returnCode;
    while(client.connected() || client.available())
    {
        size_t len = client.available();
        if(len > 0)
        {
            String headerLine = client.readStringUntil('\n');
            headerLine.trim(); // remove \r
            lastDataTime = millis();

            if(headerLine.startsWith("HTTP/1.")) {
                returnCode = headerLine.substring(
                    9, headerLine.indexOf(' ', 9)).toInt();
            }
            if(headerLine == "")
            {
                if(returnCode)
                {
                     return returnCode;
                }
                else
                {
                    return HTTPC_ERROR_NO_HTTP_SERVER;  // -7
                }
            }
        }
        else
        {
            if((millis() - lastDataTime) > tcpTimeout) {
                return HTTPC_ERROR_READ_TIMEOUT;  // -11
            }
            delay(0);
        }
    }
    return HTTPC_ERROR_CONNECTION_LOST;  // -5
    
}



/**
 * Sends an HTTP POST request with a "Big" payload
 *
 * "Big" means that its size is above the max packet size which is
 * around 3KB (2920 bytes in my experience).
 *
 * @param endpointHost
 * @param endpointURL
 * @param payload
 * @param payloadLength
 * @param chunkSize should at most WIFICLIENT_MAX_PACKET_SIZE
 *  (= HTTP_TCP_BUFFER_SIZE), currently 1460 bytes.
 * @param tcpTimeout
 * @return the HTTP code, or 0 if connection couldn't be established,
 * or a negative number for HTTPClient errors (see HTTPC_ERROR in
 * ESP8266HTTPClient.h)
 */
inline int httpsPostBigRequest(
    const char *endpointHost, const char *endpointURL,
    uint16_t endpointPort, uint8_t *payload, uint16_t payloadLength,
    String auth ="",
    char* ssl_rootCA = NULL,
    char* contentType = HttpContentType::applicationJSON,
    size_t chunkSize=WIFICLIENT_MAX_PACKET_SIZE,
    uint16_t tcpTimeout=HTTPCLIENT_DEFAULT_TCP_TIMEOUT + 10000)
{
     
    if (WiFi.status() != WL_CONNECTED)
    {
        if (debugMode)
        {
            debugPrint("WiFi disconnected: POST request failed");
        }
         return 404; // 0
    }
    char type[12];
    if (strncmp(contentType,"plain",5) == 0){
        strncpy(type,"text",12);}
    else
    {
        strncpy(type,"application",12); 
    }
    
    // create the request and headers
    String request = "POST " + String(endpointURL) + " HTTP/1.1\r\n" +
        "Host: " + String(endpointHost) + "\r\n" +
        "Accept: application/json" + "\r\n" +
        "Content-Type:" + type + "/" + contentType +
        "Authorization: Basic " + auth +"\r\n" +
        "Content-Length: " + String(payloadLength) + "\r\n\r\n";
    // Use WiFiClient class to create TCP connections
    //WiFiClient client;
    WiFiClientSecure client;
    //if(ssl_rootCA != NULL){
    if(ssl_rootCA == NULL){
        //device should auto verify cert
    }
    else{
        client.setCACert(ssl_rootCA);
    }
    int connectResult = client.connect(endpointHost, endpointPort);
    if (connectResult == 0)
    {
        if (debugMode)
        {
            debugPrint("Couldn't connect to host: ", false);
            debugPrint(endpointHost, false);
            debugPrint(":", false);
            debugPrint(endpointPort);
            debugPrint("\nHEADERS:");
            debugPrint(request);
        }
        return 505; //connectResult;  // 0 means no connection
    }
    // This will send the request and headers to the server
    client.print(request);
    // now we need to chunk the payload into 1000 byte chunks
    size_t cIndex;
    size_t retSize;
    if(payloadLength > chunkSize) {
    for (cIndex = 0; cIndex < payloadLength - chunkSize;
         cIndex = cIndex + chunkSize)
    {
        retSize = client.write(&payload[cIndex], chunkSize);
        if(retSize != chunkSize)
        {
            if (client.connected() || (client.available() > 0))
            {
                client.stop();
            }
            return HTTPC_ERROR_SEND_PAYLOAD_FAILED;  // -3
        }
    }
    retSize = client.write(&payload[cIndex], payloadLength - cIndex);
    if(retSize != payloadLength - cIndex)
    {
        if (client.connected() || (client.available() > 0))
        {
            client.stop();
        }
        return HTTPC_ERROR_SEND_PAYLOAD_FAILED;  // -3
    }
    }
    else if(payloadLength < chunkSize && payloadLength > 0)
    {
        retSize = client.write(&payload[0], payloadLength);
        if(retSize != payloadLength)
        {
            if (client.connected() || (client.available() > 0))
            {
                client.stop();
            }

            return HTTPC_ERROR_SEND_PAYLOAD_FAILED;  // -3
        }       
    }
    // Handle response
    uint32_t lastDataTime = millis();
    int returnCode;
    while(client.connected() || client.available())
    {
        size_t len = client.available();
        if(len > 0)
        {
            String headerLine = client.readStringUntil('\n');
            headerLine.trim(); // remove \r
            lastDataTime = millis();

            if(headerLine.startsWith("HTTP/1.")) {
                returnCode = headerLine.substring(
                    9, headerLine.indexOf(' ', 9)).toInt();
            }
            if(headerLine == "")
            {
                if(returnCode)
                {
                     return returnCode;
                }
                else
                {
                    return HTTPC_ERROR_NO_HTTP_SERVER;  // -7
                }
            }
        }
        else
        {
            if((millis() - lastDataTime) > tcpTimeout) {
                return HTTPC_ERROR_READ_TIMEOUT;  // -11
            }
            delay(0);
        }
    }
    return HTTPC_ERROR_CONNECTION_LOST;  // -5
    
}


/*
 * 
 * post big json with httpclient
 */

inline int publishBigJSON(const char *endpointHost, const char *endpointURL,
    uint16_t endpointPort, uint8_t *payload, uint16_t payloadLength,
    size_t chunkSize=WIFICLIENT_MAX_PACKET_SIZE,
    uint16_t tcpTimeout=HTTPCLIENT_DEFAULT_TCP_TIMEOUT){

if(WiFi.status()== WL_CONNECTED){   //Check WiFi connection status
 
   HTTPClient http;    //Declare object of class HTTPClient
 
  
   //http.begin("http://115.112.92.146:58888/contineonx-web-admin/imiot-infiniteuptime-api/postdatadump");      //Specify request destination
  
   String url = String(endpointHost) + String(":")+ String(endpointPort) + String(endpointURL);   
   http.begin(url);      //Specify request destination

  // http.addHeader("POST",String(endpointURL) + " HTTP/1.1\r\n" );  //Specify content-type header
  // http.addHeader("Host",String(endpointHost) + "\r\n" );         //Specify content-type header
  // http.addHeader("Accept","application/json\r\n");         //Specify content-type header
  // http.addHeader("Content-Type", "application/json\r\n");    //Specify content-type header
  // http.addHeader("Content-Length", String(payloadLength) + "\r\n\r\n");  //Specify content-type header
  http.addHeader("Content-Type", "text/plain"); 
     
  // create the request and headers
  //String request =
 /* http.addHeader("POST " + String(end     pointURL) + " HTTP/1.1\r\n" +
                  "Host: " + String(endpointHost) + "\r\n" + 
                  "Accept: application/json" + "\r\n" + 
                  "Content-Type: application/json\r\n" +
                  "Content-Length: " + String(payloadLength) + "\r\n\r\n");
  */                
   
   //http.addHeader(request);
   
   //int httpCode = http.POST(url); //+ macAddress.toString().c_str() );   //Send the request
   int httpCode = http.POST((const char*)payload); //+ macAddress.toString().c_str() );   //Send the request
   
   String payload = http.getString();                  //Get the response payload
 
   //Serial.println(httpCode);   //Print HTTP return code
   //Serial.println(payload);    //Print request response payload
 
   http.end();  //Close connectionm_lastSentHeartbeat
   return httpCode;
 }else{

    return 222;
    debugPrint("Error in WiFi connection");   
 
 }

}

#endif // UTILITIES_H
