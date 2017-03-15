#include "IUBMX055.h"

char IUBMX055::sensorTypes[IUBMX055::sensorTypeCount] =
{
  IUABCSensor::sensorType_accelerometer,
  IUABCSensor::sensorType_gyroscope,
  IUABCSensor::sensorType_magnetometer
};

IUBMX055::IUBMX055(IUI2C *iuI2C) : IUABCSensor(), m_iuI2C(iuI2C)
{
  for (int i=0; i < 3; i++)
  {
    m_rawAccel[i] = 0;
    m_accelBias[i] = 0;
  }
  m_accelBW = 0x08 | ABW_16Hz;
  setAccelScale(AFS_2G);
  m_samplingRate = defaultSamplingRate;
}

/**
 * Set the scale then recompute resolution
 */
void IUBMX055::setAccelScale(uint8_t val)
{
  m_accelScale = val;
  computeAccelResolution();
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

/**
 * Ping the device address and, if the answer is correct, initialize it
 * This is a good test of communication.
 */
void IUBMX055::wakeUp()
{
  m_iuI2C->port->println("BMX055 accelerometer...");

  if(m_iuI2C->checkComponentWhoAmI("BMX055 ACC", ACC_ADDRESS, ACC_WHO_AM_I, ACC_I_AM))
  {
    initSensor(); // Initialize Accelerometer
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

   // start with all sensors in default mode with all registers reset
   m_iuI2C->writeByte(ACC_ADDRESS,  ACC_BGW_SOFTRESET, 0xB6);  // reset accelerometer
   delay(1000); // Wait for all registers to reset


  configureAccelerometer();
  configureGyroscope();
  configureMagnometer();

  delay(100);
  m_iuI2C->port->print("BMX055 initialized successfully.\n\n");
  m_iuI2C->port->flush();
}

void IUBMX055::configureAccelerometer()
{
  // Configure accelerometer
  m_iuI2C->writeByte(ACC_ADDRESS, ACC_PMU_RANGE, m_accelScale & 0x0F); // Set accelerometer full range
  m_iuI2C->writeByte(ACC_ADDRESS, ACC_PMU_BW, m_accelBW & 0x0F);     // Set accelerometer bandwidth;
  m_iuI2C->writeByte(ACC_ADDRESS, ACC_D_HBW, 0x00);              // Use filtered data

  m_iuI2C->writeByte(ACC_ADDRESS, ACC_INT_EN_1, 0x10);           // Enable ACC data ready interrupt
  m_iuI2C->writeByte(ACC_ADDRESS, ACC_INT_OUT_CTRL, 0x04);       // Set interrupts push-pull, active high for INT1 and INT2
  //writeByte(ACC_ADDRESS, ACC_INT_MAP_1, 0x02);        // Define INT1 (intACC1) as ACC data ready interrupt
  m_iuI2C->writeByte(ACC_ADDRESS, ACC_INT_MAP_1, 0x80);          // Define INT2 (intACC2) as ACC data ready interrupt
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
    while(!(c & 0x10))                                            // While not complete, wait
    {
      delay(10);
      c = m_iuI2C->readByte(ACC_ADDRESS, ACC_OFC_CTRL);
    }
  }
  // Acceleration bias on 8bit int - resolution is +/- 1g
  int8_t compx = m_iuI2C->readByte(ACC_ADDRESS, ACC_OFC_OFFSET_X);
  int8_t compy = m_iuI2C->readByte(ACC_ADDRESS, ACC_OFC_OFFSET_Y);
  int8_t compz = m_iuI2C->readByte(ACC_ADDRESS, ACC_OFC_OFFSET_Z);

  // Convert to m/s2 on q15_t (from 8bit to 16bit, multiply by 2**8 = 256)
  m_accelBias[0] = (q15_t) compx * 256; // accel bias in m/s2 Q15
  m_accelBias[1] = (q15_t) compy * 256; // accel bias in m/s2 Q15
  m_accelBias[2] = (q15_t) compz * 256; // accel bias in m/s2 Q15
  if (!(m_iuI2C->isSilent()))
  {
    m_iuI2C->port->println("accel biases (mG)");
    m_iuI2C->port->println(1000 * q15ToFloat(m_accelBias[0]));
    m_iuI2C->port->println(1000 * q15ToFloat(m_accelBias[1]));
    m_iuI2C->port->println(1000 * q15ToFloat(m_accelBias[2]));
  }
}


/* ==================== Data Collection and Feature Calculation functions ======================== */

/**
 * Read acceleration data and store it as a signed 16-bit integer (q15_t)
 * Data is read from device as 2 bytes: LSB first (4 bits to use) then MSB (8 bits to use)
 * 4 last bits of LSB byte are used as flags (new data, etc).
 */
void IUBMX055::readAccelData()
{
  // Read the six raw data registers into data array
  uint8_t rawData[6];
  m_iuI2C->readBytes(ACC_ADDRESS, ACC_D_X_LSB, 6, &rawData[0]);

  if(!((rawData[0] & 0x01) && (rawData[2] & 0x01) && (rawData[4] & 0x01)))
  {
    return; // Check that all 3 axes have new data, if not return
  }
  // Turn the LSB and MSB into a signed 16-bit value (q15_t)
  // NB: Casting has precedence over << or >> operators
  m_rawAccel[0] = (int16_t) (((int16_t)rawData[1] << 8) | rawData[0]) >> 4;
  m_rawAccel[1] = (int16_t) (((int16_t)rawData[3] << 8) | rawData[2]) >> 4;
  m_rawAccel[2] = (int16_t) (((int16_t)rawData[5] << 8) | rawData[4]) >> 4;
  // Multiply by resolution to have a measure in G, in q15_t format and add the bias
  m_accelData[0] = m_rawAccel[0] * m_accelResolution + m_accelBias[0];
  m_accelData[1] = m_rawAccel[1] * m_accelResolution + m_accelBias[1];
  m_accelData[2] = m_rawAccel[2] * m_accelResolution + m_accelBias[2];
}


void IUBMX055::readData()
{
  readAccelData();
}


void IUBMX055::sendToReceivers()
{
  for (int i = 0; i < m_receiverCount; i++)
  {
    if (m_receivers[i])
    {
      switch(m_toSend[i])
      {
        case dataSendOption::xAccel:
          m_receivers[i]->receive(m_receiverSourceIndex[i], m_accelData[0]);
          break;
        case dataSendOption::yAccel:
          m_receivers[i]->receive(m_receiverSourceIndex[i], m_accelData[1]);
          break;
        case dataSendOption::zAccel:
          m_receivers[i]->receive(m_receiverSourceIndex[i], m_accelData[2]);
          break;
        default:
          break;
      }
    }
  }
}

/**
 * Dump acceleration data to serial via I2C
 * NB: We want to do this in *DATA COLLECTION* mode
 */
void IUBMX055::dumpDataThroughI2C()
{
  byte* data;
  for (uint8_t i = 0; i < 3; i++)
  {
    // Stream float value as 4 bytes
    data = (byte *) &m_accelData[i];
    m_iuI2C->port->write(data, 4);
  }
  m_iuI2C->port->flush();
}

/* ==================== Update and Control Functions =============================== */
/*
 * The following functions check and read the I2C wire buffer to see if configuration updates
 * have been received. If so, the updates are done.
 * Functions return true if an update happened.
 */

