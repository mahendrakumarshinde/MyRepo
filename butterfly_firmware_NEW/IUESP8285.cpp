#include "IUESP8285.h"


/* =============================================================================
    Constructor & desctructors
============================================================================= */

IUESP8285::IUESP8285(HardwareSerial *serialPort, uint32_t rate,
                     uint16_t dataReceptionTimeout) :
    IUSerial(StreamingMode::WIFI, serialPort, rate, 20, ';',
             dataReceptionTimeout)
{
}


/* =============================================================================
    Hardware & power management
============================================================================= */

/**
 * Switch to ACTIVE power mode
 */
void IUESP8285::wakeUp()
{
    // TODO Implement power mode change at EESP8285 level
    IUSerial::wakeUp();
}

/**
 * Switch to SLEEP power mode
 */
void IUESP8285::sleep()
{
    // TODO Implement power mode change at EESP8285 level
    IUSerial::sleep();
    // Passing 0 makes the WiFi sleep indefinitly
    //WiFiSerial::Put_WiFi_To_Sleep(0.0f);
}

/**
 * Switch to SUSPEND power mode
 */
void IUESP8285::suspend()
{
    // TODO Implement power mode change at EESP8285 level
    IUSerial::suspend();
    // Passing 0 makes the WiFi sleep indefinitly
    //WiFiSerial::Put_WiFi_To_Sleep(0.0f);
}


/* =============================================================================
    Communication with WiFi chip
============================================================================= */

/**
 * Print the given MAC address to ESP8285 UART (with header)
 */
void IUESP8285::sendBleMacAddress(char *macAddress)
{
    port->print("BLEMAC-");
    port->print(macAddress);
    port->print(";");
}


/* =============================================================================
    Instanciation
============================================================================= */

IUESP8285 iuWiFi(&Serial1, 57600, 2000);
