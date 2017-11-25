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
 * Set up the component and finalize the object initialization
 */
void IUESP8285::setupHardware()
{
    IUSerial::setupHardware();
}

/**
 * Switch to ACTIVE power mode
 */
void IUESP8285::wakeUp()
{
    m_powerMode = PowerMode::ACTIVE;
}

/**
 * Switch to SLEEP power mode
 */
void IUESP8285::sleep()
{
    m_powerMode = PowerMode::SLEEP;
    // Passing 0 makes the WiFi sleep indefinitly
    //WiFiSerial::Put_WiFi_To_Sleep(0.0f);
}

/**
 * Switch to SUSPEND power mode
 */
void IUESP8285::suspend()
{
    m_powerMode = PowerMode::SUSPEND;
    // Passing 0 makes the WiFi sleep indefinitly
    //WiFiSerial::Put_WiFi_To_Sleep(0.0f);
}


/* =============================================================================
    WiFi communication
============================================================================= */



/* =============================================================================
    Instanciation
============================================================================= */

IUESP8285 iuWiFi(&Serial1, 57600, 2000);
