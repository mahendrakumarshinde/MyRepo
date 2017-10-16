#include "IUBMX055Mag.h"


/* =============================================================================
    Constructors and destructors
============================================================================= */

/**
 * Initializer the Magnetometer part of the BMX-055
 *
 * NB: The WHO AM I does not work when in SUSPEND mode, so the BMX055
 * magnetometer needs to be soft-reset or waked up first.
 */
IUBMX055Mag::IUBMX055Mag(IUI2C *iuI2C, const char* name, Feature *magneticX,
                         Feature *magneticY, Feature *magneticZ) :
    AsynchronousSensor(name, 3, magneticX, magneticY, magneticZ),
    m_forcedMode(false),
    m_odr(defaultODR),
    m_accuracy(defaultAccuracy)
{
    m_iuI2C = iuI2C;
    m_samplingRate = defaultSamplingRate;
}


/* =============================================================================
    Hardware & power management
============================================================================= */

/**
 * Set up the component and finalize the object initialization
 */
void IUBMX055Mag::setupHardware()
{
    softReset();
    if (!m_iuI2C->checkComponentWhoAmI("BMX055 Mag", ADDRESS, WHO_AM_I, I_AM))
    {
        if (debugMode)
        {
            debugPrint("MAGERR");
        }
        return;
    }
    setODR(m_odr);
    setAccuracy(m_accuracy);
    setSamplingRate(m_samplingRate);
}

/**
 * Soft reset - Soft reset always bring the magnetometer into sleep mode
 */
void IUBMX055Mag::softReset()
{
    // Write 1 to both bit7 and bit1 from power control byte 0x4B
    m_iuI2C->writeByte(ADDRESS, PWR_CNTL1, 0x82);
    delay(100);
}


/**
 * Set the power mode to ACTIVE (between 170μA and 4.9mA)
 *
 * By default, the device will be waked up and put into forced mode (so it
 * will actually be asleep until a record is requested or until exitForcedMode
 * is called).
 */
void IUBMX055Mag::wakeUp()
{
    AsynchronousSensor::wakeUp();
    m_iuI2C->writeByte(ADDRESS, PWR_CNTL1, 0x01);
    exitForcedMode();
}

/**
 * Set the power mode to SLEEP
 *
 * This is the default mode after soft-reset.
 */
void IUBMX055Mag::sleep()
{
    AsynchronousSensor::sleep();
    m_iuI2C->writeByte(ADDRESS, PWR_CNTL1, 0x01);
    m_iuI2C->writeByte(ADDRESS, PWR_CNTL2, ((uint8_t) m_odr << 3) | 0x06);
}

/**
 * Set the power mode to SUSPEND (11μA)
 *
 * This is the default after Power-On Reset
 */
void IUBMX055Mag::suspend()
{
    AsynchronousSensor::suspend();
    m_iuI2C->writeByte(ADDRESS, PWR_CNTL1, 0x00);
}


/* =============================================================================
    Configuration and calibration
============================================================================= */

bool IUBMX055Mag::configure(JsonVariant &config)
{
    JsonVariant my_config = config[m_name];
    if (!my_config)
    {
        return false;
    }
    // ODR
    JsonVariant value = my_config["FREQ"];
    if (value.success())
    {
        setODR((ODROption) (value.as<int>()));
    }
    // Accuracy Preset
    value = my_config["ACY"];
    if (value.success())
    {
        setAccuracy((accuracyPreset) (value.as<int>()));
    }
    return true;
}

/**
 * Enter Forced Mode => on demand data acquisition
 */
void IUBMX055Mag::enterForcedMode()
{
    m_forcedMode = true;
    writeODRRegister();
}

/**
 * Exit Forced Mode and go into Active Mode => records will be sample at set ODR
 *
 * NB: ODR = Output Data Rate
 */
void IUBMX055Mag::exitForcedMode()
{
    m_forcedMode = false;
    writeODRRegister();
}

/**
 * Set the output data rate - only relevant in Normal (non-forced) mode
 */
void IUBMX055Mag::setODR(IUBMX055Mag::ODROption ODR)
{
    m_odr = ODR;
    switch (m_odr)
    {
    case ODR_10Hz:
        setSamplingRate(10);
        break;
    case ODR_2Hz:
        setSamplingRate(2);
        break;
    case ODR_6Hz:
        setSamplingRate(6);
        break;
    case ODR_8Hz:
        setSamplingRate(8);
        break;
    case ODR_15Hz:
        setSamplingRate(15);
        break;
    case ODR_20Hz:
        setSamplingRate(20);
        break;
    case ODR_25Hz:
        setSamplingRate(25);
        break;
    case ODR_30Hz:
        setSamplingRate(30);
        break;
    }
    if (debugMode && m_forcedMode)
    {
        debugPrint("Set Magnetometer ODR but Forced Mode is active: ODR will"
                   "be ignored");
    }
    writeODRRegister();
}

/**
 * Write to the ODR registers (define Force Mode + Output Data Rate)
 */
void IUBMX055Mag::writeODRRegister()
{
    uint8_t opModeBit = (uint8_t) m_forcedMode;
    m_iuI2C->writeByte(ADDRESS, PWR_CNTL2, ((uint8_t) m_odr << 3) | \
        (opModeBit << 1));
}

/**
 * Set the accuracy by changing the oversampling on XY and Z axis
 */
void IUBMX055Mag::setAccuracy(IUBMX055Mag::accuracyPreset accuracy)
{
    m_accuracy = accuracy;
    switch (m_accuracy)
    {
    case accuracyPreset::LOWPOWER:
        m_iuI2C->writeByte(ADDRESS, REP_XY, 0x01);  // 3 repetitions
        m_iuI2C->writeByte(ADDRESS, REP_Z,  0x02);  // 3 repetitions
        break;
    case accuracyPreset::REGULAR:
        m_iuI2C->writeByte(ADDRESS, REP_XY, 0x04);  //  9 repetitions
        m_iuI2C->writeByte(ADDRESS, REP_Z,  0x16);  // 15 repetitions
        break;
    case accuracyPreset::ENHANCEDREGULAR:
        m_iuI2C->writeByte(ADDRESS, REP_XY, 0x07);  // 15 repetitions
        m_iuI2C->writeByte(ADDRESS, REP_Z,  0x22);  // 27 repetitions
        break;
    case accuracyPreset::HIGHACCURACY:
        m_iuI2C->writeByte(ADDRESS, REP_XY, 0x17);  // 47 repetitions
        m_iuI2C->writeByte(ADDRESS, REP_Z,  0x51);  // 83 repetitions
        break;
    }
}


/* =============================================================================
    Data acquisition
============================================================================= */



/* =============================================================================
    Communication
============================================================================= */



/* =============================================================================
    Debugging
============================================================================= */


