#ifndef IUMPU9250_H
#define IUMPU9250_H

#include <Arduino.h>
/* CMSIS-DSP library for RFFT */
#include <arm_math.h>

#include "IUUtilities.h"
#include "IUI2CTeensy.h"
#include "IUBLE.h"

#if ADO
#define MPU9250_ADDRESS 0x69  // Device address when ADO = 1
#else
#define MPU9250_ADDRESS 0x68  // Device address when ADO = 0
#endif

class IUMPU9250
{
  public:
    enum Axis : uint8_t {X = 0,
                         Y = 1,
                         Z = 2};
    /*===== DEVICE CONFIGURATION AND CONSTANTS =======*/
    // Address
    static const uint8_t ADDRESS              = MPU9250_ADDRESS;
    static const uint8_t WHO_AM_I             = 0x75;
    static const uint8_t WHO_AM_I_ANSWER      = 0x71; //WHO_AM_I should always be 0x71 for the entire chip
    static const uint8_t SMPLRT_DIV           = 0x19;
    static const uint8_t GYRO_CONFIG          = 0x1B;
    static const uint8_t ACCEL_CONFIG         = 0x1C;
    static const uint8_t ACCEL_CONFIG2        = 0x1D;
    static const uint8_t LP_ACCEL_ODR         = 0x1E;
    static const uint8_t INT_PIN_CFG          = 0x37;
    static const uint8_t INT_ENABLE           = 0x38;
    static const uint8_t PWR_MGMT_1           = 0x6B; // Device defaults to the SLEEP mode
    static const uint8_t PWR_MGMT_2           = 0x6C;
    // EM7180 SENtral register map
    // see http://www.emdeveloper.com/downloads/7180/EMSentral_EM7180_Register_Map_v1_3.pdf
    static const uint8_t EM7180_ADDRESS          = 0x28;
    static const uint8_t EM7180_AlgorithmControl = 0x54;
    static const uint8_t EM7180_PassThruControl  = 0xA0;
    static const uint8_t EM7180_PassThruStatus   = 0x9E;
    //TODO check why we only use ! OUT_H value (corresponding to XOUT_H)
    //static constexpr uint8_t OUT_H[3] = {0x3B, 0x3D, 0x3F};
    //static constexpr uint8_t OUT_L[3] = {0x3C, 0x3E, 0x40};
    static const uint8_t OUT_H = 0x3B;
    // Define MPU ACC full scale options
    // TODO: CHANGE: new constants for MPU
    enum ScaleOption : uint8_t {AFS_2G = 0,
                                AFS_4G = 1,
                                AFS_8G = 2,
                                AFS_16G = 3};
    // MPU-9250 accel register constants
    static constexpr uint8_t SELF_TEST[3] = {0x0D, 0x0E, 0x0F};
    // Frequency and sampling setting
    static const uint16_t FREQ = 1000;     // ex ACCEL_FREQ  // Accel frequency set to 1000 Hz
    // Vibration Energy
    static const uint32_t ENERGY_INTERVAL = 128; // ex ENERGY_INTERVAL_ACCEL  //All data in buffer
    // Mean Crossing Rate
    static const uint32_t MCR_INTERVAL = 64; // ex MCR_INTERVAL_ACCEL  //All data in buffer
    static const uint16_t MAX_INTERVAL = 512;  // ex MAX_INTERVAL_ACCEL  //Number of Acceleration readings per cycle
    // RFFT Variables
    static const uint16_t NFFT = 512;  // ex ACCEL_NFFT  // Accel FFT Index
    //const uint8_t ACCEL_RESCALE = 8; // Scaling factor in fft function

    // Constructors, destructors, getters, setters
    IUMPU9250(IUI2CTeensy iuI2C, IUBLE iuBLE);
    IUMPU9250(IUI2CTeensy iuI2C, IUBLE iuBLE, ScaleOption scale);
    uint8_t getScale() { return m_scale; }
    void setScale(uint8_t val);
    uint8_t getBandwidth() { return m_bandwidth; }
    void setBandwidth(uint8_t val) { m_bandwidth = val; }
    float getResolution() { return m_resolution; }
    void setResolution(float val) { m_resolution = val; }
    void setTargetSample(uint16_t target) { m_targetSample = target; }
    void resetTargetSample() { m_targetSample = FREQ; }
    uint16_t getTargetSample() { return m_targetSample; }
    float getEnergy() { return m_energy; }
    
    // Methods
    void computeResolution();
    void setSENtralToPassThroughMode();
    void wakeUp();
    void initSensor();
    bool updateAccelRangeFromI2C();
    bool updateSamplingRateFromI2C();
    void readAccelData();
    void pushDataToBatch(uint8_t buffer_record_index, uint32_t index);
    void dumpDataThroughI2C();
    void showRecordFFT(uint8_t buffer_compute_index, String mac_address);
    float LSB_to_ms2(int16_t accelLSB);
    float getScalingFactor(IUMPU9250::Axis axis);
    float computeEnergy (uint8_t buffer_compute_index, uint32_t batchSize, uint16_t counterTarget); // ex feature_energy
    float computeAccelRMS(uint8_t buffer_compute_index, IUMPU9250::Axis axis);
    float computeVelocity(uint8_t buffer_compute_index, IUMPU9250::Axis axis, float cutTreshold);
    void computeAccelRFFT(uint8_t buffer_compute_index, IUMPU9250::Axis axis, q15_t *hamming_window);

  private:
    IUI2CTeensy m_iuI2C;
    IUBLE m_iuBLE;
    uint8_t m_scale;
    uint8_t m_bandwidth;
    float m_energy;
    /// Per axis bias corrections for accelerometer
    int16_t m_bias[3];
    /// Stores the 16-bit signed accelerometer sensor output
    int16_t m_rawAccel[3]; // ex accelCount
    /// resolution of accelerometer
    float m_resolution;
    /// variables to hold latest sensor data values
    float m_a[3];
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

// Specify sensor full scale
//uint8_t ACCBW  = 0x08 & ABW_1000Hz;  // Choose bandwidth for accelerometer

#endif // IUMPU9250_H
