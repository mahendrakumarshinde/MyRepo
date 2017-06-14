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

#ifndef IUBMX055ACC_H
#define IUBMX055ACC_H

#include <Arduino.h>

#include "IUABCSensor.h"
#include "IUFeature.h"
#include "IUI2C.h"


class IUBMX055Acc : public IUABCSensor
{
  public:
    enum Axis : uint8_t {X = 0,
                         Y = 1,
                         Z = 2};
    static const sensorTypeOptions sensorType = IUABCSensor::ACCELERATION;

    /*===== DEVICE CONFIGURATION AND CONSTANTS =======*/
    static const uint8_t ADDRESS          = 0x18;
    static const uint8_t WHO_AM_I         = 0x00;
    static const uint8_t I_AM             = 0xFA;
    static const uint8_t OFC_CTRL         = 0x36;
    static const uint8_t OFC_SETTING      = 0x37;
    static const uint8_t OFC_OFFSET_X     = 0x38;
    static const uint8_t OFC_OFFSET_Y     = 0x39;
    static const uint8_t OFC_OFFSET_Z     = 0x3A;
    static const uint8_t D_X_LSB          = 0x02;
    static const uint8_t D_Y_LSB          = 0x04;
    static const uint8_t D_Z_LSB          = 0x06;
    static const uint8_t PMU_RANGE        = 0x0F;
    static const uint8_t PMU_BW           = 0x10;
    static const uint8_t PMU_LPW          = 0x11;
    static const uint8_t D_HBW            = 0x13;
    static const uint8_t INT_EN_1         = 0x17;
    static const uint8_t INT_MAP_1        = 0x1A;
    static const uint8_t INT_OUT_CTRL     = 0x20;
    static const uint8_t BGW_SOFTRESET    = 0x14;
    static const uint8_t INT_PIN          = 7;

    enum scaleOption : uint8_t {AFS_2G  = 0x03,
                                AFS_4G  = 0x05,
                                AFS_8G  = 0x08,
                                AFS_16G = 0x0C};
    // Bandwidths for filtered readings
    enum bandwidthOption : uint8_t {ABW_8Hz    = 0x08,    // 7.81 Hz,  64 ms update time
                                    ABW_16Hz   = 0x09,    // 15.63 Hz, 32 ms update time
                                    ABW_31Hz   = 0x0A,    // 31.25 Hz, 16 ms update time
                                    ABW_63Hz   = 0x0B,    // 62.5  Hz,  8 ms update time
                                    ABW_125Hz  = 0x0C,    // 125   Hz,  4 ms update time
                                    ABW_250Hz  = 0x0D,    // 250   Hz,  2 ms update time
                                    ABW_500Hz  = 0x0E,    // 500   Hz,  1 ms update time
                                    ABW_1000Hz = 0x0F};   // 1000  Hz,  0.5 ms update time
    static const scaleOption defaultScale = AFS_4G;
    static const bandwidthOption defaultBandwidth = ABW_16Hz;

    enum dataSendOption : uint8_t {dataX        = 0,
                                   dataY        = 1,
                                   dataZ        = 2,
                                   resolution   = 3,
                                   samplingRate = 4,
                                   optionCount  = 5};

    static const uint16_t defaultSamplingRate = 1000; // Hz

    // Constructor, destructor, getters and setters
    IUBMX055Acc(IUI2C *iuI2C);
    virtual ~IUBMX055Acc() {}
    virtual char getSensorType() { return (char) sensorType; }
    // Hardware & power management options
    void softReset();
    virtual void wakeUp();
    virtual void sleep();
    virtual void suspend();
    void setScale(scaleOption scale);
    void resetScale() { setScale(defaultScale); }
    void setBandwidth(bandwidthOption bandwidth);
    void useFilteredData(bandwidthOption bandwidth);
    void useUnfilteredData();
    void configureInterrupts();
    void doFastCompensation(float *destination);
    // Data acquisition methods
    void computeResolution();
    virtual void readData();
    bool isNewData() {return m_newData; }
    q15_t getData(uint8_t index) { return m_data[index]; }
    // Communication methods
    virtual void sendToReceivers();
    void dumpDataThroughI2C();
    virtual void dumpDataForDebugging();
    // Diagnostic Functions
    virtual void exposeCalibration();

  protected:
    static IUI2C *m_iuI2C;
    // Data acquisition
    static bool m_newData;
    static uint8_t m_rawBytes[6]; // 12bits / 2 bytes per axis: LSB, MSB => last 4 bits of LSB are not data, they are flags
    static q15_t m_rawData[3];    // Stores the Q15 accelerometer sensor raw output
    static q15_t m_data[3];       // Stores the Q15 data (with bias) in G
    static q15_t m_bias[3];       // Bias corrections
    static q15_t m_resolution;    // Resolution
    static void processData(uint8_t wireStatus);
    // Configurations
    scaleOption m_scale;          // Scale
    bandwidthOption m_bandwidth;  // Bandwidth
    bool m_filteredData;          // Use filter?
};


#endif // IUBMX055ACC_H
