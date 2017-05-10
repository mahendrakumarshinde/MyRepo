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

#include "IUABCSensor.h"
#include "IUFeature.h"
#include "IUI2C.h"

/**
 * BMX055 data sheet http://ae-bst.resource.bosch.com/media/products/dokumente/bmx055/BST-BMX055-DS000-01v2.pdf
 * The BMX055 is a conglomeration of three separate motion sensors packaged together but
 * addressed and communicated with separately by design
 */
class IUBMX055 : public IUABCSensor
{
  public:
    enum Axis : uint8_t {X = 0,
                         Y = 1,
                         Z = 2};
    static const uint8_t sensorTypeCount = 3;
    static char sensorTypes[sensorTypeCount];

    /*===== DEVICE CONFIGURATION AND CONSTANTS =======*/
    // Accelerometer
    static const uint8_t ACC_ADDRESS          = 0x18;   // Address of accelerometer
    static const uint8_t ACC_WHO_AM_I         = 0x00;
    static const uint8_t ACC_I_AM             = 0xFA;
    static const uint8_t ACC_OFC_CTRL         = 0x36;
    static const uint8_t ACC_OFC_SETTING      = 0x37;
    static const uint8_t ACC_OFC_OFFSET_X     = 0x38;
    static const uint8_t ACC_OFC_OFFSET_Y     = 0x39;
    static const uint8_t ACC_OFC_OFFSET_Z     = 0x3A;
    static const uint8_t ACC_D_X_LSB          = 0x02;
    static const uint8_t ACC_D_Y_LSB          = 0x04;
    static const uint8_t ACC_D_Z_LSB          = 0x06;
    static const uint8_t ACC_PMU_RANGE        = 0x0F;
    static const uint8_t ACC_PMU_BW           = 0x10;
    static const uint8_t ACC_D_HBW            = 0x13;
    static const uint8_t ACC_INT_EN_1         = 0x17;
    static const uint8_t ACC_INT_MAP_1        = 0x1A;
    static const uint8_t ACC_INT_OUT_CTRL     = 0x20;
    static const uint8_t ACC_BGW_SOFTRESET    = 0x14;
    static const uint8_t ACC_INT_PIN          = 7;

    enum accelScaleOption : uint8_t {AFS_2G  = 0x03,
                                     AFS_4G  = 0x05,
                                     AFS_8G  = 0x08,
                                     AFS_16G = 0x0C};
    // Accelerometer bandwidths for filtered acceleration readings
    enum accelBandwidthOption : uint8_t {ABW_8Hz    = 0x08,    // 7.81 Hz,  64 ms update time
                                         ABW_16Hz   = 0x09,    // 15.63 Hz, 32 ms update time
                                         ABW_31Hz   = 0x0A,    // 31.25 Hz, 16 ms update time
                                         ABW_63Hz   = 0x0B,    // 62.5  Hz,  8 ms update time
                                         ABW_125Hz  = 0x0C,    // 125   Hz,  4 ms update time
                                         ABW_250Hz  = 0x0D,    // 250   Hz,  2 ms update time
                                         ABW_500Hz  = 0x0E,    // 500   Hz,  1 ms update time
                                         ABW_1000Hz = 0x0F};   // 1000  Hz,  0.5 ms update time
    static const accelScaleOption defaultAccelScale = AFS_4G;
    static const accelBandwidthOption defaultAccelBandwidth = ABW_16Hz;

    //Gyroscope
    static const uint8_t GYRO_ADDRESS         = 0x68;  // Address of gyroscope
    static const uint8_t GYRO_WHO_AM_I        = 0x00;  // WHO_AM_I should be 0x0F
    static const uint8_t GYRO_I_AM            = 0x0F;
    static const uint8_t GYRO_RANGE           = 0x0F;
    static const uint8_t GYRO_BW              = 0x10;
    enum gyroScaleOption : uint8_t {GFS_2000DPS,
                                    GFS_1000DPS,
                                    GFS_500DPS,
                                    GFS_250DPS,
                                    GFS_125DPS};
    enum gyroBandwidthOption : uint8_t {G_2000Hz523Hz,              // 2000 Hz ODR and unfiltered (bandwidth 523Hz)
                                        G_2000Hz230Hz,
                                        G_1000Hz116Hz,
                                        G_400Hz47Hz,
                                        G_200Hz23Hz,
                                        G_100Hz12Hz,
                                        G_200Hz64Hz,
                                        G_100Hz32Hz};               // 100 Hz ODR and 32 Hz bandwidth
    static const gyroScaleOption defaultGyroScale = GFS_125DPS;
    static const gyroBandwidthOption defaultGyroBandwidth = G_200Hz23Hz;

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
                            regular,                   // rms noise ~0.6 microTesla, 0.5 mA power
                            enhancedRegular,           // rms noise ~0.5 microTesla, 0.8 mA power
                            highAccuracy};             // rms noise ~0.3 microTesla, 4.9 mA power

    enum dataSendOption : uint8_t {xAccel          = 0, // Options of data to send to receivers
                                   yAccel          = 1, // Can be data from one of the 3 sensors on 1 axis
                                   zAccel          = 2,
                                   accelResolution = 3,
                                   xGyro           = 4,
                                   yGyro           = 5,
                                   zGyro           = 6,
                                   gyroResolution  = 7,
                                   xMag            = 8,
                                   yMag            = 9,
                                   zMag            = 10,
                                   samplingRate    = 11,
                                   optionCount     = 12};

    static const uint16_t defaultSamplingRate = 1000; // Hz

    // Constructor, destructor, getters and setters
    IUBMX055(IUI2C *iuI2C);
    virtual ~IUBMX055() {}
    virtual uint8_t getSensorTypeCount() { return sensorTypeCount; }
    virtual char getSensorType(uint8_t index) { return sensorTypes[index]; }
    bool isNewData() {return m_newData; }
    void setAccelScale(accelScaleOption scale);
    void resetAccelScale() { setAccelScale(defaultAccelScale); }
    void setAccelBandwidth(accelBandwidthOption bandwidth);
    q15_t getAccelData(uint8_t index) { return m_accelData[index]; }
    void useFilteredData(accelBandwidthOption bandwidth);
    void useUnfilteredData();
    void accelSoftReset();
    // Methods
    virtual void wakeUp();
    void initSensor();
    void configureAccelerometer();
    void configureGyroscope();
    void configureMagnometer(magMode mMode);
    void computeAccelResolution();
    bool checkAccelerometerWhoAmI();
    bool checkGyroscopeWhoAmI();
    bool checkMagnetometerWhoAmI();
    void doAcellFastCompensation(float * dest1);
    void readAccelData();
    virtual void readData();
    virtual void sendToReceivers();
    void dumpDataThroughI2C();
    virtual void dumpDataForDebugging();
    // Diagnostic Functions
    virtual void exposeCalibration();

    /*
    // BMX055 orientation filter as implemented by Kris Winer: https://github.com/kriswiner/BMX-055/blob/master/quaternionFilters.ino
    void MadgwickQuaternionUpdate(float ax, float ay, float az, float gx, float gy, float gz, float mx, float my, float mz);
    void MahonyQuaternionUpdate(float ax, float ay, float az, float gx, float gy, float gz, float mx, float my, float mz);
    */


  protected:
    static IUI2C *m_iuI2C;
    bool m_newData;
    // Accelerometer:
    static uint8_t m_rawAccelBytes[6];                // 12bits / 2 bytes per axis: LSB, MSB => last 4 bits of LSB are not data, they are flags
    static q15_t m_rawAccel[3];                       // Stores the Q15 accelerometer sensor raw output
    static q15_t m_accelData[3];                      // Stores the Q15 acceleration data (with bias) in G
    static q15_t m_accelBias[3];                      // Bias corrections
    static q15_t m_accelResolution;                   // Resolution
    static void processAccelData(uint8_t wireStatus);
    accelScaleOption m_accelScale;                    // Scale
    accelBandwidthOption m_accelBandwidth;            // Bandwidth
    bool m_filteredData;

    /*
    // Gyroscope:
    int16_t m_rawGyro[3];         // Stores the 16-bit signed gyro sensor output
    float m_g[3];                 // Latest data values
    float m_gyroBias[3];          // Bias corrections
    q15_t m_gyroResolution;       // Resolution
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
