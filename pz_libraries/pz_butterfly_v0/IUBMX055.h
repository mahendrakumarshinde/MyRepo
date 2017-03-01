#ifndef IUBMX055_H
#define IUBMX055_H

#include <Arduino.h>
/* CMSIS-DSP library for RFFT */
#include <arm_math.h>

#include "IUUtilities.h"
#include "IUBLE.h"

/** BMX055 componant
 * 
 * NB: In order to minimize sketch weight, variables and functions that concerns the 
 * magnetometer and gyroscope have been commented. To use this functionnalities, uncomment
 * the corresponding lines.
 */
class IUBMX055
{
  public:
    enum Axis : uint8_t {X = 0,
                         Y = 1,
                         Z = 2};
    /*===== DEVICE CONFIGURATION AND CONSTANTS =======*/
    // Address
    static const uint8_t ACC_ADDRESS          = 0x18   // Address of accelerometer
    static const uint8_t GYRO_ADDRESS         = 0x68   // Address of gyroscope
    static const uint8_t MAG_ADDRESS          = 0x10   // Address of magnetometer
    static const uint8_t ACC_WHO_AM_I         = 0x00;  // WHO_AM_I should be 0xFA
    static const uint8_t ACC_I_AM             = 0xFA; 
    static const uint8_t GYRO_WHO_AM_I        = 0x00;  // WHO_AM_I should be 0x0F
    static const uint8_t GYRO_I_AM            = 0x0F;
    static const uint8_t MAG_WHO_AM_I         = 0x40;  // WHO_AM_I should be 0x32
    static const uint8_t MAG_I_AM             = 0x32;
    static const uint8_t MAG_PWR_CNTL1        = 0x4B
    static const uint8_t MAG_PWR_CNTL2        = 0x4C
    
    
    
    // Frequency and sampling setting
    static const uint16_t FREQ = 1000;             // Accel sampling rate
    // Vibration Energy
    static const uint32_t ENERGY_INTERVAL = 128;   // All data in buffer
    // Mean Crossing Rate
    static const uint32_t MCR_INTERVAL = 64;       // All data in buffer
    static const uint16_t MAX_INTERVAL = 512;      // Number of Acceleration readings per cycle
    // RFFT Variables
    static const uint16_t NFFT = 512;              // Accel FFT Index

    // Constructors, destructors, getters, setters
    IUBMX055(IUI2CTeensy iuI2C, IUBLE iuBLE);
    virtual ~IUBMX055();
   
    void setTargetSample(uint16_t target) { m_targetSample = target; }
    void resetTargetSample() { m_targetSample = FREQ; }
    uint16_t getTargetSample() { return m_targetSample; }
    float getEnergy() { return m_energy; }
    
    // Methods
    void wakeUp();
    void initSensor();
    void readAccelData();
    void pushDataToBatch(uint8_t buffer_record_index, uint32_t index);
    void dumpDataThroughI2C();
    void showRecordFFT(uint8_t buffer_compute_index, String mac_address);
    float computeEnergy (uint8_t buffer_compute_index, uint32_t batchSize, uint16_t counterTarget); // ex feature_energy
    float computeAccelRMS(uint8_t buffer_compute_index, IUBMX055::Axis axis);
    float computeVelocity(uint8_t buffer_compute_index, IUBMX055::Axis axis, float cutTreshold);
    void computeAccelRFFT(uint8_t buffer_compute_index, IUBMX055::Axis axis, q15_t *hamming_window);
    /* BMX055 orientation filter as implemented by Kris Winer: https://github.com/kriswiner/BMX-055/blob/master/quaternionFilters.ino */
    void MadgwickQuaternionUpdate(float ax, float ay, float az, float gx, float gy, float gz, float mx, float my, float mz);
    void MahonyQuaternionUpdate(float ax, float ay, float az, float gx, float gy, float gz, float mx, float my, float mz);

  private:
    IUI2CTeensy m_iuI2C;
    IUBLE m_iuBLE;
    float m_energy;
    uint8_t m_scale;
    float m_resolution;                                 // resolution of accelerometer

    // Accelerometer:
    int16_t m_accelCount[3];      // Stores the 16-bit signed accelerometer sensor output
    float m_a[3];                 // Latest data values
    float m_accelBias[3];         // Bias corrections

    /*
    // Gyroscope:
    int16_t m_gyroCount[3];       // Stores the 16-bit signed gyro sensor output
    float m_g[3];                 // Latest data values
    float m_gyroBias[3];          // Bias corrections
    
    // Magnetometer:
    int16_t m_magCount[3];        // Stores the 13/15-bit signed magnetometer sensor output
    float m_m[3];                 // Latest data values
    float m_magBias[3];           // Bias corrections
    float q[4] = {1.0f, 0.0f, 0.0f, 0.0f};              // vector to hold quaternion
    float eInt[3] = {0.0f, 0.0f, 0.0f};                 // vector to hold integral error for Mahony method
    */

    
    // freq and sampling
    uint16_t m_targetSample; // ex TARGET_ACCEL_SAMPLE // Target Accel frequency may change in data collection mode
    
    // FFT frequency detection indexes
    int m_maxIndex[3];  // Frequency Index for X, Y, Z axis
    // Acceleration Buffers
    q15_t m_batch[3][2][MAX_INTERVAL];  // Accel buffer along X, Y, Z axis - 2 buffers namely for recording and computation simultaneously
    // ex accel_x_batch, accel_y_batch, accel_z_batch 
    
    q15_t m_buffer[3][MAX_INTERVAL]; // Accel buff along X, Y, Z axis for fft calculation
    // ex accel_x_buff, accel_y_buff, accel_z_buff

    q15_t m_rfftBuffer[NFFT * 2]; // ex rfft_accel_buffer // RFFT Output and Magnitude Buffer
    //TODO: May be able to use just one instance. Check later.
    arm_rfft_instance_q15 m_rfftInstance;
    arm_cfft_radix4_instance_q15 m_cfftInstance;
};

#endif // IUBMX055_H
