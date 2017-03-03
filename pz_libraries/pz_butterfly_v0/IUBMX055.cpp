#include "IUBMX055.h"


IUBMX055::IUBMX055(IUI2C iuI2C, IUBLE iuBLE) : m_iuI2C(iuI2C), m_iuBLE(iuBLE), m_energy(0)
{
  for (int i=0; i < 3; i++)
  {
    m_rawAccel[i] = 0;
    m_maxAccelIndex[i] = 0;
    m_accelBias[i] = 0;
  }
  m_accelBW = 0x08 | ABW_16Hz;
  computeAccelResolution();
}

/**
 * Set the scale then recompute resolution
 */
void IUBMX055::setAccelScale(uint8_t val)
{
  m_accelScale = val;
  computeAccelResolution();
}


void IUBMX055::computeAccelResolution() {
  switch (m_accelScale)
  {
   // Possible accelerometer scales (and their register bit settings) are:
  // 2 Gs (0011), 4 Gs (0101), 8 Gs (1000), and 16 Gs  (1100). 
  // BMX055 ACC data is signed 12 bit
    case AFS_2G:
          m_accelResolution = 2.0f/2048.0f;
          break;
    case AFS_4G:
          m_accelResolution = 4.0f/2048.0f;
          break;
    case AFS_8G:
          m_accelResolution = 8.0f/2048.0f;
          break;
    case AFS_16G:
          m_accelResolution = 16.0f/2048.0f;
          break;
  }
}


/**
 * Ping the device address and, if the answer is correct, initialize it
 * This is a good test of communication.
 */
void IUBMX055::wakeUp()
{ 
  m_iuI2C.port->println("BMX055 accelerometer...");
  
  if(m_iuI2C.checkComponentWhoAmI("BMX055 ACC", ACC_ADDRESS, ACC_WHO_AM_I, ACC_I_AM))
  {
    initSensor(); // Initialize Accelerometer
  }
  else
  {
    m_iuI2C.setErrorMessage("BMXERR");
  }
  delay(15);

  /* // Disable Gyro and mag in current firmware
  m_iuI2C.checkComponentWhoAmI("BMX055 GYRO", GYRO_ADDRESS, GYRO_WHO_AM_I, GYRO_I_AM)
  m_iuI2C.checkComponentWhoAmI("BMX055 MAG", MAG_ADDRESS, MAG_WHO_AM_I, MAG_I_AM)  
  */
  
}

//TODO: Fix initSensor
/**
 * Initialize the device by configurating the accelerometer, gyroscope and sample rates
 */
void IUBMX055::initSensor()
{
    
   // start with all sensors in default mode with all registers reset
   writeByte(BMX055_ACC_ADDRESS,  BMX055_ACC_BGW_SOFTRESET, 0xB6);  // reset accelerometer
   delay(1000); // Wait for all registers to reset 

   // Configure accelerometer
   writeByte(BMX055_ACC_ADDRESS, BMX055_ACC_PMU_RANGE, m_accelScale & 0x0F); // Set accelerometer full range
   writeByte(BMX055_ACC_ADDRESS, BMX055_ACC_PMU_BW, m_accelBW & 0x0F);     // Set accelerometer bandwidth
   writeByte(BMX055_ACC_ADDRESS, BMX055_ACC_D_HBW, 0x00);              // Use filtered data

   writeByte(BMX055_ACC_ADDRESS, BMX055_ACC_INT_EN_1, 0x10);           // Enable ACC data ready interrupt
   writeByte(BMX055_ACC_ADDRESS, BMX055_ACC_INT_OUT_CTRL, 0x04);       // Set interrupts push-pull, active high for INT1 and INT2
   //writeByte(BMX055_ACC_ADDRESS, BMX055_ACC_INT_MAP_1, 0x02);        // Define INT1 (intACC1) as ACC data ready interrupt
   writeByte(BMX055_ACC_ADDRESS, BMX055_ACC_INT_MAP_1, 0x80);          // Define INT2 (intACC2) as ACC data ready interrupt

   //writeByte(BMX055_ACC_ADDRESS, BMX055_ACC_BGW_SPI3_WDT, 0x06);       // Set watchdog timer for 50 ms
 
  //Configure Gyro
  // start by resetting gyro, better not since it ends up in sleep mode?!
  //writeByte(BMX055_GYRO_ADDRESS, BMX055_GYRO_BGW_SOFTRESET, 0xB6); // reset gyro
  //delay(100);
  // Three power modes, 0x00 Normal, 
  // set bit 7 to 1 for suspend mode, set bit 5 to 1 for deep suspend mode
  // sleep duration in fast-power up from suspend mode is set by bits 1 - 3
  // 000 for 2 ms, 111 for 20 ms, etc.
  //writeByte(BMX055_GYRO_ADDRESS, BMX055_GYRO_LPM1, 0x00);  // set GYRO normal mode
  // set GYRO sleep duration for fast power-up mode to 20 ms, for duty cycle of 50%
  //writeByte(BMX055_ACC_ADDRESS, BMX055_GYRO_LPM1, 0x0E);  
  // set bit 7 to 1 for fast-power-up mode,  gyro goes quickly to normal mode upon wake up
  // can set external wake-up interrupts on bits 5 and 4
  // auto-sleep wake duration set in bits 2-0, 001 4 ms, 111 40 ms
  //writeByte(BMX055_GYRO_ADDRESS, BMX055_GYRO_LPM2, 0x00);  // set GYRO normal mode
  // set gyro to fast wake up mode, will sleep for 20 ms then run normally for 20 ms 
  // and collect data for an effective ODR of 50 Hz, other duty cycles are possible but there
  // is a minimum wake duration determined by the bandwidth duration, e.g.,  > 10 ms for 23Hz gyro bandwidth
  //writeByte(BMX055_ACC_ADDRESS, BMX055_GYRO_LPM2, 0x87);   

  writeByte(BMX055_GYRO_ADDRESS, BMX055_GYRO_RANGE, Gscale);  // set GYRO FS range
  writeByte(BMX055_GYRO_ADDRESS, BMX055_GYRO_BW, GODRBW);     // set GYRO ODR and Bandwidth

  //writeByte(BMX055_GYRO_ADDRESS, BMX055_GYRO_INT_EN_0, 0x80);  // enable data ready interrupt
  //writeByte(BMX055_GYRO_ADDRESS, BMX055_GYRO_INT_EN_1, 0x04);  // select push-pull, active high interrupts
  //writeByte(BMX055_GYRO_ADDRESS, BMX055_GYRO_INT_MAP_1, 0x80); // select INT3 (intGYRO1) as GYRO data ready interrupt 
  
  //writeByte(BMX055_GYRO_ADDRESS, BMX055_GYRO_BGW_SPI3_WDT, 0x06); // Enable watchdog timer for I2C with 50 ms window


  // Configure magnetometer 
  writeByte(BMX055_MAG_ADDRESS, BMX055_MAG_PWR_CNTL1, 0x82);  // Softreset magnetometer, ends up in sleep mode
  delay(100);
  writeByte(BMX055_MAG_ADDRESS, BMX055_MAG_PWR_CNTL1, 0x01); // Wake up magnetometer
  delay(100);

  writeByte(BMX055_MAG_ADDRESS, BMX055_MAG_PWR_CNTL2, MODR << 3); // Normal mode
  //writeByte(BMX055_MAG_ADDRESS, BMX055_MAG_PWR_CNTL2, MODR << 3 | 0x02); // Forced mode
  
  //writeByte(BMX055_MAG_ADDRESS, BMX055_MAG_INT_EN_2, 0x84); // Enable data ready pin interrupt, active high
  
  // Set up four standard configurations for the magnetometer
  switch (Mmode)
  {
    case lowPower:
         // Low-power
          writeByte(BMX055_MAG_ADDRESS, BMX055_MAG_REP_XY, 0x01);  // 3 repetitions (oversampling)
          writeByte(BMX055_MAG_ADDRESS, BMX055_MAG_REP_Z,  0x02);  // 3 repetitions (oversampling)
          break;
    case Regular:
          // Regular
          writeByte(BMX055_MAG_ADDRESS, BMX055_MAG_REP_XY, 0x04);  //  9 repetitions (oversampling)
          writeByte(BMX055_MAG_ADDRESS, BMX055_MAG_REP_Z,  0x16);  // 15 repetitions (oversampling)
          break;
    case enhancedRegular:
          // Enhanced Regular
          writeByte(BMX055_MAG_ADDRESS, BMX055_MAG_REP_XY, 0x07);  // 15 repetitions (oversampling)
          writeByte(BMX055_MAG_ADDRESS, BMX055_MAG_REP_Z,  0x22);  // 27 repetitions (oversampling)
          break;
    case highAccuracy:
          // High Accuracy
          writeByte(BMX055_MAG_ADDRESS, BMX055_MAG_REP_XY, 0x17);  // 47 repetitions (oversampling)
          writeByte(BMX055_MAG_ADDRESS, BMX055_MAG_REP_Z,  0x51);  // 83 repetitions (oversampling)
          break;
  }
  delay(100);
  m_iuI2C.port->print("BMX055 initialized successfully.\n\n");
  m_iuI2C.port->flush();
}


void IUBMX055::doAcellFastCompensation(float * dest1) 
{
  writeByte(ACC_ADDRESS, ACC_OFC_CTRL, 0x80); // set all accel offset compensation registers to zero
  writeByte(ACC_ADDRESS, ACC_OFC_SETTING, 0x20);  // set offset targets to 0, 0, and +1 g for x, y, z axes
  writeByte(ACC_ADDRESS, ACC_OFC_CTRL, 0x20); // calculate x-axis offset

  byte c = readByte(ACC_ADDRESS, ACC_OFC_CTRL);
  while(!(c & 0x10))
  {
    // check if fast calibration complete
    c = readByte(ACC_ADDRESS, ACC_OFC_CTRL);
    delay(10);
  }
  writeByte(ACC_ADDRESS, ACC_OFC_CTRL, 0x40); // calculate y-axis offset

  c = readByte(ACC_ADDRESS, ACC_OFC_CTRL);
  while(!(c & 0x10))
  {
    // check if fast calibration complete
    c = readByte(ACC_ADDRESS, ACC_OFC_CTRL);
    delay(10);
  }
  writeByte(ACC_ADDRESS, ACC_OFC_CTRL, 0x60); // calculate z-axis offset

  c = readByte(ACC_ADDRESS, ACC_OFC_CTRL);
  while(!(c & 0x10))
  {
    // check if fast calibration complete
    c = readByte(ACC_ADDRESS, ACC_OFC_CTRL);
    delay(10);
  }

  int8_t compx = readByte(ACC_ADDRESS, ACC_OFC_OFFSET_X);
  int8_t compy = readByte(ACC_ADDRESS, ACC_OFC_OFFSET_Y);
  int8_t compz = readByte(ACC_ADDRESS, ACC_OFC_OFFSET_Z);

  m_accelBias[0] = (float) compx / 128.0f; // accleration bias in g
  m_accelBias[1] = (float) compy / 128.0f; // accleration bias in g
  m_accelBias[2] = (float) compz / 128.0f; // accleration bias in g
  if (!(m_iuI2c.isSilent()))
  {
    m_iuI2cport->println("accel biases (mg)");
    m_iuI2cport->println(1000. * m_accelBias[0]);
    m_iuI2cport->println(1000. * m_accelBias[1]);
    m_iuI2cport->println(1000. * m_accelBias[2]);
  }
}

/* ==================== Update and Control Functions =============================== */
/*
 * The following functions check and read the I2C wire buffer to see if configuration updates
 * have been received. If so, the updates are done.
 * Functions return true if an update happened.
 */



/* ==================== Data Collection and Feature Calculation functions ======================== */

/**
 * Read acceleration data and store it (in m_rawAccel)
 */
void IUBMX055::readAccelData()
{
  uint8_t rawData[6];  // x/y/z accel register data stored here
  m_iuI2C.readBytes(ACC_ADDRESS, ACC_D_X_LSB, 6, &rawData[0]);       // Read the six raw data registers into data array
  if((rawData[0] & 0x01) && (rawData[2] & 0x01) && (rawData[4] & 0x01)) // Check that all 3 axes have new data
  { 
    // Turn the MSB and LSB into a signed 12-bit value
    m_rawAccel[0] = (int16_t) (((int16_t)rawData[1] << 8) | rawData[0]) >> 4;
    m_rawAccel[1] = (int16_t) (((int16_t)rawData[3] << 8) | rawData[2]) >> 4;  
    m_rawAccel[2] = (int16_t) (((int16_t)rawData[5] << 8) | rawData[4]) >> 4; 
  }
}

/**
 * Push acceleration in the batch array to later compute the FFT
 * NB: We want to do this in *RUN* mode
 */
void IUBMX055::pushDataToBatch(uint8_t buffer_record_index, uint32_t index)
{
  for (int i=0; i < 3; i++)
  {
    m_accelBatch[i][buffer_record_index][index] = m_rawAccel[i];
  }
}

/**
 * Dump acceleration data to serial via I2C 
 * NB: We want to do this in *DATA COLLECTION* mode
 */
void IUBMX055::dumpDataThroughI2C()
{
  for (int i=0; i < 3; i ++)
  {
    m_a[i] = (float)m_rawAccel[i] * m_resolution + m_bias[i];
  }

  byte* axb = (byte *) &m_a[0];
  byte* ayb = (byte *) &m_a[1];
  byte* azb = (byte *) &m_a[2];

  m_iuI2C.port->write(axb, 4);
  m_iuI2C.port->write(ayb, 4);
  m_iuI2C.port->write(azb, 4);
  m_iuI2C.port->flush();
}

/**
 * Send raw data through BLE device (eg to an Android app for FFT calculation)
 */
void IUBMX055::showRecordFFT(uint8_t buffer_compute_index, String mac_address)
{
  for (int i = 0; i < 3; i++)
  {
    m_iuBLE.port->print("REC,");
    m_iuBLE.port->print(mac_address);
    switch(i)
    {
      case 0: m_iuBLE.port->print(",X,");
      case 1: m_iuBLE.port->print(",Y,");
      case 2: m_iuBLE.port->print(",Z,");
    }
    for (int j = 0; j < MAX_INTERVAL; j++)
    {
      m_iuBLE.port->print(LSB_to_ms2(m_accelBatch[i][buffer_compute_index][j]));
      m_iuBLE.port->print(",");
      m_iuBLE.port->flush();
    }
    m_iuBLE.port->print(";");
    m_iuBLE.port->flush();
  }
}

/**
 * Convert least significant byte acceleration data to m/s2 using device resolution
 * @param accelLSB lest significant byte data to convert
 */
float IUBMX055::LSB_to_ms2(int16_t accelLSB)
{
  return ((float)accelLSB * m_resolution * 9.8);
}

/**
 * Compute the acceleration energy, return it and store it in m_energy
 * @param buffer_compute_index index of buffer for record / compute switching 
 * @param counterTarget => ACCEL_COUNTER_TARGET
 * @param batchSize => featureBatchSize
 */
float IUBMX055::computeEnergy (uint8_t buffer_compute_index, uint32_t batchSize, uint16_t counterTarget) {
  // Take q15_t buffer as float buffer for computation
  float* compBuffer = (float*)&m_rfftBuffer;
  // Reduce re-computation of variables
  int zind = ENERGY_INTERVAL * 2;
  uint32_t idx = batchSize / counterTarget - ENERGY_INTERVAL;

  // Copy from accel batches to compBuffer
  arm_q15_to_float(&m_accelBatch[X][buffer_compute_index][idx], compBuffer, ENERGY_INTERVAL);
  arm_q15_to_float(&m_accelBatch[Y][buffer_compute_index][idx], &compBuffer[ENERGY_INTERVAL], ENERGY_INTERVAL);
  arm_q15_to_float(&m_accelBatch[Z][buffer_compute_index][idx], &compBuffer[zind], ENERGY_INTERVAL);

  // Scale all entries by resolution AND 2^15; the entries were NOT in q1.15 format.
  arm_scale_f32(compBuffer, m_resolution * 32768, compBuffer, ENERGY_INTERVAL * 3);

  // Apply accel biases
  arm_offset_f32(compBuffer, m_bias[X], compBuffer, ENERGY_INTERVAL);
  arm_offset_f32(&compBuffer[ENERGY_INTERVAL], m_bias[Z], &compBuffer[ENERGY_INTERVAL], ENERGY_INTERVAL);
  arm_offset_f32(&compBuffer[zind], m_bias[Z], &compBuffer[zind], ENERGY_INTERVAL);

  // Calculate average
  float ave_x(0), ave_y(0), ave_z(0);
  arm_mean_f32(compBuffer, ENERGY_INTERVAL, &ave_x);
  arm_mean_f32(&compBuffer[ENERGY_INTERVAL], ENERGY_INTERVAL, &ave_y);
  arm_mean_f32(&compBuffer[zind], ENERGY_INTERVAL, &ave_z);

  // Calculate energy.
  // NOTE: Using CMSIS-DSP functions to perform the following operation performed 2840 microseconds,
  //       while just using simple loop performed 2730 microseconds. Keep the latter.
  m_energy = 0;
  for (uint32_t i = 0; i < ENERGY_INTERVAL; i++) {
    m_energy += sq(compBuffer[i] - ave_x)
               +  sq(compBuffer[ENERGY_INTERVAL + i] - ave_y)
               +  sq(compBuffer[zind + i] - ave_z);
  }
  m_energy *= ENERGY_INTERVAL; //Necessary?
  return m_energy;
}

/**
 * Compute and return single axis acceleration RMS
 * Note: currently applies a corrective scaling factor (see getScalingFactor)
 * @param axis X, Y or Z
 */
float IUBMX055::computeAccelRMS(uint8_t buffer_compute_index, IUBMX055::Axis axis)
{
  return computeRMS(MAX_INTERVAL, m_accelBatch[axis][buffer_compute_index]);
}

/**
 * Compute and return single axis velocity using acceleration RMS and maxIndex
 * @param axis X, Y or Z
 * @param cutTreshold normal cutting threshold (ex featureNormalThreshold[0])
 */
float IUBMX055::computeVelocity(uint8_t buffer_compute_index, IUBMX055::Axis axis, float cutTreshold)
{
  if (m_energy < cutTreshold) {
    return 0; // level 0: not cutting
  }
  else {
    return computeAccelRMS(buffer_compute_index, axis) * 1000 / (2 * PI * m_maxAccelIndex[axis]);
  }
}

/**
 * Compute RFFT on acceleration data on given axis and update m_maxAccelIndex on this axis
 */
void IUBMX055::computeAccelRFFT(uint8_t buffer_compute_index, IUBMX055::Axis axis, q15_t *hamming_window)
{
  
  arm_mult_q15(m_accelBatch[axis][buffer_compute_index],
               hamming_window,
               m_accelBuffer[axis],
               NFFT);
  // RFFT
  arm_rfft_init_q15(&m_rfftInstance,
                    &m_cfftInstance,
                    NFFT, 0, 1);
  arm_rfft_q15(&m_rfftInstance,
               m_accelBuffer[axis],
               m_rfftBuffer);
  
  float flatness = 0;
  for (int i = 2; i < 512; i++) {
    if (m_rfftBuffer[i] > flatness) {
      flatness = m_rfftBuffer[i];
      m_maxAccelIndex[axis] = i;
    }
  }
}


