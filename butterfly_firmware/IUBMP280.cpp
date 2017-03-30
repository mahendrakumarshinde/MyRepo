#include "IUBMP280.h"

IUI2C* IUBMP280::m_iuI2C = NULL;
uint8_t IUBMP280::m_rawTempBytes[3]; // 20-bit temperature register data stored here
int32_t IUBMP280::m_fineTemperature = 0;
int16_t IUBMP280::m_digTemperature[3] = {0, 0, 0};
int16_t IUBMP280::m_temperature = 0;
uint8_t IUBMP280::m_rawPressureBytes[3] = {0, 0, 0};
int16_t IUBMP280::m_digPressure[9]= {0, 0, 0, 0, 0, 0, 0, 0, 0};
int16_t IUBMP280::m_pressure = 0;

char IUBMP280::sensorTypes[IUBMP280::sensorTypeCount] =
{
  IUABCSensor::sensorType_thermometer,
  IUABCSensor::sensorType_barometer
};

IUBMP280::IUBMP280(IUI2C *iuI2C) :
  IUABCSensor(),
  m_newData(false),
  m_posr(defaultPosr),
  m_tosr(defaultTosr),
  m_iirFilter(defaultIIRFilter),
  m_mode(defaultMode),
  m_sby(defaultSBy)
{
  m_iuI2C = iuI2C;
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
  // Set T and P oversampling rates and sensor mode
  m_iuI2C->writeByte(ADDRESS, CTRL_MEAS, m_tosr << 5 | m_posr << 2 | m_mode);
  // Set standby time interval in normal mode and bandwidth
  m_iuI2C->writeByte(ADDRESS, CONFIG, m_sby << 5 | m_iirFilter << 2);
}

void IUBMP280::reset()
{
  m_iuI2C->writeByte(ADDRESS, RESET, 0xB6); // reset BMP280 before initilization
  delay(100);
}

/**
 * Configure the BMP280 and store calibration data
 */
void IUBMP280::initSensor()
{
  reset();
  setOptions(m_posr, m_tosr, m_iirFilter, m_mode, m_sby);
  
  // Read and store calibration data
  uint8_t calib[24];
  m_iuI2C->readBytes(ADDRESS, CALIB00, 24, &calib[0]);
  for (int i = 0; i < 3; i++)
  {
    m_digTemperature[i] = (uint16_t)(((uint16_t) calib[2 * i + 1] << 8) | calib[2 * i]);
  }
  for (int i = 0; i < 9; i++)
  {
    m_digPressure[i] = (uint16_t)(((uint16_t) calib[2 * (i + 3) + 1] << 8) | calib[2 * (i + 3)]);
  }
  m_iuI2C->port->println("BMP280 Initialized successfully.\n");
  m_iuI2C->port->flush();
}

/**
 * Ping the component address and, if the answer is correct, initialize it
 */
void IUBMP280::wakeUp()
{
  // Read the WHO_AM_I register of the BMP280 this is a good test of communication
  if(m_iuI2C->checkComponentWhoAmI("BMP280", ADDRESS, WHO_AM_I, WHO_AM_I_ANSWER))
  {
    initSensor(); // Initialize BMP280
  }
  else
  {
    m_iuI2C->setErrorMessage("BMPERR");
  }
  delay(15);
}

/**
 * Update m_temperature with current temperature estimation and return it
 * @return the temperature in Celsius degree
 * Note: if there is a read error, return previous temperature estimation
 */
void IUBMP280::readTemperature() // Index 4
{
  m_iuI2C->readBytes(ADDRESS, TEMP_MSB, 3, &m_rawTempBytes[0], processTemperatureData);
}

/**
 * Process a raw Temperature reading (to be passed as callbak of Pressure reading via wire.transfer)
 */
void IUBMP280::processTemperatureData(uint8_t wireStatus)
{
  if (m_iuI2C->isReadError())
  {
    m_iuI2C->port->print("Skip temperature read: ");
    m_iuI2C->port->println(m_iuI2C->getReadError(), HEX);
    m_iuI2C->resetReadError();
    return;
  }
  int32_t rawTemp = (int32_t) (((int32_t) m_rawTempBytes[0] << 16 | (int32_t) m_rawTempBytes[1] << 8 | m_rawTempBytes[2]) >> 4);
  m_temperature = compensateTemperature(rawTemp);
}

/**
 * Return temperature in DegC
 * Note: Also update m_fineTemperature
 * @param rawT raw temperature as output by readRawTemperature
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
  // resolution is 0.01 DegC, need to divide by 100 (eg: Output value of “5123” equals 51.23 DegC.)
  return (float) T / 100.;
}


/**
 * Read and compute the current pressure, store it and return it
 * @return the pressure in Pascal (Pa)
 * Note: if there is a read error, return previous pressure reading
 */
void IUBMP280::readPressure() // Index 4
{
  m_iuI2C->readBytes(ADDRESS, PRESS_MSB, 3, &m_rawPressureBytes[0], processPressureData);
}

/**
 * Process a raw Pressure reading (to be passed as callbak of Pressure reading via wire.transfer)
 */
void IUBMP280::processPressureData(uint8_t wireStatus)
{
  if (m_iuI2C->isReadError())
  {
    m_iuI2C->port->println("Skip Pressuure read");
    m_iuI2C->port->println(m_iuI2C->getReadError(), HEX);
    m_iuI2C->resetReadError();
    return;
  }
  int32_t rawP = (int32_t) (((int32_t) m_rawPressureBytes[0] << 16 | (int32_t) m_rawPressureBytes[1] << 8 | m_rawPressureBytes[2]) >> 4);
  m_pressure = (float) compensatePressure(rawP) / 100.;
}

/**
 * Return the compensated Pressure
 * @param rawP raw pressure as output by readRawPressure
 */
float IUBMP280::compensatePressure(int32_t rawP)
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
    return 0; // avoid exception caused by division by zero
  }
  p = 1048576 - rawP;
  p = (((p << 31) - var2) * 3125) / var1;
  var1 = (((long long)m_digPressure[8]) * (p >> 13) * (p >> 13)) >> 25;
  var2 = (((long long)m_digPressure[7]) * p) >> 19;
  p = ((p + var1 + var2) >> 8) + (((long long)m_digPressure[6]) << 4);
  // pressure in Pa in Q24.8 format (24 integer bits and 8 fractional bits)
  // => need to convert to float eg: p = 24674867 represents 24674867/256 = 96386.2 Pa
  float result = (float) p / 256.;
  return result;
}

void IUBMP280::readData()
{
  readTemperature();
  readPressure();
  m_newData = true;
}

/**
 * Send data to receiver following dataSendOption
 */
void IUBMP280::sendToReceivers()
{
  if (!m_newData)
  {
    return;
  }
  for (int i = 0; i < m_receiverCount; i++)
  {
    if (m_receivers[i])
    {
      switch(m_toSend[i])
      {
        case (uint8_t) dataSendOption::temperature:
          m_receivers[i]->receiveScalar(m_receiverSourceIndex[i], m_temperature);
          break;
        case (uint8_t) dataSendOption::pressure:
          m_receivers[i]->receiveScalar(m_receiverSourceIndex[i], m_pressure);
          break;
        default:
          break;
      }
    }
  }
  m_newData = false;
}

/**
 * Dump Temperature data to serial via I2C
 * NB: We want to do this in *DATA COLLECTION* mode
 */
void IUBMP280::dumpDataThroughI2C()
{
  byte* data;
  // Stream temperature float value as 4 bytes
  data = (byte *) &m_temperature;
  m_iuI2C->port->write(data, 4);
  m_iuI2C->port->flush();
  // Stream pressure float value as 4 bytes
  data = (byte *) &m_pressure;
  m_iuI2C->port->write(data, 4);
  m_iuI2C->port->flush();
}

