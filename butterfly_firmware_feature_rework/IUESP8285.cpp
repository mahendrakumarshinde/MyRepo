#include "IUESP8285.h"

using namespace IUComponent;

/* =============================================================================
    Constructor & desctructors
============================================================================= */

IUESP8285::IUESP8285(IUI2C *iuI2C) :
    ABCComponent(),
    m_iuI2C(iuI2C)
{
    wakeUp();
}


/* =============================================================================
    Hardware & power management
============================================================================= */

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
    WiFi Communication
============================================================================= */

void IUESP8285::setBaudRate(uint32_t baudRate)
{
    m_baudRate = baudRate;
    port->flush();
    delay(2);
    port->end();
    delay(10);
    port->begin(m_baudRate);
}


/* =============================================================================
    Instanciation
============================================================================= */

IUESP8285 iuWiFi(&iuI2C);
