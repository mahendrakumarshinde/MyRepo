#include "IULSM6DSM.h"


/* =============================================================================
    Data Acquisition callbacks
============================================================================= */

bool newLSM6DSMAccelData = false;

void LSM6DSMAccelReadCallback(uint8_t wireStatus)
{
    iuI2C.releaseReadLock();
    if (wireStatus == 0)
    {
        newLSM6DSMAccelData = true;
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
IULSM6DSM::IULSM6DSM(IUI2C *iuI2C, const char* name, Feature *accelerationX,
                     Feature *accelerationY, Feature *accelerationZ,
                     Feature *tiltX, Feature *tiltY,
                     Feature *tiltZ) :
    HighFreqSensor(name, 6, accelerationX, accelerationY, accelerationZ, tiltX,
                 tiltY, tiltZ),
    m_scale(defaultScale),
    m_gyroScale(defaultGyroScale),
    m_odr(defaultODR)
{
    m_iuI2C = iuI2C;
    for (uint8_t i = 0; i < 3; ++i)
    {
        m_bias[i] = 0;
        m_gyroBias[i] = 0;
    }
}


/* =============================================================================
    Hardware & power management
============================================================================= */

/**
 * Set up the component and finalize the object initialization
 */
void IULSM6DSM::setupHardware()
{
    if (!m_iuI2C->checkComponentWhoAmI("LSM6DSM ACC", ADDRESS, WHO_AM_I, I_AM))
    {
        if (debugMode)
        {
            debugPrint(F("LSM6DSM ERROR"));
        }
        return;
    }
    softReset();

    /* CTRL3_C Data output configuration: Noting to do
    - By default, interrupts active HIGH, push pull, little endian data => OK
    - By default, block data update is disabled => OK
    - By default, I2C read bit auto-incremented => OK
    */
//    uint8_t temp = m_iuI2C->readByte(ADDRESS, CTRL3_C);
//    m_iuI2C->writeByte(ADDRESS, CTRL3_C, temp | 0x40 | 0x04);

    setPowerMode(PowerMode::REGULAR);
    setScale(m_scale);
    setGyroScale(m_gyroScale);
    setSamplingRate(defaultSamplingRate);
}

/**
 * Reset configuration and return to normal power mode
 */
void IULSM6DSM::softReset()
{
    // reset device: Read current register content and just put 1 to reset bit
    uint8_t temp = m_iuI2C->readByte(ADDRESS, CTRL3_C);
    // Set bit 0 to 1 to reset LSM6DSM
    m_iuI2C->writeByte(ADDRESS, CTRL3_C, temp | 0x01);
    delay(100); // Wait for all registers to reset
}

/**
 * Manage component power modes
 */
void IULSM6DSM::setPowerMode(PowerMode::option pMode)
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
        default:
            if (debugMode)
            {
                debugPrint(F("Unhandled power Mode "), false);
                debugPrint(m_powerMode);
            }
    }
}

/**
 * Rate cannot be set to zero + call computeDownclockingRate at the end
 */
void IULSM6DSM::setSamplingRate(uint16_t samplingRate)
{
    HighFreqSensor::setSamplingRate(samplingRate);
    m_odr = ODR_12_5Hz;
    if (samplingRate > 12) { m_odr = ODR_26Hz; }
    else if (samplingRate > 26) { m_odr = ODR_52Hz; }
    else if (samplingRate > 52) { m_odr = ODR_104Hz; }
    else if (samplingRate > 104) { m_odr = ODR_208Hz; }
    else if (samplingRate > 208) { m_odr = ODR_416Hz; }
    else if (samplingRate > 416) { m_odr = ODR_833Hz; }
    else if (samplingRate > 833) { m_odr = ODR_1660Hz; }
    else if (samplingRate > 1660) { m_odr = ODR_3330Hz; }
    else if (samplingRate > 3330) { m_odr = ODR_6660Hz; }
    // Now call setScale to set the ODR at the same time
    setScale(m_scale);
}

/**
 * Configure the interrupt pins
 */
void IULSM6DSM::configureInterrupts()
{
    // latch interrupt until data read
    m_iuI2C->writeByte(ADDRESS, DRDY_PULSE_CFG, 0x80);
    // enable significant motion interrupts on INT1
    m_iuI2C->writeByte(ADDRESS, INT1_CTRL, 0x40);
    // enable accel/gyro data ready interrupts on INT2
    m_iuI2C->writeByte(ADDRESS, INT2_CTRL, 0x03);
}

/**
 *
 */
void IULSM6DSM::computeBias()
{
    // TODO: Reimplement if needed
//    if (setupDebugMode)
//    {
//        int16_t temp[7] = {0, 0, 0, 0, 0, 0, 0};
//        int32_t sum[7] = {0, 0, 0, 0, 0, 0, 0};
//        debugPrint("Calculate accel and gyro offset biases: keep sensor flat "
//                   "and motionless!");
//        delay(4000);
//        for (int ii = 0; ii < 128; ii++)
//        {
//            readData(temp);
//            sum[1] += temp[1];
//            sum[2] += temp[2];
//            sum[3] += temp[3];
//            sum[4] += temp[4];
//            sum[5] += temp[5];
//            sum[6] += temp[6];
//            delay(50);
//        }
//        m_gyroBias[0] = (q15_t) (sum[1] * 128);
//        m_gyroBias[1] = (q15_t) (sum[2] * 128);
//        m_gyroBias[2] = (q15_t) (sum[3] * 128);
//        m_bias[0] = (q15_t) (sum[4] * 128);
//        m_bias[1] = (q15_t) (sum[5] * 128);
//        m_bias[2] = (q15_t) (sum[6] * 128);
//        // Remove gravity from the accelerometer bias calculation
//        q15_t g = 32768;
//        switch (m_scale)
//        {
//            case AFS_2G:
//                g = 16483;
//                break;
//            case AFS_4G:
//                g = 8192;
//                break;
//            case AFS_8G:
//                g = 4096;
//                break;
//            case AFS_16G:
//                g = 2048;
//                break;
//        }
//        for (uint8_t i = 0; i < 3; ++i)
//        {
//            if (m_bias[i] > g)
//            {
//                m_bias[0] -= g;
//            }
//            else if (m_bias[0] < - g)
//            {
//                m_bias[0] += g;
//            }
//        }
//    }
}


/* =============================================================================
    Configuration and calibration
============================================================================= */

void IULSM6DSM::configure(JsonVariant &config)
{
    HighFreqSensor::configure(config);  // General sensor config
    JsonVariant value = config["FSR"];  // Full Scale Range
    if (value.success())
    {
        setScale((scaleOption) (value.as<int>()));
    }
}

/**
 * Set the scale then recompute resolution
 *
 * Resolution is m/s2 per LSB.
 */
void IULSM6DSM::setScale(IULSM6DSM::scaleOption scale)
{
    m_scale = scale;
    m_iuI2C->writeByte(ADDRESS, CTRL1_XL,
                       (uint8_t) m_odr << 4 | (uint8_t) m_scale << 2);
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
 * Set the scale then recompute resolution
 *
 * Resolution is m/s2 per LSB.
 */
void IULSM6DSM::setGyroScale(IULSM6DSM::gyroScaleOption gyroScale)
{
    m_gyroScale = gyroScale;
    m_iuI2C->writeByte(ADDRESS, CTRL2_G,
                       (uint8_t) m_odr << 4 | (uint8_t) m_gyroScale << 2);
    // TODO => separate Accelerometer and Gyroscope
//    switch (m_gyroScale)
//    {
//        case GFS_250DPS:
//            setResolution(250.0 / 32768.0);
//            break;
//        case GFS_500DPS:
//            setResolution(500.0 / 32768.0);
//            break;
//        case GFS_1000DPS:
//            setResolution(1000.0 / 32768.0);
//            break;
//        case GFS_2000DPS:
//            setResolution(2000.0 / 32768.0);
//            break;
//    }
}


/* =============================================================================
    Data Collection
============================================================================= */

/**
 * Acquire new data, while handling down-clocking
 */
void IULSM6DSM::acquireData(bool inCallback, bool force)
{
    // Process data from last acquisition if needed
    processData();
    // Acquire new data
    HighFreqSensor::acquireData(inCallback, force);
}

/**
 * Read acceleration data
 *
 * Each data point is read from device as 2 bytes, LSB first.
 */
void IULSM6DSM::readData()
{
    if (!m_iuI2C->readBytes(ADDRESS, OUT_TEMP_L, 14, &m_rawBytes[0],
                            LSM6DSMAccelReadCallback))
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
 * Each data point is read from device as 2 bytes, LSB first.
*/
void IULSM6DSM::processData()
{
    if (!newLSM6DSMAccelData)
    {
        return;
    }
    m_temperature = ((int16_t)m_rawBytes[1] << 8) | m_rawBytes[0];
    m_rawGyroData[0] = ((int16_t)m_rawBytes[3] << 8) | m_rawBytes[2];
    m_rawGyroData[1] = ((int16_t)m_rawBytes[5] << 8) | m_rawBytes[4];
    m_rawGyroData[2] = ((int16_t)m_rawBytes[7] << 8) | m_rawBytes[6];
    m_rawData[0] = ((int16_t)m_rawBytes[9] << 8) | m_rawBytes[8];
    m_rawData[1] = ((int16_t)m_rawBytes[11] << 8) | m_rawBytes[10];
    m_rawData[2] = ((int16_t)m_rawBytes[13] << 8) | m_rawBytes[12];
    for (uint8_t i = 0; i < 3; ++i)
    {
        // Accelaration data
        m_data[i] = m_rawData[i] + m_bias[i];
        m_destinations[i]->addQ15Value(m_data[i]);
        // Gyroscope data
        m_gyroData[i] = m_rawGyroData[i] + m_gyroBias[i];
        m_destinations[i + 3]->addQ15Value(m_gyroData[i]);
    }
    newLSM6DSMAccelData = false;
}


/* =============================================================================
    Communication
============================================================================= */

/**
 * Dump acceleration data to serial - unit is G, in float format
 *
 * NB: We want to do this in *DATA COLLECTION* mode
 */
void IULSM6DSM::sendData(HardwareSerial *port)
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
void IULSM6DSM::exposeCalibration()
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
