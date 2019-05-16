/*
  Infinite Uptime Firmware

  Update:
    03/03/2017

  Component:
    Name:
      BMX-055
    Description:
      Accelerometer, Gyroscope and Magnetometer for Butterfly board. Each of the sensors
      has its own registers, power management and data acquisition configuration, so each
      sensor has its own class.
      Data sheet => http://ae-bst.resource.bosch.com/media/products/dokumente/bmx055/BST-BMX055-DS000-01v2.pdf
*/

#ifndef IUBMX055GYRO_H
#define IUBMX055GYRO_H

#include <Arduino.h>

#include "IUABCSensor.h"
#include "IUFeature.h"
#include "IUI2C.h"


class IUBMX055Gyro : public IUABCSensor
{
  public:
    enum Axis : uint8_t {X = 0,
                         Y = 1,
                         Z = 2};
    static const sensorTypeOptions sensorType = IUABCSensor::ORIENTATION;

    /*===== DEVICE CONFIGURATION AND CONSTANTS =======*/
    static const uint8_t ADDRESS         = 0x68;
    static const uint8_t WHO_AM_I        = 0x00;
    static const uint8_t I_AM            = 0x0F;
    static const uint8_t RANGE           = 0x0F;
    static const uint8_t BW              = 0x10;
    static const uint8_t BGW_SOFTRESET   = 0x14;
    static const uint8_t LPM1            = 0x11;
    static const uint8_t LPM2            = 0x12;

    enum scaleOption : uint8_t {GFS_2000DPS,
                                GFS_1000DPS,
                                GFS_500DPS,
                                GFS_250DPS,
                                GFS_125DPS};
    enum bandwidthOption : uint8_t {BW_2000Hz523Hz, // 2000 Hz ODR and unfiltered (bandwidth 523Hz)
                                    BW_2000Hz230Hz,
                                    BW_1000Hz116Hz,
                                    BW_400Hz47Hz,
                                    BW_200Hz23Hz,
                                    BW_100Hz12Hz,
                                    BW_200Hz64Hz,
                                    BW_100Hz32Hz};  // 100 Hz ODR and 32 Hz bandwidth
    static const scaleOption defaultScale = GFS_125DPS;
    static const bandwidthOption defaultBandwidth = BW_200Hz23Hz;

    enum dataSendOption : uint8_t {dataX        = 0,
                                   dataY        = 1,
                                   dataZ        = 2,
                                   resolution   = 3,
                                   samplingRate = 4,
                                   optionCount  = 5};
    static const uint16_t defaultSamplingRate = 1000; // Hz

    // Constructor, destructor, getters and setters
    IUBMX055Gyro(IUI2C *iuI2C);
    virtual ~IUBMX055Gyro() {}
    virtual char getSensorType() { return (char) sensorType; }
    // Hardware & power management methods
    void softReset();
    virtual void wakeUp();
    virtual void sleep();
    virtual void suspend();
    void enableFastPowerUp();
    void disableFastPowerUp();
    void powerSaveMode() {}                // Not implemented
    void setScale(scaleOption scale);
    void setBandwidth(bandwidthOption scale);
    // Data acquisition methods
    void computeResolution();
    virtual void readData() {}             // Not implemented
    // Communication methods
    virtual void sendToReceivers() {}      // Not implemented
    virtual void dumpDataThroughI2C() {}   // Not implemented
    virtual void dumpDataForDebugging() {} // Not implemented
    // Diagnostic Functions
    virtual void exposeCalibration() {}    // Not implemented

  protected:
    static IUI2C *m_iuI2C;
    // Configuration
    q15_t m_resolution;           // Resolution
    scaleOption m_scale;
    bandwidthOption m_bandwidth;
    bool m_fastPowerUpEnabled;
    // Data acquisition variables
    static int16_t m_rawData[3];  // Stores the 16-bit signed gyro sensor output
    static q15_t m_data[3];       // Latest data values
    static q15_t m_bias[3];       // Bias corrections
};


#endif // IUBMX055GYRO_H
