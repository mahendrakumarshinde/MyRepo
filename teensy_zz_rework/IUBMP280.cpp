#include "IUBMP280.h"

IUBMP280::IUBMP280(IUI2CTeensy iuI2C, IUBLE iuBLE) : m_iuI2C(iuI2C), m_iuBLE(iuBLE)
{
    // Specify BMP280 default configuration
    setOptions(P_OSR_00, T_OSR_01, BW0_042ODR, normal, t_62_5ms);
}


IUBMP280::IUBMP280(IUI2CTeensy iuI2C,
                   IUBLE iuBLE,
                   IUBMP280::posrOptions posr,
                   IUBMP280::tosrOptions tosr, 
                   IUBMP280::IIRFilterOptions iirFilter, 
                   IUBMP280::ModeOptions mode, 
                   IUBMP280::SByOptions sby) : m_iuI2C(iuI2C), m_iuBLE(iuBLE)
{
    setOptions(posr, tosr, iirFilter, mode, sby);
}

IUBMP280::~IUBMP280()
{
    //dtor
}

void IUBMP280::setOptions(IUBMP280::posrOptions posr,
                          IUBMP280::tosrOptions tosr, 
                          IUBMP280::IIRFilterOptions iirFilter,
                          IUBMP280::ModeOptions mode,
                          IUBMP280::SByOptions sby)
{
  m_posr = posr;
  m_tosr = tosr;
  m_iirFilter = iirFilter;
  m_mode = mode;
  m_sby = sby;
}

/**
 * Configure the BMP280 and store calibration data
 */
void IUBMP280::initSensor()
{
  // Set T and P oversampling rates and sensor mode
  m_iuI2C.writeByte(ADDRESS, CTRL_MEAS, m_tosr << 5 | m_posr << 2 | m_mode);
  // Set standby time interval in normal mode and bandwidth
  m_iuI2C.writeByte(ADDRESS, CONFIG, m_sby << 5 | m_iirFilter << 2);
  // Read and store calibration data
  uint8_t calib[24];
  m_iuI2C.readBytes(ADDRESS, CALIB00, 24, &calib[0]);
  for (int i = 0; i < 3; i++)
  {
    m_digTemperature[i] = (uint16_t)(((uint16_t) calib[2 * i + 1] << 8) | calib[2 * i]);
  }
  for (int i = 0; i < 9; i++)
  {
    m_digPressure[i] = (uint16_t)(((uint16_t) calib[2 * (i + 3) + 1] << 8) | calib[2 * (i + 3)]);
  }
  m_iuI2C.port->println("BMP280 Initialized successfully.\n");
  m_iuI2C.port->flush();
}

/**
 * Ping the device address and, if the answer is correct, initialize it
 */
void IUBMP280::wakeUp()
{
  // Read the WHO_AM_I register of the BMP280 this is a good test of communication
  byte c = m_iuI2C.readByte(ADDRESS, WHO_AM_I);  // Read WHO_AM_I register
  if (!m_iuI2C.isSilent())
  {
    m_iuI2C.port->print("BMP280 I AM ");
    m_iuI2C.port->print(c, HEX);
    m_iuI2C.port->print(" I should be ");
    m_iuI2C.port->println(WHO_AM_I_ANSWER, HEX);
    m_iuI2C.port->flush();
  }
  m_iuI2C.writeByte(ADDRESS, RESET, 0xB6); // reset BMP280 before initilization
  delay(100);
  if (c == WHO_AM_I_ANSWER)
  {
    initSensor(); // Initialize BMP280 altimeter
  }
  else
  {
    m_iuI2C.port->print("Could not connect to BMP280: 0x");
    m_iuI2C.port->println(c, HEX);
    m_iuI2C.setErrorMessage("BMPERR");
  }
  delay(15);
}

/**
 * Update m_temperature with current temperature estimation and return it
 * Note: if there is a read error, return previous temperature estimation
 */
float IUBMP280::getCurrentTemperature() // Index 4
{
  int32_t rawTemp = readRawTemperature();
  if (!(m_iuI2C.isReadError()))
  {
    m_temperature = (float) compensateTemperature(rawTemp) / 100.;
  }
  return m_temperature;
}

/**
 * Return the raw temperature reading
 */
int32_t IUBMP280::readRawTemperature()
{
  uint8_t rawData[3];  // 20-bit temperature register data stored here
  m_iuI2C.readBytes(ADDRESS, TEMP_MSB, 3, &rawData[0]);
  if (!(m_iuI2C.isReadError()))
  {
    return (int32_t) (((int32_t) rawData[0] << 16 | (int32_t) rawData[1] << 8 | rawData[2]) >> 4);
  }
  else
  {
    m_iuI2C.port->println("Skip temperature read");
    m_iuI2C.port->println(m_iuI2C.getReadError(), HEX);
    m_iuI2C.resetReadError();
  }
  return 0;
}

/** 
 * Return temperature in DegC, resolution is 0.01 DegC. 
 * eg: Output value of “5123” equals 51.23 DegC.
 * Note: Also update m_fineTemperature
 * @param rawT raw temperature as output by readRawTemperature
 */
int32_t IUBMP280::compensateTemperature(int32_t rawT)
{
  int32_t var1, var2, T;
  int32_t t1 = (int32_t)m_digTemperature[0];
  int32_t t2 = (int32_t)m_digTemperature[1];
  int32_t t3 = (int32_t)m_digTemperature[2];
  var1 = (((rawT >> 3) - (t1 << 1)) * t2) >> 11;
  var2 = (((((rawT >> 4) - t1) * ((rawT >> 4) - t1)) >> 12) * t3) >> 14;
  m_fineTemperature = var1 + var2;
  T = (m_fineTemperature * 5 + 128) >> 8;
  return T;
}

/**
 * Return the raw pressure reading
 */
int32_t IUBMP280::readRawPressure()
{
  uint8_t rawData[3];  // 20-bit pressure register data stored here
  m_iuI2C.readBytes(ADDRESS, PRESS_MSB, 3, &rawData[0]);
  return (int32_t) (((int32_t) rawData[0] << 16 | (int32_t) rawData[1] << 8 | rawData[2]) >> 4);
}

/**
 * Return pressure in Pa in Q24.8 format (24 integer bits and 8 fractional bits).
 * eg: Output value of “24674867” represents 24674867/256 = 96386.2 Pa = 963.862 hPa
 * @param rawPT raw pressure as output by readRawPressure
 */
uint32_t IUBMP280::compensatePressure(int32_t rawP)
{
  long long var1, var2, p;
  var1 = ((long long)m_fineTemperature) - 128000;
  var2 = var1 * var1 * (long long)m_digPressure[5];
  var2 = var2 + ((var1 * (long long)m_digPressure[4]) << 17);
  var2 = var2 + (((long long)m_digPressure[3]) << 35);
  var1 = ((var1 * var1 * (long long)m_digPressure[2]) >> 8) + ((var1 * (long long)m_digPressure[1]) << 12);
  var1 = (((((long long)1) << 47) + var1)) * ((long long)m_digPressure[0]) >> 33;
  if (var1 == 0)
  {
    return 0;
    // avoid exception caused by division by zero
  }
  p = 1048576 - rawP;
  p = (((p << 31) - var2) * 3125) / var1;
  var1 = (((long long)m_digPressure[8]) * (p >> 13) * (p >> 13)) >> 25;
  var2 = (((long long)m_digPressure[7]) * p) >> 19;
  p = ((p + var1 + var2) >> 8) + (((long long)m_digPressure[6]) << 4);
  return (uint32_t)p;
}

