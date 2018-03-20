#include "IUBMX055Acc.h"


/* =============================================================================
    Data Acquisition callbacks
============================================================================= */

bool newAccelData = false;

void accelReadCallback(uint8_t wireStatus)
{
    iuI2C.releaseReadLock();
    if (wireStatus == 0)
    {
        newAccelData = true;
    }
    else if (asyncDebugMode)
    {
        debugPrint(micros(), false);
        debugPrint(F(" Acceleration read error "), false);
        debugPrint(wireStatus);
    }
}


/* =============================================================================
    Constructors and destructors
============================================================================= */

/**
 * Initialize and configure the BMX055 for the accelerometer
 */
IUBMX055Acc::IUBMX055Acc(IUI2C *iuI2C, const char* name, Feature *accelerationX,
                         Feature *accelerationY, Feature *accelerationZ) :
    HighFreqSensor(name, 3, accelerationX, accelerationY, accelerationZ),
    m_scale(defaultScale),
    m_bandwidth(defaultBandwidth),
    m_filteredData(false)
{
    m_iuI2C = iuI2C;
}


/* =============================================================================
    Hardware & power management
============================================================================= */

/**
 * Set up the component and finalize the object initialization
 */
void IUBMX055Acc::setupHardware()
{
    if (!m_iuI2C->checkComponentWhoAmI("BMX055 ACC", ADDRESS, WHO_AM_I, I_AM))
    {
        if (debugMode)
        {
            debugPrint("ACCERR");
        }
        return;
    }
    softReset();
    setPowerMode(PowerMode::REGULAR);
    setScale(m_scale);
    setSamplingRate(defaultSamplingRate);
    useUnfilteredData();
    configureInterrupts();
}

/**
 * Reset BMX055 configuration and return to normal power mode
 */
void IUBMX055Acc::softReset()
{
    m_iuI2C->writeByte(ADDRESS, BGW_SOFTRESET, 0xB6);
    // After soft reset, must wait for at least t[up] for all register to reset
    // t[up] < 2ms (see datasheet)
    delay(10);
    m_powerMode = PowerMode::REGULAR;  // Default after soft-reset
}

/**
 * Manage component power modes
 */
void IUBMX055Acc::setPowerMode(PowerMode::option pMode)
{
    m_powerMode = pMode;
    switch (m_powerMode)
    {
        case PowerMode::PERFORMANCE:
        case PowerMode::ENHANCED:
        case PowerMode::REGULAR:
        case PowerMode::LOW_1:
        case PowerMode::LOW_2:
            m_iuI2C->writeByte(ADDRESS, PMU_LPW, 0x00);
            delay(100);
            break;
        case PowerMode::SLEEP:
            // 'SLEEP' mode correspond to the Accelerometer 'suspend' mode
            // => Suspend mode is entered (left) by writing 1 (0) to the
            // (ACC 0 11) suspend bit after bit (ACC 0x12) lowpower_mode has
            // been set to 0.
            m_iuI2C->writeByte(ADDRESS, PMU_LPW, 0x80);
            break;
        case PowerMode::DEEP_SLEEP:
        case PowerMode::SUSPEND:
            // "Deep suspend mode is entered (left) by writing 1 (0) to the
            // (ACC 0x11) deep_suspend bit while (ACC 0x11) suspend bit is set
            // to 0.
            m_iuI2C->writeByte(ADDRESS, PMU_LPW, 0x20);
            break;
        default:
            if (debugMode)
            {
                debugPrint(F("Unhandled power Mode "), false);
                debugPrint(m_powerMode);
            }
    }
}


/* =============================================================================
    Configuration and calibration
============================================================================= */

void IUBMX055Acc::configure(JsonVariant &config)
{
    HighFreqSensor::configure(config);  // General sensor config
    JsonVariant value = config["FSR"];  // Full Scale Range
    if (value.success())
    {
        setScale((scaleOption) (value.as<int>()));
    }
    value = config["BW"];  // Bandwidth for filtering
    if (value.success())
    {
        setBandwidth((bandwidthOption) (value.as<int>()));
    }
    value = config["HPF"];  // Enable filtering
    if (value.success())
    {
        if (value.as<int>())
        {
            useFilteredData(m_bandwidth);
        }
        else
        {
            useUnfilteredData();
        }
    }
}

/**
 * Set the scale then recompute resolution
 *
 * Resolution is m/s2 per LSB.
 */
void IUBMX055Acc::setScale(IUBMX055Acc::scaleOption scale)
{
    m_scale = scale;
    m_iuI2C->writeByte(ADDRESS, PMU_RANGE, (uint8_t) m_scale & 0x0F);
    switch (m_scale)
    {
        case AFS_2G:
            setResolution(2.0 * 9.80665 / 32768.0);
            break;
        case AFS_4G:
            setResolution(4.0 * 9.80665 / 32768.0);
            break;
        case AFS_8G:
            setResolution(8.0 * 9.80665 / 32768.0);
            break;
        case AFS_16G:
            setResolution(16.0 * 9.80665 / 32768.0);
            break;
    }
}

/**
 * Set the accelerometer bandwidth
 */
void IUBMX055Acc::setBandwidth(IUBMX055Acc::bandwidthOption bandwidth)
{
    m_bandwidth = bandwidth;
    m_iuI2C->writeByte(ADDRESS, PMU_BW, (uint8_t) m_bandwidth & 0x0F);
    if (setupDebugMode && !m_filteredData)
    {
        debugPrint("WARNING: Set accelerometer BW for low-pass filter but "
                   "filtering is not active");
    }
}

/**
 * Use accelerometer build in low-pass filter and set the bandwidth option
 *
 * The sampling rate of the filtered data depends on the selected filter
 * bandwidth and is always twice the selected bandwidth (BW = ODR/2).
 */
void IUBMX055Acc::useFilteredData(IUBMX055Acc::bandwidthOption bandwidth)
{
    m_filteredData = true;
    setBandwidth(bandwidth);
    m_iuI2C->writeByte(ADDRESS, D_HBW, 0x00);
}

/**
 * Use unfiltered acceleration data
 *
 * The unfiltered data is sampled with 2kHz.
 */
void IUBMX055Acc::useUnfilteredData()
{
    m_iuI2C->writeByte(ADDRESS, D_HBW, 0x01);
    m_filteredData = false;
}

/**
 * Configure the interrupt pins
 */
void IUBMX055Acc::configureInterrupts()
{
    // Setup interrupt pin on the STM32 as INPUT
    //    pinMode(INT_PIN, INPUT);
    // Enable ACC data ready interrupt
    m_iuI2C->writeByte(ADDRESS, INT_EN_1, 0x10);
    // Set interrupts push-pull, active high for INT1 and INT2
    m_iuI2C->writeByte(ADDRESS, INT_OUT_CTRL, 0x04);
    // Define INT1 (intACC1) as ACC data ready interrupt
    //m_iuI2C->writeByte(ADDRESS, INT_MAP_1, 0x02);
    // Define INT2 (intACC2) as ACC data ready interrupt
    m_iuI2C->writeByte(ADDRESS, INT_MAP_1, 0x80);
}

/**
 * Run fast compensation
 */
void IUBMX055Acc::doFastCompensation(float * dest1)
{
    // set all offset compensation registers to zero
    m_iuI2C->writeByte(ADDRESS, OFC_CTRL, 0x80);
    // set offset targets to 0, 0, and +1 g for x, y, z axes
    m_iuI2C->writeByte(ADDRESS, OFC_SETTING, 0x20);

    uint8_t offsetCmd[3] = {0x20, 0x40, 0x60}; // x, y and z axis
    byte c = 0;
    for (int i = 0; i < 3; ++i)
    {
        // calculate x, y or z axis offset
        m_iuI2C->writeByte(ADDRESS, OFC_CTRL, offsetCmd[i]);
        // check if fast calibration complete. While not complete, wait.
        c = m_iuI2C->readByte(ADDRESS, OFC_CTRL);
        while (!(c & 0x10))
        {
            delay(10);
            c = m_iuI2C->readByte(ADDRESS, OFC_CTRL);
        }
    }
    // Acceleration bias on 8bit int - resolution is +/- 1g
    int8_t compx = m_iuI2C->readByte(ADDRESS, OFC_OFFSET_X);
    int8_t compy = m_iuI2C->readByte(ADDRESS, OFC_OFFSET_Y);
    int8_t compz = m_iuI2C->readByte(ADDRESS, OFC_OFFSET_Z);

    // Convert to G in 15bit (for consistance with the accel readings)
    m_bias[0] = (uint16_t) compx << 8;
    m_bias[1] = (uint16_t) compy << 8;
    m_bias[2] = (uint16_t) compz << 8;
    if (setupDebugMode)
    {
        debugPrint("accel biases (mG):");
        debugPrint(1000 * q15ToFloat(m_bias[0]));
        debugPrint(1000 * q15ToFloat(m_bias[1]));
        debugPrint(1000 * q15ToFloat(m_bias[2]));
    }
}


/* =============================================================================
    Data Collection
============================================================================= */

/**
 * Acquire new data, while handling down-clocking
 */
void IUBMX055Acc::acquireData(bool inCallback, bool force)
{
    // Process data from last acquisition if needed
    processData();
    // Acquire new data
    HighFreqSensor::acquireData(inCallback, force);
}

/**
 * Read acceleration data
 */
void IUBMX055Acc::readData()
{
    // Read the six raw data registers into data array
    if (!m_iuI2C->readBytes(ADDRESS, D_X_LSB, 6, &m_rawBytes[0],
                            accelReadCallback))
    {
        if (asyncDebugMode)
        {
            debugPrint("Skip accel read");
        }
    }
}

/**
 * Process acceleration data and store it as a Q15 (signed 15-fractional-bit)
 *
 * Data is read from device as 2 bytes: LSB first (4 bits to use) then MSB
 * (8 bits to use) 4 last bits of LSB byte are used as flags (new data, etc).
 */
void IUBMX055Acc::processData()
{
    if (!newAccelData)
    {
        return;
    }
    // Check that all 3 axes have new data, if not return
    if (!((m_rawBytes[0] & 0x01) && (m_rawBytes[2] & 0x01) &&
            (m_rawBytes[4] & 0x01)))
    {
        return;
    }
    for (uint8_t i = 0; i < 3; ++i)
    {
        /*  Convert to Q15: LSB = LLLLXXXX, MSB = MMMMMMMMM =>
            rawAccel = MMMMMMMMLLLL0000
            use 8 bits of MSB and only the 4 left-most bits of LSB
            NB: Casting has precedence over << or >> operators */
        m_rawData[i] = (int16_t) (((int16_t)m_rawBytes[2 * i + 1] << 8) | \
                                  (m_rawBytes[2 * i] & 0xF0));
        m_data[i] = m_rawData[i] + m_bias[i];
        m_destinations[i]->addQ15Value(m_data[i]);
    }
    newAccelData = false;
}


/* =============================================================================
    Communication
============================================================================= */

/**
 * Dump acceleration data to serial - unit is G, in float format
 *
 * NB: We want to do this in *DATA COLLECTION* mode
 */
void IUBMX055Acc::sendData(HardwareSerial *port)
{
    if (loopDebugMode)  // Human readable in the console
    {
        port->print("AX: ");
        port->println((float) m_data[0] * m_resolution / 9.80665, 4);
        port->print("AY: ");
        port->println((float) m_data[1] * m_resolution / 9.80665, 4);
        port->print("AZ: ");
        port->println((float) m_data[2] * m_resolution / 9.80665, 4);
    }
    else  // Send bytes
    {
        float accel;
        byte* data;
        for (uint8_t i = 0; i < 3; ++i)
        {
            // Stream float value as 4 bytes
            accel = (float) m_data[i] * m_resolution / 9.80665;
            data = (byte*) &accel;
            port->write(data, 4);
        }
    }
}


/* =============================================================================
    Debugging
============================================================================= */

/*
 * Show calibration info
 */
void IUBMX055Acc::exposeCalibration()
{
#ifdef IUDEBUG_ANY
    debugPrint(F("Accelerometer calibration data: "));
    debugPrint(F("Resolution (m.s-2): "));
    debugPrint(m_resolution, 7);
    debugPrint(F("Bias (q15_t): "));
    for (uint8_t i = 0; i < 3; ++i)
    {
        debugPrint(m_bias[i], false);
        debugPrint(", ", false);
    }
    debugPrint(' ');
#endif
}
