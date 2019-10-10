#include "IUTMP116.h"
#define TEMP_COEFFICIENT 0.0078125

/* =============================================================================
    Constructors and destructors
============================================================================= */


IUTMP116::IUTMP116(IUI2C *iuI2C, const char* name,
                   void (*i2cReadCallback)(uint8_t wireStatus),
                   FeatureTemplate<float> *temperature) :
    LowFreqSensor(name, 1, temperature),
    m_temperature(30.0),
    m_readCallback(i2cReadCallback)
{
    m_iuI2C = iuI2C;
}

/* =============================================================================
    Hardware & power management
============================================================================= */
/**
 * Set up the component and finalize the object initialization
 */

void IUTMP116::setupHardware()
{
    Serial.println("Setting up TMP116 !");
#if 0
    if(!m_iuI2C->checkComponentWhoAmI("TMP116", ADDRESS, DEVICE_ID,I_AM))
    {
        if (debugMode)
        {
            debugPrint("TMPERR");
        }
        return;
    }
#endif
    if(ADDRESS != I_AM)
    {
        if (debugMode)
        {
            debugPrint("TMPERR");
        }
        return;
        Serial.println("Error Setting up TMP116 !");
    }
    m_iuI2C->writeBytes(ADDRESS, CFGR,0x02,0x20);
    delay(10);
    m_iuI2C->writeBytes(ADDRESS, HIGH_LIM,0x60,0x00);
    delay(10);
    m_iuI2C->writeBytes(ADDRESS, LOW_LIM,0x00,0x00);
    delay(10);
    setPowerMode(PowerMode::REGULAR);
    setTempHighLimit();
    setTempLowLimit();
}


void IUTMP116::setTempHighLimit()
{

}

void IUTMP116::setTempLowLimit()
{

}
/**
 * Manage component power modes
 */

void IUTMP116::setPowerMode(PowerMode::option pMode)
{
    m_powerMode = pMode;
    // TODO Implement
    switch (m_powerMode)
    {
        case PowerMode::PERFORMANCE:
        case PowerMode::ENHANCED:
        case PowerMode::REGULAR:
        case PowerMode::LOW_1:
        case PowerMode::LOW_2:
        case PowerMode::SLEEP:
        case PowerMode::DEEP_SLEEP:
        case PowerMode::SUSPEND:
            // TODO: Implement
            break;
        default:
            if (debugMode)
            {
                debugPrint(F("Unhandled power Mode "), false);
                debugPrint(m_powerMode);
            }
    }
}
/***** Temperature Reading *****/
void IUTMP116::readData()
{
    m_rawBytes[0] = 0;
    m_rawBytes[1] = 0;
    if (!m_iuI2C->readBytes(ADDRESS, TEMP, 2, &m_rawBytes[0],m_readCallback))
    {
        if (asyncDebugMode) 
        {
            debugPrint("Temperature Read Failure");
        }
    }
}

/**
 * Process a raw Temperature reading
 */
void IUTMP116::processTemperatureData(uint8_t wireStatus)
{
    iuI2C.releaseReadLock();
    if (wireStatus != 0) {
        if (asyncDebugMode) {
            debugPrint(micros(), false);
            debugPrint(F(" Temperature processing read error "), false);
            debugPrint(wireStatus);
        }
        return;
    }
    m_temperature = 0;
    m_temperature = float(((uint16_t) (m_rawBytes[0] << 8) | (uint16_t) m_rawBytes[1]) * TEMP_COEFFICIENT);
    m_destinations[0]->addValue(m_temperature);
//    Serial.print("TMP116:");
 //   Serial.println(m_temperature);
}

/**
   Temperature data to serial

 * If loopDebugMode is true, will send humand readable content, else nothing.
 */

void IUTMP116::sendData(HardwareSerial *port)
{
    if (loopDebugMode)  // Human readable in the console
    {
        port->print("T: ");
        port->println(m_temperature, 2); // UPTO 2 decimal
    }else {

        float temp;
        byte* data;
        for (uint8_t i = 0; i < 2; ++i) {
            temp = m_temperature;
            data = (byte*) & temp;
            port->write(data, 4);
        }
    }
}

float IUTMP116::getData(HardwareSerial *port)
{
    if (loopDebugMode)  // Human readable in the console
    {
        port->print("T: ");
        port->println(m_temperature, 2); // UPTO 2 decimal
    }
    else 
    {
        return m_temperature;        
    }
}
