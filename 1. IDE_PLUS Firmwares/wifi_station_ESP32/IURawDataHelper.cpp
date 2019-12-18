#include "IURawDataHelper.h"


/* =============================================================================
    Core
============================================================================= */

char IURawDataHelper::EXPECTED_KEYS[IURawDataHelper::EXPECTED_KEY_COUNT + 1] =
    "XYZ";


IURawDataHelper::IURawDataHelper(
    uint32_t inputTimeout, uint32_t postTimeout, const char *endpointHost,
    const char *endpointRoute, uint16_t enpointPort) :
    m_endpointPort(enpointPort),
    m_payloadCounter(0),
    m_payloadStartTime(0),
    m_inputTimeout(inputTimeout),
    m_firstPostTime(0),
    m_postTimeout(postTimeout)
{
    strncpy(m_endpointHost, endpointHost, MAX_HOST_LENGTH);
    strncpy(m_endpointRoute, endpointRoute, MAX_ROUTE_LENGTH);
    resetPayload();
}


/* =============================================================================
    Payload construction
============================================================================= */

/**
 * Empty the payload and reset the associated counters and booleans
 */
void IURawDataHelper::resetPayload()
{
    for (uint16_t i = 0; i < MAX_PAYLOAD_LENGTH; i++)
    {
        m_payload[i] = 0;
    }
    m_payloadCounter = 0;
    m_payloadStartTime = 0;
    m_firstPostTime = 0;
    for (uint8_t i = 0; i < EXPECTED_KEY_COUNT; ++i)
    {
        m_keyAdded[i] = false;
    }
}

/**
 * Tells whether too much time passed since starting it receiving inputs.
 *
 * @return true if a timeout happened and that the payload needs to be
 * reset, else false.
 */
bool IURawDataHelper::inputHasTimedOut()
{
    if (m_payloadStartTime == 0)
    {
        return false;
    }
    if (m_payloadCounter > 0 && (millis() -
            m_payloadStartTime > m_inputTimeout))
    {
        if (debugMode)
        {
            debugPrint("Raw data payload has timed out.");
        }
        return true;
    }
    return false;
}

/**
 * Tells whether too much time passed since starting it receiving inputs.
 *
 * @return true if a timeout happened and that the payload needs to be
 * reset, else false.
 */
bool IURawDataHelper::postPayloadHasTimedOut()
{
    if (m_firstPostTime == 0)
    {
        return false;
    }
    if (millis() - m_firstPostTime > m_postTimeout)
    {
        if (debugMode)
        {
            debugPrint("Raw data post payload has expired.");
        }
        return true;
    }
    return false;
}

/**
 * Add a key-value pair to the JSON payload.
 *
 * Function checks that:
 *  - given key is part of the expected keys
 *  - given keyhas not already been added.
 *  - the payload is large enough to add the key-value pair.
 *
 * @return true if the key, value pair was succesfully added, else false.
 */
bool IURawDataHelper::addKeyValuePair(char key, const char *value,
                                      uint16_t valueLength)
{
    char *foundKey = strchr(EXPECTED_KEYS, key);
    if (foundKey == NULL)
    {
        if (debugMode)
        {
            debugPrint("Raw data handler received unexpected key: ", false);
            debugPrint(key);
        }
        return false;
    }
    uint8_t idx = (uint8_t) (foundKey - EXPECTED_KEYS);
    if (m_keyAdded[idx])
    {
        if (debugMode)
        {
            debugPrint("Raw data handler received twice the key: ", false);
            debugPrint(key, false);
            debugPrint(" at idx: ", false);
            debugPrint(idx);
        }
        return false;
    }
    uint16_t available = MAX_PAYLOAD_LENGTH - m_payloadCounter - 1;
    if (valueLength + 7 > available)
    {
        if (debugMode)
        {
            debugPrint("Raw data handler - not enough available space in "
                       "payload", false);
            debugPrint(key);
        }
        return false;
    }
    if (m_payloadCounter == 0)
    {
        strcpy(m_payload, "{\"");
    }
    else
    {
        strncat(m_payload, ",\"", 2);
    }
    m_payloadCounter += 2;
    m_payload[m_payloadCounter++] = key;
    m_payload[m_payloadCounter++] = '"';
    m_payload[m_payloadCounter++] = ':';
    m_payload[m_payloadCounter++] = '"';
//    strncat(m_payload, &key, 1);
//    strncat(m_payload, "\":\"", 3);
    strncat(m_payload, value, valueLength);
    m_payloadCounter += valueLength;
    m_payload[m_payloadCounter++] = '"';
//    char quote = '\"';
//    strncat(m_payload, &quote, 1);
//    m_payloadCounter += valueLength + 7;
    // Handle key duplication and timeout
    if (m_payloadStartTime == 0)
    {
        m_payloadStartTime = millis();
    }
    m_keyAdded[idx] = true;
    return true;
}

/**
 * Check whether all expected keys have been added to the payload.
 */
bool IURawDataHelper::areAllKeyPresent()
{
    for (uint8_t i = 0; i < EXPECTED_KEY_COUNT; ++i)
    {
        if (!m_keyAdded[i])
        {
            return false;
        }
    }
    return true;
}


/* =============================================================================
    HTTP Post request
============================================================================= */

/**
 * Post the payload if ready
 */
int IURawDataHelper::publishIfReady(MacAddress macAddress)
{
    if (inputHasTimedOut() || postPayloadHasTimedOut())
    {
        resetPayload();
        return 111;   //false
    }
    if (!areAllKeyPresent()) // Raw Data payload is not ready
    {   
        
        return 222; //false
    }
    
    m_firstPostTime = millis();
    int httpCode = httpPostPayload(macAddress);
    if (debugMode)
    {
        debugPrint("Post raw data: ", false);
        debugPrint(httpCode);
    }
    if (httpCode == 200)
    {
        resetPayload();
        return httpCode;  //200
    }
    else
    {
        return httpCode; //false
    }
}

/**
 * Post the payload
 */
int IURawDataHelper::httpPostPayload(MacAddress macAddress)
{
    // Close JSON first (last curled brace) if not closed yet
    char *result = NULL;
    if (m_payload[m_payloadCounter - 1] != '}')
    {
        m_payload[m_payloadCounter++] = '}';
    }
    if (debugMode)
    {
        debugPrint("Post payload: ", false);
        debugPrint(m_payload);
    }
    char fullUrl[strlen(m_endpointRoute) + 18];
    strcpy(fullUrl, m_endpointRoute);
    strncat(fullUrl, macAddress.toString().c_str(), 18);
    if (debugMode)
    {
        debugPrint("Host: ", false);
        debugPrint(m_endpointHost);
        debugPrint("Port: ", false);
        debugPrint(m_endpointPort);
        debugPrint("URL: ", false);
        debugPrint(fullUrl);
    }
    result = strstr(fullUrl,"/raw_data?mac=");
    if(result != NULL){
      // infinite uptime http
      return httpPostBigRequest(m_endpointHost, fullUrl, m_endpointPort,
                                    (uint8_t*) m_payload, m_payloadCounter);
      
    }
    else {
    // others like indicus software
    return publishBigJSON(m_endpointHost, fullUrl, m_endpointPort,
                                 (uint8_t*) m_payload, m_payloadCounter);   
    }
}

/*
 * Post the JSON forfully
 * 
 */
int IURawDataHelper:: publishJSON(MacAddress macAddress,char* value, uint16_t valueLength){

  if(WiFi.status()== WL_CONNECTED){   //Check WiFi connection status
 
   HTTPClient http;    //Declare object of class HTTPClient
 
   http.begin("http://115.112.92.146:58888/contineonx-web-admin/imiot-infiniteuptime-api/postdatadump");      //Specify request destination
   http.addHeader("Content-Type", "text/plain");  //Specify content-type header
 
   int httpCode = http.POST(value); //+ macAddress.toString().c_str() );   //Send the request
   String payload = http.getString();                  //Get the response payload
 
   //Serial.println(httpCode);   //Print HTTP return code
   //Serial.println(payload);    //Print request response payload
 
   http.end();  //Close connection
   return 111;
 }else{

    return 222;
    debugPrint("Error in WiFi connection");   
 
 }
 
}

/* =============================================================================
    HTTP GET request
============================================================================= */

/**
 * Post the payload if ready
 */
String IURawDataHelper::publishConfigMessage(MacAddress macAddress)
{
   
    String httpResponseMessage = httpGetPayload(macAddress);
    if (debugMode)
    {
        debugPrint("Get the Pending http messages: ", false);
        debugPrint(httpResponseMessage);
    }

    return httpResponseMessage;
}

/**
 * GET the payload
 */
String IURawDataHelper::httpGetPayload(MacAddress macAddress)
{
    if (debugMode)
    {
        debugPrint("Get payload: ", false);
        debugPrint(m_payload);
    }
    char fullUrl[200];//strlen(m_endpointRoute) + 18];
    String url;
    url = String(m_endpointHost) + String(":")+ String(m_endpointPort) + "/iu-web/iu-infiniteuptime-api/getpendingdeviceconfig?mac=";
 
    //strcpy(fullUrl,m_endpointHost);
    //strcat(fullUrl,":");
    //strcat(fullUrl,String(m_endpointPort.c_str()) );
    //strcat(fullUrl,"/iu-web/iu-infiniteuptime-api/getpendingdeviceconfig?mac=");
    
    strcpy(fullUrl,url.c_str() );//"http://13.232.122.10:8080/iu-web/iu-infiniteuptime-api/getpendingdeviceconfig?mac=");
    //char* fullUrl = "http://13.232.122.10:8080/iu-web/iu-infiniteuptime-api/getpendingdeviceconfig?mac=94:54:93:43:25:1C";
    strncat(fullUrl,macAddress.toString().c_str(), 18);
    if (debugMode)
    {
        debugPrint("Host: ", false);
        debugPrint(m_endpointHost);
        debugPrint("Port: ", false);
        debugPrint(m_endpointPort);
        debugPrint("URL: ", false);
        //debugPrint(fullUrl);
    }
    return httpGET(fullUrl,0,NULL); // 0 -response length
    
}



/*
 * Publish big JSON
 * 
 */
/*void IURawDataHelper:: publishBigJSON(char* values){


  int contentLength = values.length();

  // create the request and headers
  String request = "POST " + String(endpointURL) + " HTTP/1.1\r\n" +
  "Host: " + String(endpointHost) + "\r\n" + 
  "Accept: application/json" + "\r\n" + 
  "Content-Type: application/json\r\n" +
  "Content-Length: " + String(contentLength) + "\r\n\r\n";

  // Use WiFiClient class to create TCP connections
  WiFiClient client;

  connectResult = client.connect(endpointHost, atoi(endpointPort));

  // This will send the request and headers to the server
  client.print(request);

  // now we need to chunk the payload into 1000 byte chunks
  int cIndex;
  for (cIndex = 0; cIndex < contentLength; cIndex = cIndex + 1000) {
    client.print(jsonPayload.substring(cIndex, cIndex+1000));
    //Serial.print(jsonPayload.substring(cIndex, cIndex+1000));
  }
  client.print(jsonPayload.substring(cIndex));
  //Serial.println(jsonPayload.substring(cIndex));


}
*/
