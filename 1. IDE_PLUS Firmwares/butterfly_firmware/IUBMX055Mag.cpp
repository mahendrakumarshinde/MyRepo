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
    DrivenSensor(name, 3, magneticX, magneticY, magneticZ),
    m_forcedMode(false),
    m_odr(defaultODR),
    m_accuracy(defaultAccuracy)
{
    m_iuI2C = iuI2C;
}


/* =============================================================================
    Hardware & power management
============================================================================= */

/**
 * Set up the component and finalize the object initialization
 *
 * By default, this
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
}

/**
 * Soft reset - Soft reset always bring the magnetometer into sleep mode
 */
void IUBMX055Mag::softReset()
{
    // Write 1 to both bit7 and bit1 from power control byte 0x4B
    m_iuI2C->writeByte(ADDRESS, PWR_CNTL1, 0x82);
    delay(1000);
    // Reflect the actual state of the sensor
    DrivenSensor::lowPower();
    m_forcedMode = true;
    setAccuracy(m_accuracy);
    setODR(defaultODR);
}


/**
 * Set the power mode to ACTIVE (between 170μA and 4.9mA).
 *
 * After waking up, the Magnetometer will be in Forced Mode by default.
 */
void IUBMX055Mag::wakeUp()
{
    if (m_powerMode == PowerMode::SUSPEND)
    {
        m_iuI2C->writeByte(ADDRESS, PWR_CNTL1, 0x01);
        delay(100);
        // When exiting suspend mode, need to rewrite configurations
        setAccuracy(m_accuracy);
    }
    enterForcedMode();
    DrivenSensor::wakeUp();
}

/**
 * Set the power mode to ECONOMY
 *
 * The registers can be read but no data acquisition can be performed.
 */
void IUBMX055Mag::lowPower()
{
    // When exiting suspend mode, need to rewrite configurations
    if (m_powerMode == PowerMode::SUSPEND)
    {
        m_iuI2C->writeByte(ADDRESS, PWR_CNTL1, 0x01);
        setAccuracy(m_accuracy);
    }
    m_iuI2C->writeByte(ADDRESS, PWR_CNTL2, 0x06);
    m_forcedMode = true;  // Reflect the actual state of the sensor
    DrivenSensor::lowPower();
}

/**
 * Set the power mode to SUSPEND (11μA)
 *
 * This is the default after Power-On Reset
 */
void IUBMX055Mag::suspend()
{
    m_iuI2C->writeByte(ADDRESS, PWR_CNTL1, 0x00);
    DrivenSensor::suspend();
}


/* =============================================================================
    Configuration and calibration
============================================================================= */

void IUBMX055Mag::configure(JsonVariant &config)
{
    JsonVariant value = config["FREQ"];  // ODR
    if (value.success())
    {
        setODR((ODROption) (value.as<int>()));
    }
    value = config["ACY"];  // Accuracy Preset
    if (value.success())
    {
        setAccuracy((accuracyPreset) (value.as<int>()));
    }
}

/**
 * Enter Forced Mode => on demand data acquisition
 */
void IUBMX055Mag::enterForcedMode()
{
    m_forcedMode = true;
    m_iuI2C->writeByte(ADDRESS, PWR_CNTL2, 0x02);
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
        debugPrint("Set Magnetometer ODR but Forced Mode is active: ODR will "
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
