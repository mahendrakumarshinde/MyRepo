#include "IUESP8285.h"

/* =============================================================================
    Constructor & desctructors
============================================================================= */

IUESP8285::IUESP8285(HardwareSerial *serialPort, char *charBuffer,
                     uint16_t bufferSize,  uint32_t rate,
                     uint16_t dataReceptionTimeout) :
    IUSerial(StreamingMode::WIFI, serialPort, charBuffer, bufferSize, rate,
             ';', dataReceptionTimeout)
{
}


/* =============================================================================
    Hardware & power management
============================================================================= */

/**
 * Set up the component and finalize the object initialization
 */
void IUESP8285::setupHardware()
{
    // Check if UART is being driven by something else (eg: FTDI connnector)
    // used to flash the ESP8285
    // TODO Test the following
//    pinMode(UART_TX_PIN, INPUT_PULLDOWN);
//    delay(100);
//    if(digitalRead(UART_TX_PIN))
//    {
//        // UART driven by something else
//        // TODO How does this behave when there is a battery?
//        IUSerial::suspend();
//        return;
//    }
    IUSerial::setupHardware();
    delay(10);
    port->print("WIFI-HARDRESET;");
}

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

char iuWiFiBuffer[500] = "";
IUESP8285 iuWiFi(&Serial3, iuWiFiBuffer, 500, 115200, 2000);
