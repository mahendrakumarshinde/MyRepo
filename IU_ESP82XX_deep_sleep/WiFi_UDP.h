#ifndef WiFi_UDP_h
#define WiFi_UDP_h

#include <ESP8266WiFi.h>
#include <WiFiUdp.h>
#include "Bridge_WiFiserial.h"
#include <EEPROM.h>
#include "config.h"

#define WIFI_LED_PIN                     15
#define WIFI_LINK_PINMODE                pinMode(WIFI_LED_PIN, OUTPUT);
#define WIFI_LINK_LED_ON                 digitalWrite(WIFI_LED_PIN, HIGH);
#define WIFI_LINK_LED_OFF                digitalWrite(WIFI_LED_PIN, LOW);
#define WIFI_LED_TOGGLE                  if(digitalRead(WIFI_LED_PIN)) digitalWrite(WIFI_LED_PIN, LOW); else digitalWrite(WIFI_LED_PIN, HIGH);


extern float                             data_update[UPDATE_SIZE];
extern uint32_t                          currentTime;
extern char                              ssid[32];
extern uint8_t                           dongle_found;
extern uint8_t                           connect_success;
extern String                            ssid_string;
extern char                              ssidtest[17];
extern uint16_t                          updateSize;
extern const char*                       password;

class WiFi_UDP
{
  public:
                                         WiFi_UDP();
    static void                          WiFi_UDP_init();
    static void                          Handle_Client();
  private:
    static void                          setupWiFi();
};

#endif // WiFi_UDP_h
