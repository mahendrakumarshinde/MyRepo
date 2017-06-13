#include "IUBMX055Acc.h"

/* ================================= Static member definition ================================= */

IUI2C* IUBMX055Acc::m_iuI2C = NULL;
uint8_t IUBMX055Acc::m_rawBytes[6] = {0, 0, 0, 0, 0, 0};
q15_t IUBMX055Acc::m_rawData[3] = {0, 0, 0};
q15_t IUBMX055Acc::m_data[3] = {0, 0, 0};
q15_t IUBMX055Acc::m_bias[3] = {0, 0, 0};
q15_t IUBMX055Acc::m_resolution = 0;

/* ================================= Method definition ================================= */

/**
 * Initialize and configure the BMX055 for the accelerometer
 */
IUBMX055Acc::IUBMX055Acc(IUI2C *iuI2C) :
  IUABCSensor(),
  m_newData(false),
  m_scale(defaultScale),
  m_filteredData(false),
  m_bandwidth(defaultBandwidth)
{
  m_iuI2C = iuI2C;
  m_samplingRate = defaultSamplingRate;
  if (!m_iuI2C->checkComponentWhoAmI("BMX055 ACC", ACC_ADDRESS, ACC_WHO_AM_I, ACC_I_AM))
  {
    m_iuI2C->setErrorMessage("ACCERR");
    return;
  }
  softReset();
  wakeUp();
  setscale(m_scale);
  useUnfilteredData();
  configureInterrupts();
  m_asynchronous = true;
  if (setupDebugMode) { debugPrint(F("Accelerometer initialized successfully.\n")); }
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
}

/* ============================  Hardware & power management methods ============================ */

/**
 * Set the power mode to ACTIVE (130μA)
 */
void IUBMX055Acc::wakeUp()
{
  m_powerMode = powerMode::ACTIVE;
  writeByte(BMX055_ACC_ADDRESS, BMX055_ACC_PMU_LPW, 0x00);
  delay(100);
}

/**
 * Set the power mode to SLEEP (2.1μA)
 * IU 'SLEEP' mode correspond to the Accelerometer 'suspend' mode
 * "Suspend mode is entered (left) by writing 1 (0) to the (ACC 0 11)
 * suspend bit after bit (ACC 0x12) lowpower_mode has been set to 0."
 */
void IUBMX055Acc::sleep()
{
  m_powerMode = powerMode::SLEEP;
  writeByte(ADDRESS, PMU_LPW, 0x80);
}

/**
 * Set the power mode to SUSPEND (1μA)
 * IU 'SUSPEND' mode correspond to the Accelerometer 'deep-suspend' mode
 * "Deep suspend mode is entered (left) by writing 1 (0) to the (ACC 0x11)
 * deep_suspend bit while (ACC 0x11) suspend bit is set to 0."
 */
void IUBMX055Acc::suspend()
{
  m_powerMode = powerMode::SUSPEND;
  writeByte(ADDRESS, PMU_LPW, 0x20);
}

/**
 * Set the scale then recompute resolution
 */
void IUBMX055Acc::setScale(IUBMX055Acc::scaleOption scale)
{
  m_scale = scale;
  m_iuI2C->writeByte(ADDRESS, PMU_RANGE, (uint8_t) m_scale & 0x0F);
  computeResolution();
}

/**
 * Set the accelerometer bandwidth
 */
void IUBMX055Acc::setBandwidth(IUBMX055Acc::bandwidthOption bandwidth)
{
  m_Bandwidth = bandwidth;
  m_iuI2C->writeByte(ADDRESS, PMU_BW, (uint8_t) m_Bandwidth & 0x0F);
  if (setupDebugMode && !m_filteredData)
  {
    debugPrint("WARNING: Set accelerometer BW for low-pass filter but filtering is not active");
  }
}

/**
 * Use accelerometer build in low-pass filter and set the bandwidth option
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
  m_iuI2C->writeByte(ADDRESS, INT_EN_1, 0x10);     // Enable ACC data ready interrupt
  m_iuI2C->writeByte(ADDRESS, INT_OUT_CTRL, 0x04); // Set interrupts push-pull, active high for INT1 and INT2
  //m_iuI2C->writeByte(ADDRESS, INT_MAP_1, 0x02);  // Define INT1 (intACC1) as ACC data ready interrupt
  m_iuI2C->writeByte(ADDRESS, INT_MAP_1, 0x80);    // Define INT2 (intACC2) as ACC data ready interrupt
}

/**
 * Run fast compensation
 */
void IUBMX055Acc::doFastCompensation(float * dest1)
{
  m_iuI2C->writeByte(ADDRESS, OFC_CTRL, 0x80);    // set all offset compensation registers to zero
  m_iuI2C->writeByte(ADDRESS, OFC_SETTING, 0x20); // set offset targets to 0, 0, and +1 g for x, y, z axes

  uint8_t offsetCmd[3] = {0x20, 0x40, 0x60}; // x, y and z axis
  byte c = 0;
  for (int i = 0; i < 3; i++)
  {
    m_iuI2C->writeByte(ADDRESS, OFC_CTRL, offsetCmd[i]);   // calculate x, y or z axis offset
    c = m_iuI2C->readByte(ADDRESS, OFC_CTRL);              // check if fast calibration complete
    while(!(c & 0x10))                                             // While not complete, wait
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


/* ==================== Data Collection functions ======================== */

/**
 * Set the resolution depending on the full scale value
 */
void IUBMX055Acc::computeResolution() {
  switch (m_scale)
  {
    case AFS_2G:
      m_resolution = 2;
      break;
    case AFS_4G:
      m_resolution = 4;
      break;
    case AFS_8G:
      m_resolution = 8;
      break;
    case AFS_16G:
      m_resolution = 16;
      break;
  }
}

/**
 * Asynchronously read acceleration data and and call processData as callback
 * Data is read from device as 2 bytes: LSB first (4 bits to use) then MSB
 * (8 bits to use) 4 last bits of LSB byte are used as flags (new data, etc).
 */
void IUBMX055Acc::readData()
{
  // Read the six raw data registers into data array
  if(!m_iuI2C->readBytes(ADDRESS, D_X_LSB, 6, &m_rawBytes[0], processData))
  {
    if (callbackDebugMode) { debugPrint("Skip accel read"); }
  }
}

/**
 * Process acceleration data and store it as a Q15 (signed 15-fractional-bit)
 * Data is read from device as 2 bytes: LSB first (4 bits to use) then MSB
 * (8 bits to use) 4 last bits of LSB byte are used as flags (new data, etc).
 */
void IUBMX055Acc::processData(uint8_t wireStatus)
{
  if(!((m_rawBytes[0] & 0x01) && (m_rawBytes[2] & 0x01) && (m_rawBytes[4] & 0x01)))
  {
    return; // Check that all 3 axes have new data, if not return
  }
  for (uint8_t i = 0; i < 3; ++i)
  {
    // Convert to Q15: LSB = LLLLXXXX, MSB = MMMMMMMMM => rawAccel = MMMMMMMMLLLL0000
    // use 8 bits of MSB and only the 4 left-most bits of LSB
    // NB: Casting has precedence over << or >> operators
    m_rawData[i] = (int16_t) (((int16_t)m_rawBytes[2 * i + 1] << 8) | (m_rawBytes[2 * i] & 0xF0));
    m_data[i] = m_rawData[i] + m_bias[i];
  }
  m_newData = true;
}


/* ==================== Communication functions ======================== */

void IUBMX055Acc::sendToReceivers()
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
        case (uint8_t) dataSendOption::X:
          m_receivers[i]->receiveScalar(m_receiverSourceIndex[i], m_data[0]);
          break;
        case (uint8_t) dataSendOption::Y:
          m_receivers[i]->receiveScalar(m_receiverSourceIndex[i], m_data[1]);
          break;
        case (uint8_t) dataSendOption::Z:
          m_receivers[i]->receiveScalar(m_receiverSourceIndex[i], m_data[2]);
          break;
        case (uint8_t) dataSendOption::resolution:
          m_receivers[i]->receiveScalar(m_receiverSourceIndex[i], m_resolution);
          break;
        case (uint8_t) dataSendOption::samplingRate:
          m_receivers[i]->receiveScalar(m_receiverSourceIndex[i], (q15_t) m_samplingRate);
          break;
      }
    }
  }
  m_newData = false;
}

/**
 * Dump acceleration data to serial via I2C - unit is G, in float format
 * NB: We want to do this in *DATA COLLECTION* mode
 */
void IUBMX055Acc::dumpDataThroughI2C()
{
  if (!m_newData)
  {
    return;
  }
  float accel;
  byte* data;
  for (uint8_t i = 0; i < 3; i++)
  {
    // Stream float value as 4 bytes
    accel = toG(m_data[i], m_resolution);
    data = (byte*) &accel;
    m_iuI2C->port->write(data, 4);
  }
  m_newData = false;
}

/**
 * Dump acceleration data to serial via I2C - accel unit is G, in float format
 * NB: We want to do this in *DATA COLLECTION* mode, when debugMode is true
 */
void IUBMX055Acc::dumpDataForDebugging()
{
  if (!m_newData)
  {
    return;
  }
  m_iuI2C->port->print("AX: ");
  m_iuI2C->port->println(q15ToFloat(m_data[0]), 4);
  m_iuI2C->port->print("AY: ");
  m_iuI2C->port->println(q15ToFloat(m_data[1]), 4);
  m_iuI2C->port->print("AZ: ");
  m_iuI2C->port->println(q15ToFloat(m_data[2]), 4);
  m_iuI2C->port->flush();
  m_newData = false;
}


/* ====================== Diagnostic Functions, only active when setupDebugMode = true ====================== */

/*
 * Show calibration info
 */
void IUBMX055Acc::exposeCalibration()
{
  #ifdef DEBUGMODE
  debugPrint(F("Accelerometer calibration data: "));
  debugPrint(F("Resolution (G): "));
  debugPrint(m_resolution);
  debugPrint(F("Bias (q15_t): "));
  for (uint8_t i = 0; i < 3; ++i)
  {
    debugPrint(m_bias[i], false);
    debugPrint(", ", false);
  }
  debugPrint(' ');
  #endif
}

