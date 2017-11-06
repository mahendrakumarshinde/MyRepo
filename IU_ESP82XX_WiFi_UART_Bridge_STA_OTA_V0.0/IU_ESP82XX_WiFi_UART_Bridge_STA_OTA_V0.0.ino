/* ESP82XX STA WiFi/UART Bridge and OTA update example code
   by: Greg Tomasch
   date: May 8, 2017
   license: Beerware - Use this code however you'd like. If you 
   find it useful you can buy me a beer some time.
*/

#include <ESP8266WiFi.h>
#include <ESP8266mDNS.h>
#include <WiFiUdp.h>
#include "WiFi_UDP.h"
#include "Bridge_WiFiserial.h"
#include "config.h"

extern "C"
{
  #include "user_interface.h"
  bool wifi_set_sleep_type(sleep_type_t);
  sleep_type_t wifi_get_sleep_type(void);
}

/************* Global variables *************/
String      ssid_string;
char        ssid[32];
char        ssidtest[17];
uint8_t     dongle_found = 0;
uint8_t     connect_success = 0;
uint16_t    updateSize = 0;
uint16_t    noBytes;
byte        packetBuffer[4*UPDATE_SIZE];
uint32_t    Timestamp;
uint8_t     unity = 1;
float       data_update[UPDATE_SIZE];
uint32_t    currentTime = 0;
uint32_t    previousTime = 0;

// OTA specific
uint8_t     ack_nak_byte;                                                 // Flow control byte; Prnding = 2, OTA update request ACK = 4, data block ACK = 1, data block resend = 3, NAK = 0
uint8_t     OTA_bytes[258];                                               // Array for holding the uint16_t (2-byte) block number and the data payload up to 256-bytes

void setup()
{
  WiFiSerial::SerialOpen(0, 115200);
  delay(1000);
  Serial.setRxBufferSize(1024);                                           // Configure the ESP82XX UART Rx bufer to a generous size so it does not overflow
  WiFi_UDP::WiFi_UDP_init();
  EEPROM.begin(2048);
  
  // Enable Light sleep mode
  wifi_set_sleep_type(LIGHT_SLEEP_T);

}

void loop()
{
  WiFiSerial::serialCom();
  WiFi_UDP::Handle_OTA();
  currentTime = micros();
  if((currentTime - previousTime) > BRIDGE_STAT_UPDATE_TARGET)
  {
    // If connected to the AP, tell the host MCU you're open for business
    if(WiFi.status() == WL_CONNECTED)
    {
      WiFiSerial::WiFi_Bridge_Ready(1);
    } else
    {
      WiFiSerial::WiFi_Bridge_Ready(0);
    }
    previousTime = currentTime;
  }
}
