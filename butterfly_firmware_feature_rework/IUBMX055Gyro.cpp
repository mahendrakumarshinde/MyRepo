#include "IUBMX055Gyro.h"


/* =============================================================================
    Constructors and destructors
============================================================================= */

IUBMX055Gyro::IUBMX055Gyro(IUI2C *iuI2C, const char* name, Feature *tiltX,
                           Feature *tiltY, Feature *tiltZ) :
    AsynchronousSensor(name, 3, tiltX, tiltY, tiltZ)
{
    m_iuI2C = iuI2C;
    // Fast Power Up is disabled at POR (Power-On Reset)
    m_fastPowerUpEnabled = false;
}


/* =============================================================================
    Hardware & power management
============================================================================= */

/**
 * Set up the component and finalize the object initialization
 */
void IUBMX055Gyro::setupHardware()
{
    if (!m_iuI2C->checkComponentWhoAmI("BMX055 Gyro", ADDRESS, WHO_AM_I, I_AM))
    {
        if (debugMode)
        {
            debugPrint("GYROERR");
        }
        return;
    }
    softReset();
    setScale(defaultScale);
    setBandwidth(defaultBandwidth);
    setSamplingRate(defaultSamplingRate);
}

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
    AsynchronousSensor::wakeUp();
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
    AsynchronousSensor::sleep();
    m_fastPowerUpEnabled = true;
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
    AsynchronousSensor::suspend();
    m_fastPowerUpEnabled = false;
    // Write {0, 1} to {suspend (bit7), deep-suspend (bit5)}
    m_iuI2C->writeByte(ADDRESS,  LPM1, 0x20);
}


/* =============================================================================
    Configuration and calibration
============================================================================= */

bool IUBMX055Gyro::configure(JsonVariant &config)
{
    JsonVariant my_config = config[m_name];
    if (!my_config)
    {
        return false;
    }
    // Sampling Rate and bandwidth
    JsonVariant value = my_config["FREQ"];
    if (value.success())
    {
        setBandwidth((bandwidthOption) (value.as<int>()));
    }
    // Full Scale Range
    value = my_config["FSR"];
    if (value.success())
    {
        setScale((scaleOption) (value.as<int>()));
    }
    return true;
}

/**
 * Set the scale then recompute resolution
 */
void IUBMX055Gyro::setScale(IUBMX055Gyro::scaleOption scale)
{
    m_scale = scale;
    m_iuI2C->writeByte(ADDRESS, RANGE, m_scale);


    //TODO Implement
    m_resolution = 0;


}

/**
 * Set the accelerometer bandwidth
 */
void IUBMX055Gyro::setBandwidth(IUBMX055Gyro::bandwidthOption bandwidth)
{
    m_bandwidth = bandwidth;
    m_iuI2C->writeByte(ADDRESS, BW, m_bandwidth);
    switch (bandwidth)
    {
    case BW_2000Hz523Hz:
        setSamplingRate(2000);
        break;
    case BW_2000Hz230Hz:
        setSamplingRate(2000);
        break;
    case BW_1000Hz116Hz:
        setSamplingRate(1000);
        break;
    case BW_400Hz47Hz:
        setSamplingRate(400);
        break;
    case BW_200Hz23Hz:
        setSamplingRate(200);
        break;
    case BW_100Hz12Hz:
        setSamplingRate(100);
        break;
    case BW_200Hz64Hz:
        setSamplingRate(200);
        break;
    case BW_100Hz32Hz:
        setSamplingRate(100);
        break;
    }
}


/* =============================================================================
    Data Acquisition
============================================================================= */


/* =============================================================================
    Communication
============================================================================= */



/* =============================================================================
    Data Acquisition
============================================================================= */



/* =============================================================================
    Debugging
============================================================================= */

