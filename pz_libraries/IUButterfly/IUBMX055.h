/*
  Infinite Uptime Firmware

  Update:
    03/03/2017
  
  Component:
    Name:
      BMX-055
    Description:
      Accelerometer, Gyroscope and Magnetometer for Butterfly board

   Comments:
    Only accelerometer features are currently used. Variables and functions that 
    concerns the magnetometer and gyroscope have not been developed, or have been
    commented in order to minimize sketch weight.
*/

#ifndef IUBMX055_H
#define IUBMX055_H

#include <Arduino.h>
/* CMSIS-DSP library for RFFT */
#include <arm_math.h>

#include "IUUtilities.h"
#include "IUFeature.h"
#include "IUI2C.h"
#include "IUBLE.h"

/**
 * BMX055 data sheet http://ae-bst.resource.bosch.com/media/products/dokumente/bmx055/BST-BMX055-DS000-01v2.pdf
 * The BMX055 is a conglomeration of three separate motion sensors packaged together but
 * addressed and communicated with separately by design
 */
class IUBMX055 : public IUABCProducer
{
  public:
    enum Axis : uint8_t {X = 0,
                         Y = 1,
                         Z = 2};
    /*===== DEVICE CONFIGURATION AND CONSTANTS =======*/
    // Accelerometer
    static const uint8_t ACC_ADDRESS          = 0x18;   // Address of accelerometer
    static const uint8_t ACC_WHO_AM_I         = 0x00;  // WHO_AM_I should be 0xFA
    static const uint8_t ACC_I_AM             = 0xFA;
    static const uint8_t ACC_OFC_CTRL         = 0x36;
    static const uint8_t ACC_OFC_SETTING      = 0x37;
    static const uint8_t ACC_OFC_OFFSET_X     = 0x38;
    static const uint8_t ACC_OFC_OFFSET_Y     = 0x39;
    static const uint8_t ACC_OFC_OFFSET_Z     = 0x3A;
    static const uint8_t ACC_TRIM_GPO         = 0x3B;
    static const uint8_t ACC_TRIM_GP1         = 0x3C;
    static const uint8_t ACC_D_X_LSB          = 0x02;
    static const uint8_t ACC_D_X_MSB          = 0x03;
    static const uint8_t ACC_D_Y_LSB          = 0x04;
    static const uint8_t ACC_D_Y_MSB          = 0x05;
    static const uint8_t ACC_D_Z_LSB          = 0x06;
    static const uint8_t ACC_D_Z_MSB          = 0x07;
    static const uint8_t ACC_PMU_RANGE        = 0x0F;
    static const uint8_t ACC_PMU_BW           = 0x10;
    static const uint8_t ACC_D_HBW            = 0x13;
    static const uint8_t ACC_INT_EN_0         = 0x16;
    static const uint8_t ACC_INT_EN_1         = 0x17;
    static const uint8_t ACC_INT_EN_2         = 0x18;
    static const uint8_t ACC_INT_MAP_0        = 0x19;
    static const uint8_t ACC_INT_MAP_1        = 0x1A;
    static const uint8_t ACC_INT_MAP_2        = 0x1B;
    static const uint8_t ACC_INT_OUT_CTRL     = 0x20;
    static const uint8_t ACC_BGW_SOFTRESET    = 0x14;
    
    enum accelScaleOption : uint8_t {AFS_2G  = 0x03,
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
    static const uint8_t GYRO_ADDRESS         = 0x68;  // Address of gyroscope
    static const uint8_t GYRO_WHO_AM_I        = 0x00;  // WHO_AM_I should be 0x0F
    static const uint8_t GYRO_I_AM            = 0x0F;
    static const uint8_t GYRO_BW              = 0x10;
    enum gyroScaleOption : uint8_t {GFS_2000DPS,
                                    GFS_1000DPS,
                                    GFS_500DPS,
                                    GFS_250DPS,
                                    GFS_125DPS};
    enum GODRBW : uint8_t {G_2000Hz523Hz,              // 2000 Hz ODR and unfiltered (bandwidth 523Hz)
                           G_2000Hz230Hz,
                           G_1000Hz116Hz,
                           G_400Hz47Hz,
                           G_200Hz23Hz,
                           G_100Hz12Hz,
                           G_200Hz64Hz,
                           G_100Hz32Hz};               // 100 Hz ODR and 32 Hz bandwidth

    //Magnetometer
    static const uint8_t MAG_ADDRESS          = 0x10;  // Address of magnetometer
    static const uint8_t MAG_WHO_AM_I         = 0x40;  // WHO_AM_I should be 0x32
    static const uint8_t MAG_I_AM             = 0x32;
    static const uint8_t MAG_PWR_CNTL1        = 0x4B;
    static const uint8_t MAG_PWR_CNTL2        = 0x4C;
    static const uint8_t MAG_REP_XY           = 0x51;
    static const uint8_t MAG_REP_Z            = 0x52;
    enum MODR : uint8_t {MODR_10Hz,                    // 10 Hz ODR  
                         MODR_2Hz,                     // 2 Hz ODR
                         MODR_6Hz,                     // 6 Hz ODR
                         MODR_8Hz,                     // 8 Hz ODR
                         MODR_15Hz,                    // 15 Hz ODR
                         MODR_20Hz,                    // 20 Hz ODR
                         MODR_25Hz,                    // 25 Hz ODR
                         MODR_30Hz};                   // 30 Hz ODR
    enum magMode : uint8_t {lowPower,                  // rms noise ~1.0 microTesla, 0.17 mA power
                            Regular,                   // rms noise ~0.6 microTesla, 0.5 mA power
                            enhancedRegular,           // rms noise ~0.5 microTesla, 0.8 mA power
                            highAccuracy};             // rms noise ~0.3 microTesla, 4.9 mA power

    enum dataSendOption : uint8_t {xAccel,             // Options of data to send to receivers
                                   yAccel,             // Can data from one of the 3 sensors on 1 axis
                                   zAccel,
                                   xGyro,
                                   yGyro,
                                   zGyro,
                                   xMag,
                                   yMag,
                                   zMag}; 
    
    // Constructor, destructor, getters and setters
    IUBMX055(IUI2C iuI2C, IUBLE iuBLE);
    virtual ~IUBMX055() {}
    // Methods
    void wakeUp();
    void initSensor();
    void configureAccelerometer();
    void configureGyroscope() { return; }              // Not implemented
    void configureMagnometer() { return; }             // Not implemented
    void setAccelScale(uint8_t val);
    void computeAccelResolution();
    q15_t lsbToMs2(q15_t acceldata);
    void readAccelData();
    void dumpDataThroughI2C();
    void doAcellFastCompensation(float * dest1);
    
    /*
    // BMX055 orientation filter as implemented by Kris Winer: https://github.com/kriswiner/BMX-055/blob/master/quaternionFilters.ino
    void MadgwickQuaternionUpdate(float ax, float ay, float az, float gx, float gy, float gz, float mx, float my, float mz);
    void MahonyQuaternionUpdate(float ax, float ay, float az, float gx, float gy, float gz, float mx, float my, float mz);
    */
    

  private:
    IUI2C m_iuI2C;
    IUBLE m_iuBLE;

    // Accelerometer:
    q15_t m_rawAccel[3];          // Stores the q15_t accelerometer sensor raw output
    q15_t m_accelData[3];          // Stores the q15_t acceleration data (with resolution and bias) in G
    q15_t m_accelBias[3];         // Bias corrections
    float m_accelResolution;      // Resolution
    uint8_t m_accelScale;         // Scale
    uint8_t m_accelBW;            // Bandwidth
    
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
