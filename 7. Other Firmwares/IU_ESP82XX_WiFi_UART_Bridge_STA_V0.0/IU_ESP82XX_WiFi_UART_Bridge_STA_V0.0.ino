#include <ESP8266WiFi.h>
#include <ESP8266mDNS.h>
#include <WiFiUdp.h>
#include <ArduinoOTA.h>
#include "WiFi_UDP.h"
#include "Bridge_WiFiserial.h"
#include "config.h"

extern "C"
{
  #include "user_interface.h"
  bool wifi_set_sleep_type(sleep_type_t);
  sleep_type_t wifi_get_sleep_type(void);
}

// Global variables
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

void setup()
{
  WiFiSerial::SerialOpen(0, 115200);
  delay(1000);
  WiFi_UDP::WiFi_UDP_init();
  EEPROM.begin(2048);
  
  // Enable Light sleep mode
  wifi_set_sleep_type(LIGHT_SLEEP_T);

}

void loop()
{
  WiFiSerial::serialCom();
  currentTime = micros();
  if((currentTime - previousTime) > BRIDGE_STAT_UPDATE_TARGET)
  {
    // Tell the host MCU you're open for business
    WiFiSerial::WiFi_Bridge_Ready();
    previousTime = currentTime;
  }
}
