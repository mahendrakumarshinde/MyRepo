#include "Arduino.h"
#include "WiFi_UDP.h"

static byte              packet_byte[1];
static IPAddress         txip;
static IPAddress         ipServer(192,168,4,1);
static IPAddress         IP_CLIENT;
static IPAddress         Subnet(255,255,255,0);
const char*              password = "startnow";
const uint16_t           rxport = 9047;
const uint16_t           txport = TX_PORT;
const uint16_t           OTA_TX_port = OTA_TX_PORT;
const uint16_t           OTA_RX_port = OTA_RX_PORT;

// Two UDP instances needed for Tx anRx ports
WiFiUDP                  tx_udp;
WiFiUDP                  rx_udp;
WiFiUDP                  OTA_rx_udp;

WiFi_UDP::WiFi_UDP()
{
}

/**
* @fn: WiFi_UDP_init()
*
* @brief: Sets up WiFi in STA mode and instatiates UDP-related classes
* 
* @params:
* @returns:
*/
void WiFi_UDP::WiFi_UDP_init()
{
  WiFi_UDP::setupWiFi();
  rx_udp.begin(rxport);
  OTA_rx_udp.begin(OTA_RX_port);
}

/**
* @fn: Handle_Client()
*
* @brief: Loads general update data from data arrays, builds and sends UDP data update packet to the AP node
* 
* @params:
* @returns:
*/
void WiFi_UDP::Handle_Client()
{
  uint8_t noBytes;
  union
  {
    float    float_data;
    uint8_t  float_data_byte[4];
  };
  union
  {
    int32_t  long_signed;
    uint8_t  long_signed_byte[4];
  };
  union
  {
    uint32_t long_unsigned;
    uint8_t  long_unsigned_byte[4];
  };
  
  // Construct/send the UPD packet
  txip = ipServer;
  tx_udp.beginPacket(txip, txport);
  for(uint8_t i=0; i < UPDATE_SIZE; i++)
  {
    float_data = data_update[i];
    for(uint8_t j=0; j<4; j++)
    {
      tx_udp.write(float_data_byte[j]);
    }
  }
  tx_udp.endPacket();
}

/**
* @fn: XMIT_OTA_ACK_NAK()
*
* @brief: Constructs OTA update ACK/NAK response into a UDP packet and sends to the AP node
* 
* @params:
* @returns:
*/
void WiFi_UDP::XMIT_OTA_ACK_NAK()
{
  // Construct/send the UPD ACK/NAK packet
  txip = ipServer;
  tx_udp.beginPacket(txip, OTA_TX_port);
  tx_udp.write(ack_nak_byte);
  tx_udp.endPacket();
}

/**
* @fn: Handle_OTA()
*
* @brief: Receives and decodes OTA request and data packets coming in from the AP node
* 
* @params:
* @returns:
*/
void WiFi_UDP::Handle_OTA()
{
  uint16_t noBytes;
  
  // Check the local Rx port to see if there is an update packet of the correct size; 6 for OTA initiate, 130 for FW data block
  noBytes = OTA_rx_udp.parsePacket();
  if(noBytes == 6)
  {
    OTA_rx_udp.read(OTA_bytes, noBytes);
    WiFiSerial::Initiate_OTA_Update();   
  }
  if(noBytes == 130)
  {
    OTA_rx_udp.read(OTA_bytes, noBytes);
    WiFiSerial::Push_OTA_Data();
  }
}

/**
* @fn: setupWiFi()
*
* @brief: Identifies the semi-uniuqe AP name and configures the ESP8285 as a WiFi client
* 
* @params:
* @returns:
*/
void WiFi_UDP::setupWiFi()
{
  WiFi.mode(WIFI_STA);
  String wap_check;

  // Check the EEPROM for the last dongle connected
  for (int i = 0; i < 15; ++i)
  {
    wap_check += char(EEPROM.read(i));
  }

  // If the first 16bytes of the EEPROM WAP space is "ESP8266 Inf_Upt"
  // Read the full WAP SSID, try connecting to that dongle first
  if(wap_check == "ESP8266 Inf_Upt")
  {
    for (int i = 0; i < 32; ++i)
    {
      ssid[i] = char(EEPROM.read(i));
    }
    dongle_found = 1;
  }

  // Now Connect to the dongle
  while(connect_success == 0)
  {
    // If the dongle SSID is unknown or connect times out, scan available networks
    while(dongle_found == 0)
    {
      WIFI_LINK_LED_ON;
      delay(100);
      #ifdef SERIAL_DEBUG
        Serial.println("scan start");
      #endif
      // WiFi.scanNetworks will return the number of networks found
      int n = WiFi.scanNetworks();
      WIFI_LINK_LED_OFF;
      #ifdef SERIAL_DEBUG
        Serial.println("scan done");
      #endif
      if (n == 0)
      {
        #ifdef SERIAL_DEBUG
          Serial.println("no networks found");
        #endif
      } else
      {
        #ifdef SERIAL_DEBUG
          Serial.print(n);
          Serial.println(" networks found");
        #endif
        for (int i = 0; i < n; ++i)
        {
          // Print SSID and RSSI for each network found
          #ifdef SERIAL_DEBUG
            Serial.print(i + 1);
            Serial.print(": ");
          #endif
          ssid_string = String(WiFi.SSID(i));
          ssid_string.toCharArray(ssidtest,16);
          if(String(ssidtest) == "ESP8266 Inf_Upt")
          {
            ssid_string.toCharArray(ssid,32);
            if(String(ssidtest) == "ESP8266 Inf_Upt") updateSize = UPDATE_SIZE;
            dongle_found = 1;
          }
          #ifdef SERIAL_DEBUG
            Serial.print(ssid_string);
            Serial.print(" (");
            Serial.print(WiFi.RSSI(i));
            Serial.print(")");
            Serial.println((WiFi.encryptionType(i) == ENC_TYPE_NONE)?" ":"*");
          #endif
          delay(10);
        }
      }
    }
    #ifdef SERIAL_DEBUG
      Serial.println();
      Serial.println();
      Serial.print("Connecting to ");
      Serial.println(ssid);
    #endif

    // Try connecting to the allowed SSID
    WiFi.disconnect();
    delay(1000);
    WiFi.mode(WIFI_STA);
    WiFi.begin(ssid, password);
    WiFi.config(ipClient, ipServer, Subnet);
    uint8_t j = 0;
    while(1)
    {
      if(WiFi.status() == WL_CONNECTED)
      {
        // If connects, break out of the loop successful
        connect_success = 1;
        break;
      }
      if(j > 19)
      {
        // If times out break out of loop unsuccessful and go scan available networks
        #ifdef SERIAL_DEBUG
          Serial.println();
        #endif
        connect_success = 0;
        dongle_found = 0;
        break;
      }
      WIFI_LED_TOGGLE;
      delay(500);
      #ifdef SERIAL_DEBUG
        Serial.print(".");
      #endif
      j++;
    }
  }
  WIFI_LINK_LED_OFF;
  #ifdef SERIAL_DEBUG
    Serial.println("");
    Serial.println("WiFi connected");  
    Serial.println("IP address: ");
    Serial.println(WiFi.localIP());
  #endif

  // If you're here, you've successfully connected; save the SSID to EEPROM
  for (int i = 0; i < 32; ++i)
  {
    EEPROM.write(i, ssid[i]);
  }
  EEPROM.commit();
}
