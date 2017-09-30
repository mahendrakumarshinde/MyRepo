#include "IUBMX055Gyro.h"

using namespace IUComponent;


/* =============================================================================
    Constructors and destructors
============================================================================= */

IUBMX055Gyro::IUBMX055Gyro(IUI2C *iuI2C, uint8_t id, FeatureBuffer *tiltX,
                           FeatureBuffer *tiltY, FeatureBuffer *tiltZ);
    Sensor(id, 3, tiltX, tiltY, tiltZ),
    m_resolution(0)
{
    m_iuI2C = iuI2C;
    if (!m_iuI2C->checkComponentWhoAmI("BMX055 Gyro", ADDRESS, WHO_AM_I, I_AM))
    {
        m_iuI2C->setErrorMessage("GYRERR");
        return;
    }
    m_samplingRate = defaultSamplingRate;
    softReset();
    setScale(defaultScale);
    setBandwidth(defaultBandwidth);
    setSamplingRate(defaultSamplingRate);
}


/* =============================================================================
    Hardware & power management
============================================================================= */

/**
 * Soft reset gyroscope and return to normal power mode
 */
void IUBMX055Gyro::softReset()
{
    m_iuI2C->writeByte(ADDRESS,  BGW_SOFTRESET, 0xB6);
    delay(15);
}

/**
 * Set the power mode to ACTIVE (5mA)
 *
 * This is the default mode after POR (Power-On Reset)
 */
void IUBMX055Gyro::wakeUp()
{
    Sensor::wakeUp();
    // Write {0, 0} to {suspend (bit7), deep-suspend (bit5)}
    m_iuI2C->writeByte(ADDRESS,  LPM1, 0x00);
}

/**
 * Set the power mode to SLEEP (25μA)
 *
 * This implementation SLEEP mode corresponds to the gyroscope 'suspend' mode.
 */
void IUBMX055Gyro::sleep()
{
    Sensor::sleep();
    // Write {1, 0} to {suspend (bit7), deep-suspend (bit5)}
    m_iuI2C->writeByte(ADDRESS,  LPM1, 0x80);
}

/**
 * Set the power mode to SUSPEND (<5μA)
 *
 * This implementation SUSPEND mode corresponds to the gyroscope 'deep-suspend'
 * mode.
 * Note that, in SUSPEND mode, configuration values are lost, they need to be
 * re-set when exiting the SUSPEND mode.
 */
void IUBMX055Gyro::suspend()
{
    Sensor::suspend();
    // Write {0, 1} to {suspend (bit7), deep-suspend (bit5)}
    m_iuI2C->writeByte(ADDRESS,  LPM1, 0x20);
}


/* =============================================================================
    Configuration and calibration
============================================================================= */

/**
 * Enable Fast Power Up option of SLEEP mode (2.5mA)
 *
 * Fast power up, when enabled, modifies the SLEEP mode. Fast Power Up must be
 * enabled before calling sleep().
 */
void IUBMX055Gyro::enableFastPowerUp()
{
    m_fastPowerUpEnabled = true;
    sleep();
}

/**
 * Disable Fast Power Up option of SLEEP mode (classic SLEEP MODE is then used)
 *
 * Fast Power Up is disabled at POR (Power-On Reset)
 */
void IUBMX055Gyro::disableFastPowerUp()
{
    m_fastPowerUpEnabled = false;
    sleep();
}

/**
 * Set the scale then recompute resolution
 */
void IUBMX055Gyro::setScale(IUBMX055Gyro::scaleOption scale)
{
    m_scale = scale;
    m_iuI2C->writeByte(ADDRESS, RANGE, m_scale);
    computeResolution();
}

/**
 * Set the accelerometer bandwidth
 */
void IUBMX055Gyro::setBandwidth(IUBMX055Gyro::bandwidthOption bandwidth)
{
    m_bandwidth = bandwidth;
    m_iuI2C->writeByte(ADDRESS, BW, m_bandwidth);
}


/* =============================================================================
    Data Acquisition
============================================================================= */

/**
 * Compute the gyroscope resolution based on the full scale range
 */
void IUBMX055Gyro::computeResolution()
{
    //TODO Implement
    m_resolution = 0;
}


/* =============================================================================
    Communication
============================================================================= */



/* =============================================================================
    Data Acquisition
============================================================================= */



/* =============================================================================
    Debugging
============================================================================= */

