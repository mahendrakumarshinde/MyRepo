#include "IUBMP280.h"


/* =============================================================================
    Data Acquisition callbacks
============================================================================= */

bool newTemperatureData = false;
bool newPressureData = false;

void temperatureReadCallback(uint8_t wireStatus)
{
    if (wireStatus == 0)
    {
        newTemperatureData = true;
    }
    else if (callbackDebugMode)
    {
        debugPrint(F("Temperature read error "), false);
        debugPrint(wireStatus);
    }
}

void pressureReadCallback(uint8_t wireStatus)
{
    if (wireStatus == 0)
    {
        newPressureData = true;
    }
    else if (callbackDebugMode)
    {
        debugPrint(F("Pressure read error "), false);
        debugPrint(wireStatus);
    }
}


/* =============================================================================
    Constructors and destructors
============================================================================= */

IUBMP280::IUBMP280(IUI2C *iuI2C, const char* name, Feature *temperature,
                   Feature *pressure) :
    SynchronousSensor(name, 2, temperature, pressure),
    m_temperature(28),
    m_fineTemperature(0),
    m_pressure(1013)
{
    m_iuI2C = iuI2C;
    temperature->setResolution(0.01);
    pressure->setResolution(0.01);
}


/* =============================================================================
    Hardware & power management
============================================================================= */

/**
 * Set up the component and finalize the object initialization
 */
void IUBMP280::setupHardware()
{
    if(!m_iuI2C->checkComponentWhoAmI("BMP280", ADDRESS, WHO_AM_I,
                                      WHO_AM_I_ANSWER))
    {
        if (debugMode)
        {
            debugPrint("BMPERR");
        }
        return;
    }
    softReset();
    m_usagePreset = usagePreset::P_REGULAR;
    wakeUp();
    writeConfigRegister();
    calibrate();
}

/**
 * Soft reset to default Power-On settings
 */
void IUBMP280::softReset()
{
    m_iuI2C->writeByte(ADDRESS, RESET, 0xB6);
    delay(100);
}

/**
 * Switch to ACTIVE power mode
 *
 * IU 'ACTIVE' power mode correspond to the FORCED BMP280 power mode
 */
void IUBMP280::wakeUp()
{
    SynchronousSensor::wakeUp();
    changeUsagePreset(m_usagePreset);
}

/**
 * Switch to SLEEP power mode
 */
void IUBMP280::sleep()
{
    SynchronousSensor::sleep();
    writeControlMeasureRegister();
}

/**
 * Switch to SUSPEND power mode
 */
void IUBMP280::suspend()
{
    SynchronousSensor::suspend();
    writeControlMeasureRegister();
}


/* =============================================================================
    Configuration and calibration
============================================================================= */

void IUBMP280::switchToLowUsage()
{
    m_usagePreset = usagePreset::P_LOW;
    m_powerByte = (uint8_t) powerModeBytes::FORCED;
    m_pressureOSR = overSamplingRates::SKIPPED;
    m_temperatureOSR = overSamplingRates::OSR_01;
    writeControlMeasureRegister();
    m_iirFilter = IIRFilterCoeffs::OFF;
    m_standByDuration = StandByDurations::t_4000ms;
    writeConfigRegister();
    setSamplingPeriod(60000);  // 1 min
}

void IUBMP280::switchToRegularUsage()
{
    m_usagePreset = usagePreset::P_REGULAR;
    m_powerByte = (uint8_t) powerModeBytes::FORCED;
    m_pressureOSR = overSamplingRates::SKIPPED;
    m_temperatureOSR = overSamplingRates::OSR_01;
    writeControlMeasureRegister();
    m_iirFilter = IIRFilterCoeffs::OFF;
    m_standByDuration = StandByDurations::t_4000ms;
    writeConfigRegister();
    setSamplingPeriod(15000);  // 15s
}

void IUBMP280::switchToEnhancedUsage()
{
    m_usagePreset = usagePreset::P_ENHANCED;
    m_powerByte = (uint8_t) powerModeBytes::FORCED;
    m_pressureOSR = overSamplingRates::SKIPPED;
    m_temperatureOSR = overSamplingRates::OSR_01;
    writeControlMeasureRegister();
    m_iirFilter = IIRFilterCoeffs::OFF;
    m_standByDuration = StandByDurations::t_2000ms;
    writeConfigRegister();
    setSamplingPeriod(5000);  // 5s
}

void IUBMP280::switchToHighUsage()
{
    m_usagePreset = usagePreset::P_HIGH;
    m_powerByte = (uint8_t) powerModeBytes::FORCED;
    m_pressureOSR = overSamplingRates::SKIPPED;
    m_temperatureOSR = overSamplingRates::OSR_01;
    writeControlMeasureRegister();
    m_iirFilter = IIRFilterCoeffs::OFF;
    m_standByDuration = StandByDurations::t_250ms;
    writeConfigRegister();
    setSamplingPeriod(1000);  // 1s
}

/**
 * Set the oversampling rates for pressure and temperature.
 *
 * Oversampling increase reading accuracy but use up more power.
 */
void IUBMP280::setOverSamplingRates(overSamplingRates pressureOSR,
                                    overSamplingRates temperatureOSR)
{
    m_pressureOSR = pressureOSR;
    m_temperatureOSR = temperatureOSR;
    writeControlMeasureRegister();
}

/**
 * Set Internal IIR filtering coefficient
 *
 * IIR filtering reduces short term disturbance (door slamming, wind...)
 */
void IUBMP280::setIIRFiltering(IIRFilterCoeffs iirFilter)
{
    m_iirFilter = iirFilter;
    writeConfigRegister();
}

/**
 * Set up the standby (inactive) duration in Normal mode
 */
void IUBMP280::setStandbyDuration(StandByDurations duration)
{
    m_standByDuration = duration;
    writeConfigRegister();
}

/***** Private hardware management methods *****/

/**
 * Write to the configuration register
 */
void IUBMP280::writeConfigRegister()
{
    m_iuI2C->writeByte(ADDRESS, CONFIG,
                       m_standByDuration << 5 | m_iirFilter << 2);
}

/**
 * Write the Control Measure configuration to the register
 */
void IUBMP280::writeControlMeasureRegister()
{
    m_iuI2C->writeByte(ADDRESS, CTRL_MEAS,
                       m_temperatureOSR << 5 | m_pressureOSR << 2 | m_powerByte);
}

/**
 * Read and store calibration data
 */
void IUBMP280::calibrate()
{
    uint8_t calib[24];
    if (!m_iuI2C->readBytes(ADDRESS, CALIB00, 24, &calib[0]))
    {
        if (setupDebugMode)
        {
            debugPrint("Failed to calibrate BMP280");
        }
        return;
    }
    for (int i = 0; i < 3; ++i)
    {
        m_digTemperature[i] =
            (uint16_t)(((uint16_t) calib[2 * i + 1] << 8) | calib[2 * i]);
    }
    for (int i = 0; i < 9; ++i)
    {
        m_digPressure[i] = (uint16_t)(((uint16_t) calib[2 * (i + 3) + 1] << 8) \
            | calib[2 * (i + 3)]);
    }
}


/* =============================================================================
    Data Acquisition
============================================================================= */


/***** Temperature Reading *****/

/**
 * Update m_temperature with current temperature estimation
 *
 * If there is a read error, the previous temperature is kept instead.
 */
void IUBMP280::readTemperature()
{
  if (!m_iuI2C->readBytes(ADDRESS, TEMP_MSB, 3, &m_rawTempBytes[0],
                          temperatureReadCallback))
  {
    if (callbackDebugMode)
    {
        debugPrint("Skip temperature read");
    }
  }
}

/**
 * Process a raw Temperature reading
 */
void IUBMP280::processTemperatureData()
{
    if (!newTemperatureData)
    {
        return;
    }
    int32_t rawTemp = (int32_t) (((int32_t) m_rawTempBytes[0] << 16 | \
        (int32_t) m_rawTempBytes[1] << 8 | m_rawTempBytes[2]) >> 4);
    m_temperature = compensateTemperature(rawTemp);
    m_destinations[0]->addFloatValue(m_temperature);
    newTemperatureData = false;
}

/**
 * Return temperature in Degree Celsius
 *
 * Also update m_fineTemperature
 * @param rawT Raw temperature as read from register
 */
float IUBMP280::compensateTemperature(int32_t rawT)
{
  int32_t var1, var2, T;
  int32_t t1 = (int32_t)m_digTemperature[0];
  int32_t t2 = (int32_t)m_digTemperature[1];
  int32_t t3 = (int32_t)m_digTemperature[2];
  var1 = (((rawT >> 3) - (t1 << 1)) * t2) >> 11;
  var2 = (((((rawT >> 4) - t1) * ((rawT >> 4) - t1)) >> 12) * t3) >> 14;
  m_fineTemperature = var1 + var2;
  T = (m_fineTemperature * 5 + 128) >> 8;
  return (float) T;  // Resolution is 0.01, so value of 2563 => 25.63 degree C
}


/***** Temperature Reading *****/

/**
 * Read, compute and store the current pressure in hectoPascal (hPa)
 *
 * If there is a read error, the previous pressure is kept.
 */
void IUBMP280::readPressure() // Index 4
{
    if (!m_iuI2C->readBytes(ADDRESS, PRESS_MSB, 3, &m_rawPressureBytes[0],
                            pressureReadCallback))
    {
        if (callbackDebugMode)
        {
            debugPrint("Skip pressure read");
        }
    }
}

/**
 * Process and store a raw Pressure reading in hectoPascal (hPa)
 */
void IUBMP280::processPressureData()
{
    if (!newPressureData)
    {
        return;
    }
    int32_t rawP = (int32_t) (((int32_t) m_rawPressureBytes[0] << 16 | \
        (int32_t) m_rawPressureBytes[1] << 8 | m_rawPressureBytes[2]) >> 4);
    m_pressure = compensatePressure(rawP);
    m_destinations[1]->addFloatValue(m_pressure);
    newPressureData = false;
}

/**
 * Return the compensated Pressure in hPa
 * @param rawP Raw pressure as output by readRawPressure
 */
float IUBMP280::compensatePressure(int32_t rawP)
{
    long long var1, var2, p;
    var1 = ((long long)m_fineTemperature) - 128000;
    var2 = var1 * var1 * (long long)m_digPressure[5];
    var2 = var2 + ((var1 * (long long)m_digPressure[4]) << 17);
    var2 = var2 + (((long long)m_digPressure[3]) << 35);
    var1 = ((var1 * var1 * (long long)m_digPressure[2]) >> 8) + \
        ((var1 * (long long)m_digPressure[1]) << 12);
    var1 = (((((long long)1) << 47) + var1)) * \
        ((long long)m_digPressure[0]) >> 33;
    if (var1 == 0)
    {
        return 0; // avoid exception caused by division by zero
    }
    p = 1048576 - rawP;
    p = (((p << 31) - var2) * 3125) / var1;
    var1 = (((long long)m_digPressure[8]) * (p >> 13) * (p >> 13)) >> 25;
    var2 = (((long long)m_digPressure[7]) * p) >> 19;
    p = ((p + var1 + var2) >> 8) + (((long long)m_digPressure[6]) << 4);
    // pressure in Pa in Q24.8 format (24 integer bits and 8 fractional bits)
    // => need to convert to float eg: p = 24674867 represents 24674867/256 =
    // 96386.2 Pa
    return (float) p / 256.;
}


/***** Acquisition *****/

/**
 * Acquire new data, while handling sampling period
 */
void IUBMP280::acquireData()
{
    // Process data from last acquisition if needed
    processTemperatureData();
    processPressureData();
    // Acquire new data
    SynchronousSensor::acquireData();
}

void IUBMP280::readData()
{
    readTemperature();
    readPressure();
}


/* =============================================================================
    Communication
============================================================================= */

/**
 * Dump Temperature data to serial
 *
 * NB: We want to do this in *DATA COLLECTION* mode.
 * If loopDebugMode is true, will send humand readable content, else nothing.
 */
void IUBMP280::sendData(HardwareSerial *port)
{
    if (loopDebugMode)  // Human readable in the console
    {
        port->print("T: ");
        port->println(m_temperature, 4);
        port->print("P: ");
        port->println(m_pressure, 4);
        port->flush();
    }
}


/* =============================================================================
    Debugging
============================================================================= */

/**
 * Show calibration info
 */
void IUBMP280::exposeCalibration()
{
    #ifdef DEBUGMODE
    debugPrint(F("Calibration data: "));
    debugPrint(F("3 digital Temp vars: "));
    for (uint8_t i = 0; i < 3; ++i)
    {
        debugPrint(m_digTemperature[i], false);
        debugPrint(", ", false);
    }
    debugPrint(' ');
    debugPrint(F("9 digital Pressure vars: "));
    for (uint8_t i = 0; i < 9; ++i)
    {
        debugPrint(m_digPressure[i], false);
        debugPrint(", ", false);
    }
    debugPrint(' ');
    #endif
}


/* =============================================================================
    Instanciation
============================================================================= */
//
//IUBMP280 iuAltimeter(&iuI2C, "ALT", &temperature, &pressure);
