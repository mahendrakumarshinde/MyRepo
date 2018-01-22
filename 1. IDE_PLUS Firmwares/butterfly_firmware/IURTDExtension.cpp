#include "IURTDExtension.h"

#ifdef RTD_DAUGHTER_BOARD // Optionnal hardware

/* =============================================================================
    Constructors and destructors
============================================================================= */

IURTDExtension::IURTDExtension(IUI2C *iuI2C, const char* name,
                               Feature *temperatures, uint8_t rtdCount) :
    LowFreqSensor(name, 1, temperatures),
    m_rtdCount(rtdCount)
{
    m_iuI2C = iuI2C;
}


/* =============================================================================
    Hardware & power management
============================================================================= */

/**
 * Set up the component and finalize the object initialization
 */
void IURTDExtension::setupHardware()
{
    // Set the PCA9534 pins to outputs
    writeToExpanderRegister(PCA9435_CONFIGURATION, 0x00);
//    Wire.beginTransmission(ADDRESS);
//    Wire.write(0x03);            // control register
//    Wire.write(0x00);            // all pins output
//    Wire.endTransmission();
}


/* =============================================================================
    Configuration and calibration
============================================================================= */


/* =============================================================================
    Data acquisition
============================================================================= */

/**
 * Acquire new data, while handling sampling period
 */
void IURTDExtension::acquireData(bool inCallback, bool force)
{
    LowFreqSensor::acquireData(inCallback, force);
}

/**
 * Read sensor data
 */
void IURTDExtension::readData()
{
    for (uint8_t i = 0; i < m_rtdCount; ++i)
    {
        m_destinations[0]->addFloatValue(0);
    }
}


/* =============================================================================
    Communication
============================================================================= */


/* =============================================================================
    Debugging
============================================================================= */

/**
 *
 */
void IURTDExtension::exposeCalibration()
{

}


/* =============================================================================
    GPIO Expander Communication
============================================================================= */

/**
 *
 */
bool IURTDExtension::writeToExpanderRegister(uint8_t exRegister,
                                             uint8_t data)
{
    return m_iuI2C->writeByte(ADDRESS, exRegister, data);
}

/**
 *
 */
bool IURTDExtension::readFromExpanderRegister(uint8_t exRegister,
                                              uint8_t *destination,
                                              uint8_t byteCount)
{
//    return m_iuI2C->readBy/tes(ADDRESS, exRegister, byteCount, destination);
    // FIXME above line returns the error "stray '\33' in program"
    return true;
}


/* =============================================================================
    Instanciation
============================================================================= */

extern IURTDExtension iuRTDExtension(&iuI2C, "RTD", &rtdTemp);

#endif // RTD_DAUGHTER_BOARD
