#include "IUMPU9250.h"


IUMPU9250::IUMPU9250(IUI2CTeensy iuI2C, IUBLE iuBLE) : m_iuI2C(iuI2C), m_iuBLE(iuBLE), m_energy(0)
{
  m_scale = AFS_2G;
  for (int i=0; i < 3; i++)
  {
    m_bias[i] = 0;
    m_rawAccel[i] = 0;
    m_maxIndex[i] = 0;
  }
  computeResolution();
  resetTargetSample();
}

IUMPU9250::IUMPU9250(IUI2CTeensy iuI2C, IUBLE iuBLE, IUMPU9250::ScaleOption scale) : m_iuI2C(iuI2C), m_iuBLE(iuBLE), m_energy(0)
{
  m_scale = scale;
  for (int i=0; i < 3; i++)
  {
    m_bias[i] = 0;
    m_rawAccel[i] = 0;
    m_maxIndex[i] = 0;
  }
  computeResolution();
  resetTargetSample();
}

IUMPU9250::~IUMPU9250()
{
  //dtor
}

/**
 * Set the scale then recompute resolution
 */
void IUMPU9250::setScale(uint8_t val)
{
  m_scale = val;
  computeResolution();
}

/**
 * Set the acc resolution value depending on the resolution scale
 */
void IUMPU9250::computeResolution()
{
  switch (m_scale)
  {
    // Possible accelerometer scales (and their register bit settings) are:
    // 2 Gs (0011), 4 Gs (0101), 8 Gs (1000), and 16 Gs  (1100).
    // TODO CHANGE: MPU data is signed 16 bit??
    case AFS_2G:
      m_resolution = 2.0 / 32768.0; // change from 2^11 to 2^15
      break;
    case AFS_4G:
      m_resolution = 4.0 / 32768.0; // is this even right? should it be 2^16?
      break;
    case AFS_8G:
      m_resolution = 8.0 / 32768.0;
      break;
    case AFS_16G:
      m_resolution = 16.0 / 32768.0;
      break;
  }
}

/** 
 * Se the SENtral MPU9250 to pass-through mode.
 * Note: SENtral is interfacing MPU9250 with the main CPU. It needs 
 * to be set to Pass-Through mode so that the MPU9250 can be accessed directly.
 */
void IUMPU9250::setSENtralToPassThroughMode()
{
  // First put SENtral in standby mode
  uint8_t c = m_iuI2C.readByte(EM7180_ADDRESS, EM7180_AlgorithmControl);
  m_iuI2C.writeByte(EM7180_ADDRESS, EM7180_AlgorithmControl, c | 0x01);
  // Verify standby status
  if (!m_iuI2C.isSilent())
  {
    m_iuBLE.port->println("SENtral in standby mode");
  }
  // Place SENtral in pass-through mode
  m_iuI2C.writeByte(EM7180_ADDRESS, EM7180_PassThruControl, 0x01);
  if (m_iuI2C.readByte(EM7180_ADDRESS, EM7180_PassThruStatus) & 0x01) {
    if (!m_iuI2C.isSilent())
    {
      m_iuBLE.port->println("SENtral in pass-through mode");
    }
  }
  else {
    m_iuBLE.port->println("ERROR! SENtral not in pass-through mode!");
  }
}

/**
 * Ping the device address and, if the answer is correct, initialize it
 */
void IUMPU9250::wakeUp()
{
  byte c = m_iuI2C.readByte(ADDRESS, WHO_AM_I); // Read WHO_AM_I register on MPU
  if (!m_iuI2C.isSilent())
  {
    m_iuI2C.port->print("MPU9250 I AM ");
    m_iuI2C.port->print(c, HEX);
    m_iuI2C.port->print(" I should be ");
    m_iuI2C.port->println(WHO_AM_I_ANSWER, HEX);
    m_iuI2C.port->println(" ");
    m_iuI2C.port->flush();
  }
  if (c == WHO_AM_I_ANSWER) { // correct
    initSensor();  // Initialize MPU9250 Accelerometer sensor
    computeResolution();  // Set the acc resolution value depending on the accel resolution selected
  }
  else
  {
    m_iuI2C.port->print("Could not connect to MPU9250: 0x");
    m_iuI2C.port->println(c, HEX);
    m_iuI2C.setErrorMessage("MPUERR");
  }
  delay(15);
}

/**
 * Initialize the device by configurating the accelerometer, gyroscope and sample rates
 */
void IUMPU9250::initSensor()
{
  // wake up device
  m_iuI2C.writeByte(ADDRESS, PWR_MGMT_1, 0x00); // Clear sleep mode bit (6), enable all sensors
  delay(100); // Wait for all registers to reset

  // get stable time source
  m_iuI2C.writeByte(ADDRESS, PWR_MGMT_1, 0x01);  // Auto select clock source to be PLL gyroscope reference if ready else
  delay(200);

  // disable gyroscope
  m_iuI2C.writeByte(ADDRESS, PWR_MGMT_2, 0x07);

  // Set sample rate = gyroscope output rate/(1 + SMPLRT_DIV)
  m_iuI2C.writeByte(ADDRESS, SMPLRT_DIV, 0x00);  // Use a 200 Hz rate; a rate consistent with the filter update rate
  // determined inset in CONFIG above

  // Set accelerometer full-scale range configuration
  byte c = m_iuI2C.readByte(ADDRESS, ACCEL_CONFIG); // get current ACCEL_CONFIG register value
  // c = c & ~0xE0; // Clear self-test bits [7:5]
  c = c & ~0x18;  // Clear accelerometer full scale bits [4:3]
  c = c | m_scale << 3; // Set full scale range for the accelerometer
  m_iuI2C.writeByte(ADDRESS, ACCEL_CONFIG, c); // Write new ACCEL_CONFIG register value

  // Set accelerometer sample rate configuration
  // It is possible to get a 4 kHz sample rate from the accelerometer by choosing 1 for
  // accel_fchoice_b bit [3]; in this case the bandwidth is 1.13 kHz

  //MODIFIED SAMPLE RATE
  c = m_iuI2C.readByte(ADDRESS, ACCEL_CONFIG2); // get current ACCEL_CONFIG2 register value
  c = c & ~0x0F; // Clear accel_fchoice_b (bit 3) and A_DLPFG (bits [2:0])
  c = c | 0x03;  // MODIFIED: Instead of 1k rate, 41hz bandwidth: we are doing 4k rate, 1.13K bandwidth
  m_iuI2C.writeByte(ADDRESS, ACCEL_CONFIG2, c); // Write new ACCEL_CONFIG2 register value
  // The accelerometer, gyro, and thermometer are set to 1 kHz sample rates,
  // but all these rates are further reduced by a factor of 5 to 200 Hz because of the SMPLRT_DIV setting

  // Configure Interrupts and Bypass Enable
  // Set interrupt pin active high, push-pull, hold interrupt pin level HIGH until interrupt cleared,
  // clear on read of INT_STATUS, and enable I2C_BYPASS_EN so additional chips
  // can join the I2C bus and all can be controlled by the Arduino as master
  m_iuI2C.writeByte(ADDRESS, INT_PIN_CFG, 0x22);
  m_iuI2C.writeByte(ADDRESS, INT_ENABLE, 0x01);  // Enable data ready (bit 0) interrupt
  delay(100);
  m_iuI2C.port->print("MPU-9250 initialized successfully.\n\n");
  m_iuI2C.port->flush();
}

/* ==================== Update and Control Functions =============================== */
/*
 * The following functions check and read the I2C wire buffer to see if configuration updates
 * have been received. If so, the updates are done.
 * Functions return true if an update happened.
 */

/**
 * Check the I2C wire buffer for accel range update
 */
bool IUMPU9250::updateAccelRangeFromI2C()
{
  String wireBuffer = m_iuI2C.getBuffer();
  // Change acceleration range, then change the scale
  if (wireBuffer.indexOf("Arange") > -1)
  {
    int Arangelocation = wireBuffer.indexOf("Arange");
    String Arange2 = wireBuffer.charAt(Arangelocation + 7);
    uint8_t Arange = (uint8_t) Arange2.toInt();
    setScale(Arange);
    return true;
  }
  return false;
}

/**
 * Check the I2C wire buffer for update of sampling rate
 */
bool IUMPU9250::updateSamplingRateFromI2C()
{
  String wireBuffer = m_iuI2C.getBuffer();
  if (wireBuffer.indexOf("accsr") > -1)
  {
    int accsrLocation = wireBuffer.indexOf("accsr");
    int target_sample_C = wireBuffer.charAt(accsrLocation + 6) - 48;
    int target_sample_D = wireBuffer.charAt(accsrLocation + 7) - 48;
    int target_sample_E = wireBuffer.charAt(accsrLocation + 8) - 48;
    int target_sample_F = wireBuffer.charAt(accsrLocation + 9) - 48;

    int target_sample_vib1 = (target_sample_C * 1000 + target_sample_D * 100 + target_sample_E * 10 + target_sample_F);
    switch (target_sample_vib1) {
      case 8:
        m_targetSample = 7.8125;
        break;
      case 16:
        m_targetSample = 15.625;
        break;
      case 31:
        m_targetSample = 31.25;
        break;
      case 63:
        m_targetSample = 62.5;
        break;
      case 125:
        m_targetSample = 125;
        break;
      case 250:
        m_targetSample = 250;
        break;
      case 500:
        m_targetSample = 500;
        break;
      case 1000:
        m_targetSample = 1000;
        break;
    }
    return true;
  }
  return false;
}

/* ==================== Data Collection and Feature Calculation functions ======================== */

/**
 * Read acceleration data and writes it to m_rawAccel
 * @param destination an array[3] to receive x, y, z accelaration
 */
void IUMPU9250::readAccelData()
{
  uint8_t rawData[6];  // x/y/z accel register data stored here
  m_iuI2C.readBytes(ADDRESS, OUT_H, 6, &rawData[0]);  // Read the six raw data registers into data array
  m_rawAccel[0] = ((int16_t)rawData[0] << 8) | rawData[1] ;  // Turn the MSB and LSB into a signed 16-bit value
  m_rawAccel[1] = ((int16_t)rawData[2] << 8) | rawData[3] ;
  m_rawAccel[2] = ((int16_t)rawData[4] << 8) | rawData[5] ;
}

/**
 * Push acceleration in the batch array to later compute the FFT
 * NB: We want to do this in *RUN* mode
 */
void IUMPU9250::pushDataToBatch(uint8_t buffer_record_index, uint32_t index)
{
  for (int i=0; i < 3; i++)
  {
    m_batch[i][buffer_record_index][index] = m_rawAccel[i];
  }
}

/**
 * Dump acceleration data to serial via I2C 
 * NB: We want to do this in *DATA COLLECTION* mode
 */
void IUMPU9250::dumpDataThroughI2C()
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
void IUMPU9250::showRecordFFT(uint8_t buffer_compute_index, String mac_address)
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
      m_iuBLE.port->print(LSB_to_ms2(m_batch[i][buffer_compute_index][j]));
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
float IUMPU9250::LSB_to_ms2(int16_t accelLSB)
{
  return ((float)accelLSB * m_resolution * 9.8);
}

/**
 * Compute corrective scaling factor on given axis
 */
float IUMPU9250::getScalingFactor(IUMPU9250::Axis axis)
{
  return 1 + 0.00266 * m_maxIndex[axis] + 0.000106 * sq(m_maxIndex[axis]);
}

/**
 * Compute the acceleration energy, return it and store it in m_energy
 * @param buffer_compute_index index of buffer for record / compute switching 
 * @param counterTarget => ACCEL_COUNTER_TARGET
 * @param batchSize => featureBatchSize
 */
float IUMPU9250::computeEnergy (uint8_t buffer_compute_index, uint32_t batchSize, uint16_t counterTarget) {
  // Take q15_t buffer as float buffer for computation
  float* compBuffer = (float*)&m_rfftBuffer;
  // Reduce re-computation of variables
  int zind = ENERGY_INTERVAL * 2;
  uint32_t idx = batchSize / counterTarget - ENERGY_INTERVAL;

  // Copy from accel batches to compBuffer
  arm_q15_to_float(&m_batch[X][buffer_compute_index][idx], compBuffer, ENERGY_INTERVAL);
  arm_q15_to_float(&m_batch[Y][buffer_compute_index][idx], &compBuffer[ENERGY_INTERVAL], ENERGY_INTERVAL);
  arm_q15_to_float(&m_batch[Z][buffer_compute_index][idx], &compBuffer[zind], ENERGY_INTERVAL);

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
float IUMPU9250::computeAccelRMS(uint8_t buffer_compute_index, IUMPU9250::Axis axis)
{
  float   accelRMS = 0;
  accelRMS = computeRMS(MAX_INTERVAL, m_batch[axis][buffer_compute_index]);
  accelRMS *= getScalingFactor(axis);
  return accelRMS;
}

/**
 * Compute and return single axis velocity using acceleration RMS and maxIndex
 * @param axis X, Y or Z
 * @param cutTreshold normal cutting threshold (ex featureNormalThreshold[0])
 */
float IUMPU9250::computeVelocity(uint8_t buffer_compute_index, IUMPU9250::Axis axis, float cutTreshold)
{
  if (m_energy < cutTreshold) {
    return 0; // level 0: not cutting
  }
  else {
    return computeAccelRMS(buffer_compute_index, axis) * 1000 / (2 * PI * m_maxIndex[axis]);
  }
}

/**
 * Compute RFFT on acceleration data on given axis and update m_maxIndex on this axis
 */
void IUMPU9250::computeAccelRFFT(uint8_t buffer_compute_index, IUMPU9250::Axis axis, q15_t *hamming_window)
{
  
  arm_mult_q15(m_batch[axis][buffer_compute_index],
               hamming_window,
               m_buffer[axis],
               NFFT);
  // RFFT
  arm_rfft_init_q15(&m_rfftInstance,
                    &m_cfftInstance,
                    NFFT, 0, 1);
  arm_rfft_q15(&m_rfftInstance,
               m_buffer[axis],
               m_rfftBuffer);
  
  float flatness = 0;
  for (int i = 2; i < 512; i++) {
    if (m_rfftBuffer[i] > flatness) {
      flatness = m_rfftBuffer[i];
      m_maxIndex[axis] = i;
    }
  }
}



