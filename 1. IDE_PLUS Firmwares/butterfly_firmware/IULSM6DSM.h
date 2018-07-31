#ifndef IULSM6DSM_H
#define IULSM6DSM_H

#include <Arduino.h>
#include "Sensor.h"
#include "IUI2C.h"
#include "FeatureUtilities.h"


// TODO => Separate Accelerometer and Gyroscope, as it doesn't fit in the model
// For example, a sensor can have only 1 resolution.

/**
 * Accelerometer
 *
 * Component:
 *  Name:
 *    IULSM6DSM 6-axis accelerometer + gyroscope
 *  Description:
 *    Accelerometer
 * Destinations:
 *      - accelerationX: a Q15 feature
 *      - accelerationY: a Q15 feature
 *      - accelerationZ: a Q15 feature
 */
class IULSM6DSM : public HighFreqSensor
{
    public:
        /***** Preset values and default settings *****/
        static const uint8_t INT1_PIN         = 41; // if Dragonfly
        static const uint8_t ADDRESS          = 0x6A;
        static const uint8_t WHO_AM_I         = 0x0F;
        static const uint8_t I_AM             = 0x6A;
        static const uint8_t CTRL1_XL         = 0x10;  // Activate Acc
        static const uint8_t CTRL2_G          = 0x11;  // Activate Gyro
        static const uint8_t CTRL3_C          = 0x12;  // Data read options
        // Low Pass filtering config
        static const uint8_t CTRL8_XL         = 0x17;
        // Data output register
        static const uint8_t OUT_TEMP_L       = 0x20;
        static const uint8_t OUT_TEMP_H       = 0x21;
        static const uint8_t OUTX_L_G         = 0x22;
        static const uint8_t OUTX_H_G         = 0x23;
        static const uint8_t OUTY_L_G         = 0x24;
        static const uint8_t OUTY_H_G         = 0x25;
        static const uint8_t OUTZ_L_G         = 0x26;
        static const uint8_t OUTZ_H_G         = 0x27;
        static const uint8_t OUTX_L_XL        = 0x28;
        static const uint8_t OUTX_H_XL        = 0x29;
        static const uint8_t OUTY_L_XL        = 0x2A;
        static const uint8_t OUTY_H_XL        = 0x2B;
        static const uint8_t OUTZ_L_XL        = 0x2C;
        static const uint8_t OUTZ_H_XL        = 0x2D;
        // Interrupt configuration
        static const uint8_t DRDY_PULSE_CFG   = 0x0B;
        static const uint8_t INT1_CTRL        = 0x0D;
        static const uint8_t INT2_CTRL        = 0x0E;
        // accelerometer scaling
        enum scaleOption : uint8_t {AFS_2G  = 0x00,
                                    AFS_4G  = 0x02,
                                    AFS_8G  = 0x03,
                                    AFS_16G = 0x01};
        // gyroscope scaling
        enum gyroScaleOption : uint8_t {GFS_250DPS  = 0x00,
                                        GFS_500DPS  = 0x01,
                                        GFS_1000DPS  = 0x02,
                                        GFS_2000DPS = 0x03};
        // ODR are the same for accel and gyro in normal mode
        enum ODROption : uint8_t {ODR_12_5Hz =  0x01,
                                  ODR_26Hz   =  0x02,
                                  ODR_52Hz   =  0x03,
                                  ODR_104Hz  =  0x04,
                                  ODR_208Hz  =  0x05,
                                  ODR_416Hz  =  0x06,
                                  ODR_833Hz  =  0x07,
                                  ODR_1660Hz =  0x08,
                                  ODR_3330Hz =  0x09,
                                  ODR_6660Hz =  0x0A};
        static const scaleOption defaultScale = AFS_4G;
        static const gyroScaleOption defaultGyroScale = GFS_250DPS;
        static const ODROption defaultODR = ODR_1660Hz;
        static const uint16_t defaultSamplingRate = 1000; // Hz
        /***** Constructors and destructors *****/
        IULSM6DSM(IUI2C *iuI2C, const char* name,
                  void (*i2cReadCallback)(uint8_t wireStatus),
                  FeatureTemplate<q15_t> *accelerationX,
                  FeatureTemplate<q15_t> *accelerationY,
                  FeatureTemplate<q15_t> *accelerationZ,
                  FeatureTemplate<q15_t> *tiltX,
                  FeatureTemplate<q15_t> *tiltY,
                  FeatureTemplate<q15_t> *tiltZ,
                  FeatureTemplate<float> *temperature);
        virtual ~IULSM6DSM() {}
        /***** Hardware & power management *****/
        virtual void setupHardware();
        void softReset();
        virtual void setPowerMode(PowerMode::option pMode);
        virtual void setSamplingRate(uint16_t samplingRate);
        void configureInterrupts();
        void computeBias();
        /***** Configuration and calibration *****/
        virtual void configure(JsonVariant &config);
        void setScale(scaleOption scale);
        scaleOption getScale() { return m_scale; }
        void resetScale() { setScale(defaultScale); }
        void setGyroScale(gyroScaleOption gyroScale);
        gyroScaleOption getGyroScale() { return m_gyroScale; }
        void resetGyroScale() { setGyroScale(defaultGyroScale); }
        ODROption getODR() { return m_odr; }
        /***** Data acquisition *****/
        virtual void readData();
        void processData(uint8_t wireStatus);
        q15_t getData(uint8_t index) { return m_data[index]; }
        /***** Communication *****/
        void sendData(HardwareSerial *port);
        /***** Debugging *****/
        virtual void exposeCalibration();

    protected:
        /***** Core *****/
        IUI2C *m_iuI2C;
        void setODR(ODROption odr);
        /***** Configuration and calibration *****/
        scaleOption m_scale;
        gyroScaleOption m_gyroScale;
        ODROption m_odr;
        /***** Data acquisition *****/
        void (*m_readCallback)(uint8_t wireStatus);
        volatile bool m_readingData;
        // 14bits = 2 bytes per value (Temp, accel X, Y, Z and gyro X, Y, Z)
        uint8_t m_rawBytes[14];
        float m_temperature;
        q15_t m_rawData[3];     // Q15 accelerometer raw output
        q15_t m_data[3];        // Q15 data (with bias) in G
        q15_t m_bias[3];        // Bias corrections
        q15_t m_rawGyroData[3];     // Q15 accelerometer raw output
        q15_t m_gyroData[3];        // Q15 data (with bias) in G
        q15_t m_gyroBias[3];        // Bias corrections
};

#endif // IULSM6DSM_H
