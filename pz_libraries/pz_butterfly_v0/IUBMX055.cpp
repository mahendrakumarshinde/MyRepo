#include "IUBMX055.h"


IUMPU9250::IUMPU9250(IUI2CTeensy iuI2C, IUBLE iuBLE) : m_iuI2C(iuI2C), m_iuBLE(iuBLE), m_energy(0)
{
  for (int i=0; i < 3; i++)
  {
    m_rawAccel[i] = 0;
    m_maxIndex[i] = 0;
  }
}

IUMPU9250::~IUMPU9250()
{
  //dtor
}

/**
 * Ping the device address and, if the answer is correct, initialize it
 */
void IUMPU9250::wakeUp()
{
  
  delay(15);
}

/**
 * Initialize the device by configurating the accelerometer, gyroscope and sample rates
 */
void IUMPU9250::initSensor()
{
  
  delay(100);
  m_iuI2C.port->print("IUBMX055 initialized successfully.\n\n");
  m_iuI2C.port->flush();
}

/* ==================== Update and Control Functions =============================== */
/*
 * The following functions check and read the I2C wire buffer to see if configuration updates
 * have been received. If so, the updates are done.
 * Functions return true if an update happened.
 */



/* ==================== Data Collection and Feature Calculation functions ======================== */

/**
 * Read acceleration data and writes it to m_rawAccel
 * @param destination an array[3] to receive x, y, z accelaration
 */
void IUBMX055::readAccelData()
{
  
}

/**
 * Push acceleration in the batch array to later compute the FFT
 * NB: We want to do this in *RUN* mode
 */
void IUBMX055::pushDataToBatch(uint8_t buffer_record_index, uint32_t index)
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
  
}

/**
 * Convert least significant byte acceleration data to m/s2 using device resolution
 * @param accelLSB lest significant byte data to convert
 */
float IUBMX055::LSB_to_ms2(int16_t accelLSB)
{
  
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
float IUBMX055::computeAccelRMS(uint8_t buffer_compute_index, IUMPU9250::Axis axis)
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
float IUBMX055::computeVelocity(uint8_t buffer_compute_index, IUMPU9250::Axis axis, float cutTreshold)
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
void IUBMX055::computeAccelRFFT(uint8_t buffer_compute_index, IUMPU9250::Axis axis, q15_t *hamming_window)
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

/* BMX055 orientation filter as implemented by Kris Winer: https://github.com/kriswiner/BMX-055/blob/master/quaternionFilters.ino */
// Implementation of Sebastian Madgwick's "...efficient orientation filter for... inertial/magnetic sensor arrays"
// (see http://www.x-io.co.uk/category/open-source/ for examples and more details)
// which fuses acceleration, rotation rate, and magnetic moments to produce a quaternion-based estimate of absolute
// device orientation -- which can be converted to yaw, pitch, and roll. Useful for stabilizing quadcopters, etc.
// The performance of the orientation filter is at least as good as conventional Kalman-based filtering algorithms
// but is much less computationally intensive---it can be performed on a 3.3 V Pro Mini operating at 8 MHz!
void IUBMX055::MadgwickQuaternionUpdate(float ax, float ay, float az, float gx, float gy, float gz, float mx, float my, float mz)
{
  float q1 = q[0], q2 = q[1], q3 = q[2], q4 = q[3];   // short name local variable for readability
  float norm;
  float hx, hy, _2bx, _2bz;
  float s1, s2, s3, s4;
  float qDot1, qDot2, qDot3, qDot4;

  // Auxiliary variables to avoid repeated arithmetic
  float _2q1mx;
  float _2q1my;
  float _2q1mz;
  float _2q2mx;
  float _4bx;
  float _4bz;
  float _2q1 = 2.0f * q1;
  float _2q2 = 2.0f * q2;
  float _2q3 = 2.0f * q3;
  float _2q4 = 2.0f * q4;
  float _2q1q3 = 2.0f * q1 * q3;
  float _2q3q4 = 2.0f * q3 * q4;
  float q1q1 = q1 * q1;
  float q1q2 = q1 * q2;
  float q1q3 = q1 * q3;
  float q1q4 = q1 * q4;
  float q2q2 = q2 * q2;
  float q2q3 = q2 * q3;
  float q2q4 = q2 * q4;
  float q3q3 = q3 * q3;
  float q3q4 = q3 * q4;
  float q4q4 = q4 * q4;

  // Normalise accelerometer measurement
  norm = sqrt(ax * ax + ay * ay + az * az);
  if (norm == 0.0f) return; // handle NaN
  norm = 1.0f/norm;
  ax *= norm;
  ay *= norm;
  az *= norm;

  // Normalise magnetometer measurement
  norm = sqrt(mx * mx + my * my + mz * mz);
  if (norm == 0.0f) return; // handle NaN
  norm = 1.0f/norm;
  mx *= norm;
  my *= norm;
  mz *= norm;

  // Reference direction of Earth's magnetic field
  _2q1mx = 2.0f * q1 * mx;
  _2q1my = 2.0f * q1 * my;
  _2q1mz = 2.0f * q1 * mz;
  _2q2mx = 2.0f * q2 * mx;
  hx = mx * q1q1 - _2q1my * q4 + _2q1mz * q3 + mx * q2q2 + _2q2 * my * q3 + _2q2 * mz * q4 - mx * q3q3 - mx * q4q4;
  hy = _2q1mx * q4 + my * q1q1 - _2q1mz * q2 + _2q2mx * q3 - my * q2q2 + my * q3q3 + _2q3 * mz * q4 - my * q4q4;
  _2bx = sqrt(hx * hx + hy * hy);
  _2bz = -_2q1mx * q3 + _2q1my * q2 + mz * q1q1 + _2q2mx * q4 - mz * q2q2 + _2q3 * my * q4 - mz * q3q3 + mz * q4q4;
  _4bx = 2.0f * _2bx;
  _4bz = 2.0f * _2bz;

  // Gradient decent algorithm corrective step
  s1 = -_2q3 * (2.0f * q2q4 - _2q1q3 - ax) + _2q2 * (2.0f * q1q2 + _2q3q4 - ay) - _2bz * q3 * (_2bx * (0.5f - q3q3 - q4q4) + _2bz * (q2q4 - q1q3) - mx) + (-_2bx * q4 + _2bz * q2) * (_2bx * (q2q3 - q1q4) + _2bz * (q1q2 + q3q4) - my) + _2bx * q3 * (_2bx * (q1q3 + q2q4) + _2bz * (0.5f - q2q2 - q3q3) - mz);
  s2 = _2q4 * (2.0f * q2q4 - _2q1q3 - ax) + _2q1 * (2.0f * q1q2 + _2q3q4 - ay) - 4.0f * q2 * (1.0f - 2.0f * q2q2 - 2.0f * q3q3 - az) + _2bz * q4 * (_2bx * (0.5f - q3q3 - q4q4) + _2bz * (q2q4 - q1q3) - mx) + (_2bx * q3 + _2bz * q1) * (_2bx * (q2q3 - q1q4) + _2bz * (q1q2 + q3q4) - my) + (_2bx * q4 - _4bz * q2) * (_2bx * (q1q3 + q2q4) + _2bz * (0.5f - q2q2 - q3q3) - mz);
  s3 = -_2q1 * (2.0f * q2q4 - _2q1q3 - ax) + _2q4 * (2.0f * q1q2 + _2q3q4 - ay) - 4.0f * q3 * (1.0f - 2.0f * q2q2 - 2.0f * q3q3 - az) + (-_4bx * q3 - _2bz * q1) * (_2bx * (0.5f - q3q3 - q4q4) + _2bz * (q2q4 - q1q3) - mx) + (_2bx * q2 + _2bz * q4) * (_2bx * (q2q3 - q1q4) + _2bz * (q1q2 + q3q4) - my) + (_2bx * q1 - _4bz * q3) * (_2bx * (q1q3 + q2q4) + _2bz * (0.5f - q2q2 - q3q3) - mz);
  s4 = _2q2 * (2.0f * q2q4 - _2q1q3 - ax) + _2q3 * (2.0f * q1q2 + _2q3q4 - ay) + (-_4bx * q4 + _2bz * q2) * (_2bx * (0.5f - q3q3 - q4q4) + _2bz * (q2q4 - q1q3) - mx) + (-_2bx * q1 + _2bz * q3) * (_2bx * (q2q3 - q1q4) + _2bz * (q1q2 + q3q4) - my) + _2bx * q2 * (_2bx * (q1q3 + q2q4) + _2bz * (0.5f - q2q2 - q3q3) - mz);
  norm = sqrt(s1 * s1 + s2 * s2 + s3 * s3 + s4 * s4);    // normalise step magnitude
  norm = 1.0f/norm;
  s1 *= norm;
  s2 *= norm;
  s3 *= norm;
  s4 *= norm;

  // Compute rate of change of quaternion
  qDot1 = 0.5f * (-q2 * gx - q3 * gy - q4 * gz) - beta * s1;
  qDot2 = 0.5f * (q1 * gx + q3 * gz - q4 * gy) - beta * s2;
  qDot3 = 0.5f * (q1 * gy - q2 * gz + q4 * gx) - beta * s3;
  qDot4 = 0.5f * (q1 * gz + q2 * gy - q3 * gx) - beta * s4;

  // Integrate to yield quaternion
  q1 += qDot1 * deltat;
  q2 += qDot2 * deltat;
  q3 += qDot3 * deltat;
  q4 += qDot4 * deltat;
  norm = sqrt(q1 * q1 + q2 * q2 + q3 * q3 + q4 * q4);    // normalise quaternion
  norm = 1.0f/norm;
  q[0] = q1 * norm;
  q[1] = q2 * norm;
  q[2] = q3 * norm;
  q[3] = q4 * norm;
}
  
// Similar to Madgwick scheme but uses proportional and integral filtering on the error between estimated reference vectors and
// measured ones. 
void IUBMX055::MahonyQuaternionUpdate(float ax, float ay, float az, float gx, float gy, float gz, float mx, float my, float mz)
{
  float q1 = q[0], q2 = q[1], q3 = q[2], q4 = q[3];   // short name local variable for readability
  float norm;
  float hx, hy, bx, bz;
  float vx, vy, vz, wx, wy, wz;
  float ex, ey, ez;
  float pa, pb, pc;

  // Auxiliary variables to avoid repeated arithmetic
  float q1q1 = q1 * q1;
  float q1q2 = q1 * q2;
  float q1q3 = q1 * q3;
  float q1q4 = q1 * q4;
  float q2q2 = q2 * q2;
  float q2q3 = q2 * q3;
  float q2q4 = q2 * q4;
  float q3q3 = q3 * q3;
  float q3q4 = q3 * q4;
  float q4q4 = q4 * q4;   

  // Normalise accelerometer measurement
  norm = sqrt(ax * ax + ay * ay + az * az);
  if (norm == 0.0f) return; // handle NaN
  norm = 1.0f / norm;        // use reciprocal for division
  ax *= norm;
  ay *= norm;
  az *= norm;

  // Normalise magnetometer measurement
  norm = sqrt(mx * mx + my * my + mz * mz);
  if (norm == 0.0f) return; // handle NaN
  norm = 1.0f / norm;        // use reciprocal for division
  mx *= norm;
  my *= norm;
  mz *= norm;

  // Reference direction of Earth's magnetic field
  hx = 2.0f * mx * (0.5f - q3q3 - q4q4) + 2.0f * my * (q2q3 - q1q4) + 2.0f * mz * (q2q4 + q1q3);
  hy = 2.0f * mx * (q2q3 + q1q4) + 2.0f * my * (0.5f - q2q2 - q4q4) + 2.0f * mz * (q3q4 - q1q2);
  bx = sqrt((hx * hx) + (hy * hy));
  bz = 2.0f * mx * (q2q4 - q1q3) + 2.0f * my * (q3q4 + q1q2) + 2.0f * mz * (0.5f - q2q2 - q3q3);

  // Estimated direction of gravity and magnetic field
  vx = 2.0f * (q2q4 - q1q3);
  vy = 2.0f * (q1q2 + q3q4);
  vz = q1q1 - q2q2 - q3q3 + q4q4;
  wx = 2.0f * bx * (0.5f - q3q3 - q4q4) + 2.0f * bz * (q2q4 - q1q3);
  wy = 2.0f * bx * (q2q3 - q1q4) + 2.0f * bz * (q1q2 + q3q4);
  wz = 2.0f * bx * (q1q3 + q2q4) + 2.0f * bz * (0.5f - q2q2 - q3q3);  

  // Error is cross product between estimated direction and measured direction of gravity
  ex = (ay * vz - az * vy) + (my * wz - mz * wy);
  ey = (az * vx - ax * vz) + (mz * wx - mx * wz);
  ez = (ax * vy - ay * vx) + (mx * wy - my * wx);
  if (Ki > 0.0f)
  {
      eInt[0] += ex;      // accumulate integral error
      eInt[1] += ey;
      eInt[2] += ez;
  }
  else
  {
      eInt[0] = 0.0f;     // prevent integral wind up
      eInt[1] = 0.0f;
      eInt[2] = 0.0f;
  }

  // Apply feedback terms
  gx = gx + Kp * ex + Ki * eInt[0];
  gy = gy + Kp * ey + Ki * eInt[1];
  gz = gz + Kp * ez + Ki * eInt[2];

  // Integrate rate of change of quaternion
  pa = q2;
  pb = q3;
  pc = q4;
  q1 = q1 + (-q2 * gx - q3 * gy - q4 * gz) * (0.5f * deltat);
  q2 = pa + (q1 * gx + pb * gz - pc * gy) * (0.5f * deltat);
  q3 = pb + (q1 * gy - pa * gz + pc * gx) * (0.5f * deltat);
  q4 = pc + (q1 * gz + pa * gy - pb * gx) * (0.5f * deltat);

  // Normalise quaternion
  norm = sqrt(q1 * q1 + q2 * q2 + q3 * q3 + q4 * q4);
  norm = 1.0f / norm;
  q[0] = q1 * norm;
  q[1] = q2 * norm;
  q[2] = q3 * norm;
  q[3] = q4 * norm;
}


