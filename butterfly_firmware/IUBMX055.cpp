#include "IUBMX055.h"

/* ================================= Static member definition ================================= */
IUI2C* IUBMX055::m_iuI2C = NULL;
uint8_t IUBMX055::m_rawAccelBytes[6] = {0, 0, 0, 0, 0, 0};
q15_t IUBMX055::m_rawAccel[3] = {0, 0, 0};
q15_t IUBMX055::m_accelData[3] = {0, 0, 0};
q15_t IUBMX055::m_accelBias[3] = {0, 0, 0};
float IUBMX055::m_accelResolution = 0;

/* ================================= Method definition ================================= */

char IUBMX055::sensorTypes[IUBMX055::sensorTypeCount] =
{
  IUABCSensor::sensorType_accelerometer,
  IUABCSensor::sensorType_gyroscope,
  IUABCSensor::sensorType_magnetometer
};

IUBMX055::IUBMX055(IUI2C *iuI2C) :
  IUABCSensor(),
  m_newData(false),
  m_accelScale(defaultAccelScale),
  m_filteredData(false),
  m_accelBandwidth(defaultAccelBandwidth)
{
  m_iuI2C = iuI2C;
  m_samplingRate = defaultSamplingRate;
}

/**
 * Set the scale then recompute resolution
 */
void IUBMX055::setAccelScale(IUBMX055::accelScaleOption scale)
{
  m_accelScale = scale;
  m_iuI2C->writeByte(ACC_ADDRESS, ACC_PMU_RANGE, (uint8_t) m_accelScale & 0x0F);
  computeAccelResolution();
}

/**
 * Set the accelerometer bandwidth
 */
void IUBMX055::setAccelBandwidth(IUBMX055::accelBandwidthOption bandwidth)
{
  m_accelBandwidth = bandwidth;
  m_iuI2C->writeByte(ACC_ADDRESS, ACC_PMU_BW, (uint8_t) m_accelBandwidth & 0x0F);
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
void IUBMX055::useFilteredData(IUBMX055::accelBandwidthOption bandwidth)
{
  m_filteredData = true;
  setAccelBandwidth(bandwidth);
  m_iuI2C->writeByte(ACC_ADDRESS, ACC_D_HBW, 0x00);
}

/**
 * Use unfiltered acceleration data
 * The unfiltered data is sampled with 2kHz.
 */
void IUBMX055::useUnfilteredData()
{
  m_iuI2C->writeByte(ACC_ADDRESS, ACC_D_HBW, 0x01);
  m_filteredData = false;
}

/**
 * Reset BMX055 configuration and return to normal power mode
 */
void IUBMX055::accelSoftReset()
{
 m_iuI2C->writeByte(ACC_ADDRESS,  ACC_BGW_SOFTRESET, 0xB6);  // reset accelerometer
 delay(1000); // Wait for all registers to reset
}

/**
 * Set accelResolution depending on accelScale value
 * Possible accelerometer scales (and their register bit settings) are:
 * 2 Gs (0011), 4 Gs (0101), 8 Gs (1000), and 16 Gs  (1100)
 * The accelResolution works for Q15 value (since accelData is recorded as 16bit).
 * To get float value, we must divide by 2^15.
 */
void IUBMX055::computeAccelResolution() {
  switch (m_accelScale)
  {
   // Possible accelerometer scales (and their register bit settings) are:
  // 2 Gs (0011), 4 Gs (0101), 8 Gs (1000), and 16 Gs  (1100).
    case AFS_2G:
          m_accelResolution = 2.0f;
          break;
    case AFS_4G:
          m_accelResolution = 4.0f;
          break;
    case AFS_8G:
          m_accelResolution = 8.0f;
          break;
    case AFS_16G:
          m_accelResolution = 16.0f;
          break;
  }
}

bool IUBMX055::checkAccelerometerWhoAmI()
{
  return m_iuI2C->checkComponentWhoAmI("BMX055 ACC", ACC_ADDRESS, ACC_WHO_AM_I, ACC_I_AM);
}

bool IUBMX055::checkGyroscopeWhoAmI()
{
  return m_iuI2C->checkComponentWhoAmI("BMX055 GYRO", GYRO_ADDRESS, GYRO_WHO_AM_I, GYRO_I_AM);
}

bool IUBMX055::checkMagnetometerWhoAmI()
{ 
  m_iuI2C->writeByte(MAG_ADDRESS, MAG_PWR_CNTL1, 0x01); // wake up magnetometer first thing
  return m_iuI2C->checkComponentWhoAmI("BMX055 MAG", MAG_ADDRESS, MAG_WHO_AM_I, MAG_I_AM);
}

/**
 * Ping the device address and, if the answer is correct, initialize it
 * This is a good test of communication.
 */
void IUBMX055::wakeUp()
{
  bool iAmMyself = checkAccelerometerWhoAmI();
  //iAmMyself &= checkGyroscopeWhoAmI();
  //iAmMyself &= checkMagnetometerWhoAmI();
  
  if(iAmMyself)
  {
    initSensor();
  }
  else
  {
    m_iuI2C->setErrorMessage("BMXERR");
  }
  delay(15);

  /* // Disable Gyro and mag in current firmware
  m_iuI2C->checkComponentWhoAmI("BMX055 GYRO", GYRO_ADDRESS, GYRO_WHO_AM_I, GYRO_I_AM)
  m_iuI2C->checkComponentWhoAmI("BMX055 MAG", MAG_ADDRESS, MAG_WHO_AM_I, MAG_I_AM)
  */

}

//TODO: Fix initSensor
/**
 * Initialize the device by configurating the accelerometer, gyroscope and sample rates
 */
void IUBMX055::initSensor()
{
  configureAccelerometer();
  configureGyroscope();
  configureMagnometer(magMode::regular);

  delay(100);
  if (setupDebugMode) { debugPrint(F("BMX055 initialized successfully.\n")); }
}

void IUBMX055::configureAccelerometer()
{
  accelSoftReset();
 
  // Configure accelerometer
  setAccelScale(m_accelScale);
  useUnfilteredData();

  m_iuI2C->writeByte(ACC_ADDRESS, ACC_INT_EN_1, 0x10);      // Enable ACC data ready interrupt
  m_iuI2C->writeByte(ACC_ADDRESS, ACC_INT_OUT_CTRL, 0x04);  // Set interrupts push-pull, active high for INT1 and INT2
  //m_iuI2C->writeByte(ACC_ADDRESS, ACC_INT_MAP_1, 0x02);     // Define INT1 (intACC1) as ACC data ready interrupt
  m_iuI2C->writeByte(ACC_ADDRESS, ACC_INT_MAP_1, 0x80);     // Define INT2 (intACC2) as ACC data ready interrupt
}

void IUBMX055::configureGyroscope()
{
  m_iuI2C->writeByte(GYRO_ADDRESS, GYRO_RANGE, defaultGyroScale);    // set GYRO FS range
  m_iuI2C->writeByte(GYRO_ADDRESS, GYRO_BW, defaultGyroBandwidth);   // set GYRO ODR and Bandwidth
}

void IUBMX055::configureMagnometer(IUBMX055::magMode mMode)
{
  m_iuI2C->writeByte(MAG_ADDRESS, MAG_PWR_CNTL1, 0x82);              // Softreset magnetometer, ends up in sleep mode
  delay(100);
  //TODO remove?
  m_iuI2C->writeByte(MAG_ADDRESS, MAG_PWR_CNTL1, 0x01); // Wake up magnetometer
  delay(100);
  m_iuI2C->writeByte(MAG_ADDRESS, MAG_PWR_CNTL2, MODR::MODR_10Hz << 3); // Normal mode

// Set up four standard configurations for the magnetometer
  switch (mMode)
  {
    case magMode::lowPower:
          m_iuI2C->writeByte(MAG_ADDRESS, MAG_REP_XY, 0x01);  // 3 repetitions (oversampling)
          m_iuI2C->writeByte(MAG_ADDRESS, MAG_REP_Z,  0x02);  // 3 repetitions (oversampling)
          break;
    case magMode::regular:
          m_iuI2C->writeByte(MAG_ADDRESS, MAG_REP_XY, 0x04);  //  9 repetitions (oversampling)
          m_iuI2C->writeByte(MAG_ADDRESS, MAG_REP_Z,  0x16);  // 15 repetitions (oversampling)
          break;
    case magMode::enhancedRegular:
          m_iuI2C->writeByte(MAG_ADDRESS, MAG_REP_XY, 0x07);  // 15 repetitions (oversampling)
          m_iuI2C->writeByte(MAG_ADDRESS, MAG_REP_Z,  0x22);  // 27 repetitions (oversampling)
          break;
    case magMode::highAccuracy:
          m_iuI2C->writeByte(MAG_ADDRESS, MAG_REP_XY, 0x17);  // 47 repetitions (oversampling)
          m_iuI2C->writeByte(MAG_ADDRESS, MAG_REP_Z,  0x51);  // 83 repetitions (oversampling)
          break;
  }
}

void IUBMX055::doAcellFastCompensation(float * dest1)
{
  m_iuI2C->writeByte(ACC_ADDRESS, ACC_OFC_CTRL, 0x80); // set all accel offset compensation registers to zero
  m_iuI2C->writeByte(ACC_ADDRESS, ACC_OFC_SETTING, 0x20);  // set offset targets to 0, 0, and +1 g for x, y, z axes

  uint8_t offsetCmd[3] = {0x20, 0x40, 0x60}; // x, y and z axis
  byte c = 0;
  for (int i = 0; i < 3; i++)
  {
    m_iuI2C->writeByte(ACC_ADDRESS, ACC_OFC_CTRL, offsetCmd[i]);   // calculate x, y or z axis offset
    c = m_iuI2C->readByte(ACC_ADDRESS, ACC_OFC_CTRL);              // check if fast calibration complete
    while(!(c & 0x10))                                             // While not complete, wait
    {
      delay(10);
      c = m_iuI2C->readByte(ACC_ADDRESS, ACC_OFC_CTRL);
    }
  }
  // Acceleration bias on 8bit int - resolution is +/- 1g
  int8_t compx = m_iuI2C->readByte(ACC_ADDRESS, ACC_OFC_OFFSET_X);
  int8_t compy = m_iuI2C->readByte(ACC_ADDRESS, ACC_OFC_OFFSET_Y);
  int8_t compz = m_iuI2C->readByte(ACC_ADDRESS, ACC_OFC_OFFSET_Z);

  // Convert to G in 12bit (for consistance with the accel readings, that are on 12bits)
  m_accelBias[0] = (uint16_t) compx << 4;
  m_accelBias[1] = (uint16_t) compy << 4;
  m_accelBias[2] = (uint16_t) compz << 4;
  if (setupDebugMode)
  {
    debugPrint("accel biases (mG):");
    debugPrint(1000 * q4_11ToFloat(m_accelBias[0]));
    debugPrint(1000 * q4_11ToFloat(m_accelBias[1]));
    debugPrint(1000 * q4_11ToFloat(m_accelBias[2]));
  }
}


/* ==================== Data Collection and Feature Calculation functions ======================== */

/**
 * Asynchronously read acceleration data and and call processAccelData as callback
 * Data is read from device as 2 bytes: LSB first (4 bits to use) then MSB (8 bits to use)
 * 4 last bits of LSB byte are used as flags (new data, etc).
 */
void IUBMX055::readAccelData()
{
  // Read the six raw data registers into data array
  if(!m_iuI2C->readBytes(ACC_ADDRESS, ACC_D_X_LSB, 6, &m_rawAccelBytes[0], processAccelData))
  {
    if (callbackDebugMode) { debugPrint("Skip accel read"); }
  }
}

/**
 * Process acceleration data and store it as a Q4.11 (signed 4-integer-bit and 11-fractionnal-bit)
 * Data is read from device as 2 bytes: LSB first (4 bits to use) then MSB (8 bits to use)
 * 4 last bits of LSB byte are used as flags (new data, etc).
 */
void IUBMX055::processAccelData(uint8_t wireStatus)
{
  if(!((m_rawAccelBytes[0] & 0x01) && (m_rawAccelBytes[2] & 0x01) && (m_rawAccelBytes[4] & 0x01)))
  {
    return; // Check that all 3 axes have new data, if not return
  }
  for (uint8_t i = 0; i < 3; ++i)
  {
    // Convert to Q15: LSB = LLLLXXXX, MSB = MMMMMMMMM => rawAccel = MMMMMMMMLLLL0000
    // use 8 bits of MSB and only the 4 left-most bits of LSB (NB Casting has precedence over << or >> operators)
    m_rawAccel[i] = (int16_t) (((int16_t)m_rawAccelBytes[2 * i + 1] << 8) | (m_rawAccelBytes[2 * i] & 0xF0));
    
    // Convert to Q4.11 (divide by 2^4) without losing precision since it was originally 12bit data, and then
    // multiply by resolution to have a measure in G and add the bias
    m_accelData[i] = m_rawAccel[i] * m_accelResolution / 16 + m_accelBias[i];
  }
  // Serial.print(m_accelData[0]); Serial.print(' '); Serial.print(m_accelData[1]); Serial.print(' '); Serial.println(m_accelData[2]);
}


void IUBMX055::readData()
{
  readAccelData();
  m_newData = true;
}


void IUBMX055::sendToReceivers()
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
        case (uint8_t) dataSendOption::xAccel:
          m_receivers[i]->receiveScalar(m_receiverSourceIndex[i], m_accelData[0]);
          break;
        case (uint8_t) dataSendOption::yAccel:
          m_receivers[i]->receiveScalar(m_receiverSourceIndex[i], m_accelData[1]);
          break;
        case (uint8_t) dataSendOption::zAccel:
          m_receivers[i]->receiveScalar(m_receiverSourceIndex[i], m_accelData[2]);
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
 * Dump acceleration data to serial via I2C - accel unit is G, in float format
 * NB: We want to do this in *DATA COLLECTION* mode
 */
void IUBMX055::dumpDataThroughI2C()
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
    accel = q4_11ToFloat(m_accelData[i]);
    data = (byte *) &accel;
    m_iuI2C->port->write(data, 4);
  }
  m_iuI2C->port->flush();
  m_newData = false;
}

/**
 * Dump acceleration data to serial via I2C - accel unit is G, in float format
 * NB: We want to do this in *DATA COLLECTION* mode, when debugMode is true
 */
void IUBMX055::dumpDataForDebugging()
{
  if (!m_newData)
  {
    return;
  }
  m_iuI2C->port->print("AX: ");
  m_iuI2C->port->println(q4_11ToFloat(m_accelData[0]), 4);
  m_iuI2C->port->print("AY: ");
  m_iuI2C->port->println(q4_11ToFloat(m_accelData[1]), 4);
  m_iuI2C->port->print("AZ: ");
  m_iuI2C->port->println(q4_11ToFloat(m_accelData[2]), 4);
  m_iuI2C->port->flush();
  m_newData = false;
}

/* ==================== Update and Control Functions =============================== */
/*
 * The following functions check and read the I2C wire buffer to see if configuration updates
 * have been received. If so, the updates are done.
 * Functions return true if an update happened.
 */


/* ====================== Diagnostic Functions, only active when setupDebugMode = true ====================== */

/*
 * Show calibration info
 */
void IUBMX055::exposeCalibration()
{
  #ifdef DEBUGMODE
  debugPrint(F("Calibration data: "));
  debugPrint(F("Accel resolution (G): "));
  debugPrint(m_accelResolution);
  debugPrint(F("Accel bias (q15_t): "));
  for (uint8_t i = 0; i < 3; ++i)
  {
    debugPrint(m_accelBias[i], false);
    debugPrint(", ", false);
  }
  debugPrint(' ');
  #endif
}

