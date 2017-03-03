#ifndef IUBMX055_H
#define IUBMX055_H

#include <Arduino.h>
/* CMSIS-DSP library for RFFT */
#include <arm_math.h>

#include "IUUtilities.h"
#include "IUI2C.h"
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
    // Accelerometer
    static const uint8_t ACC_ADDRESS          = 0x18   // Address of accelerometer
    static const uint8_t ACC_WHO_AM_I         = 0x00;  // WHO_AM_I should be 0xFA
    static const uint8_t ACC_I_AM             = 0xFA;
    static const uint8_t ACC_OFC_CTRL         = 0x36
    static const uint8_t ACC_OFC_SETTING      = 0x37
    static const uint8_t ACC_OFC_OFFSET_X     = 0x38
    static const uint8_t ACC_OFC_OFFSET_Y     = 0x39
    static const uint8_t ACC_OFC_OFFSET_Z     = 0x3A
    static const uint8_t ACC_TRIM_GPO         = 0x3B
    static const uint8_t ACC_TRIM_GP1         = 0x3C
    static const uint8_t ACC_D_X_LSB          = 0x02
    static const uint8_t ACC_D_X_MSB          = 0x03
    static const uint8_t ACC_D_Y_LSB          = 0x04
    static const uint8_t ACC_D_Y_MSB          = 0x05
    static const uint8_t ACC_D_Z_LSB          = 0x06
    static const uint8_t ACC_D_Z_MSB          = 0x07
    enum AccelScaleOption : uint8_t {AFS_2G  = 0x03,
                                     AFS_4G  = 0x05,
                                     AFS_8G  = 0x08,
                                     AFS_16G = 0x0C};
    enum ACCBW {    // define BMX055 accelerometer bandwidths
                ABW_8Hz,      // 7.81 Hz,  64 ms update time
                ABW_16Hz,     // 15.63 Hz, 32 ms update time
                ABW_31Hz,     // 31.25 Hz, 16 ms update time
                ABW_63Hz,     // 62.5  Hz,  8 ms update time
                ABW_125Hz,    // 125   Hz,  4 ms update time
                ABW_250Hz,    // 250   Hz,  2 ms update time
                ABW_500Hz,    // 500   Hz,  1 ms update time
                ABW_100Hz     // 1000  Hz,  0.5 ms update time
                };

    //Gyroscope
    static const uint8_t GYRO_ADDRESS         = 0x68   // Address of gyroscope
    static const uint8_t GYRO_WHO_AM_I        = 0x00;  // WHO_AM_I should be 0x0F
    static const uint8_t GYRO_I_AM            = 0x0F;

    //Magnetometer
    static const uint8_t MAG_ADDRESS          = 0x10   // Address of magnetometer
    static const uint8_t MAG_WHO_AM_I         = 0x40;  // WHO_AM_I should be 0x32
    static const uint8_t MAG_I_AM             = 0x32;
    static const uint8_t MAG_PWR_CNTL1        = 0x4B;
    static const uint8_t MAG_PWR_CNTL2        = 0x4C;
    
    
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
    IUBMX055(IUI2C iuI2C, IUBLE iuBLE);
   
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
    /*
    // BMX055 orientation filter as implemented by Kris Winer: https://github.com/kriswiner/BMX-055/blob/master/quaternionFilters.ino
    void MadgwickQuaternionUpdate(float ax, float ay, float az, float gx, float gy, float gz, float mx, float my, float mz);
    void MahonyQuaternionUpdate(float ax, float ay, float az, float gx, float gy, float gz, float mx, float my, float mz);
    */
    

  private:
    IUI2C m_iuI2C;
    IUBLE m_iuBLE;
    float m_energy;

    // Accelerometer:
    int16_t m_rawAccel[3];        // Stores the 16-bit signed accelerometer sensor output
    float m_a[3];                 // Latest data values
    float m_accelBias[3];         // Bias corrections
    float m_accelResolution;      // Resolution
    uint8_t m_accelScale;         // Scale
    uint8_t m_accelBW;            // Bandwidth
    // freq and sampling
    uint16_t m_targetSample; // ex TARGET_ACCEL_SAMPLE // Target Accel frequency may change in data collection mode

    //FFT and post-processing
    int m_maxAccelIndex[3];                    // FFT frequency detection indexes for X, Y, Z axis
    q15_t m_accelBatch[3][2][MAX_INTERVAL];    // Accel buffer along X, Y, Z axis - 2 buffers namely for recording and computation simultaneously
    q15_t m_accelBuffer[3][MAX_INTERVAL];      // Accel buffer along X, Y, Z axis for fft calculation
    q15_t m_rfftBuffer[NFFT * 2];         // RFFT Output and Magnitude Buffer
    //TODO: May be able to use just one instance. Check later.
    arm_rfft_instance_q15 m_rfftInstance;
    arm_cfft_radix4_instance_q15 m_cfftInstance;

    /*
    // Gyroscope:
    int16_t m_rawGyro[3];         // Stores the 16-bit signed gyro sensor output
    float m_g[3];                 // Latest data values
    float m_gyroBias[3];          // Bias corrections
    float m_gyroResolution;       // Resolution
    uint8_t m_gyroScale;          // Scale
    
    // Magnetometer:
    int16_t m_rawMag[3];          // Stores the 13/15-bit signed magnetometer sensor output
    float m_m[3];                 // Latest data values
    float m_magBias[3];           // Bias corrections
    float q[4] = {1.0f, 0.0f, 0.0f, 0.0f};              // vector to hold quaternion
    float eInt[3] = {0.0f, 0.0f, 0.0f};                 // vector to hold integral error for Mahony method
    */

};

#endif // IUBMX055_H
