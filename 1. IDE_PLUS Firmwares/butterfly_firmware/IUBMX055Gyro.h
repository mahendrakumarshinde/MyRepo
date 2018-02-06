#ifndef IUBMX055GYRO_H
#define IUBMX055GYRO_H

#include <Arduino.h>
#include "Sensor.h"
#include "IUI2C.h"


/**
 * Gyroscope
 *
 * Component:
 *  Name:
 *    BMX-055 9-axis sensor
 *  Description:
 *    Accelerometer, Gyroscope and Magnetometer for Butterfly board. Each of
 *    the sensors has its own registers, power management and data
 *    acquisition configuration, so each sensor has its own class.
 *    Data sheet => http://ae-bst.resource.bosch.com/media/products/dokumente/bmx055/BST-BMX055-DS000-01v2.pdf
 * Destinations:
 *      - tiltX: a Q15Feature with section size = 128
 *      - tiltY: a Q15Feature with section size = 128
 *      - tiltZ: a Q15Feature with section size = 128
 */
class IUBMX055Gyro : public DrivenSensor
{
    public:
        /***** Preset values and default settings *****/
        static const uint8_t ADDRESS        = 0x68;
        static const uint8_t WHO_AM_I       = 0x00;
        static const uint8_t I_AM           = 0x0F;
        static const uint8_t RANGE          = 0x0F;
        static const uint8_t BW             = 0x10;
        static const uint8_t BGW_SOFTRESET  = 0x14;
        static const uint8_t LPM1           = 0x11;
        static const uint8_t LPM2           = 0x12;
        enum scaleOption : uint8_t {GFS_2000DPS,
                                    GFS_1000DPS,
                                    GFS_500DPS,
                                    GFS_250DPS,
                                    GFS_125DPS};
        enum bandwidthOption : uint8_t {
            BW_2000Hz523Hz, // 2000 Hz ODR and unfiltered (bandwidth 523Hz)
            BW_2000Hz230Hz,
            BW_1000Hz116Hz,
            BW_400Hz47Hz,
            BW_200Hz23Hz,
            BW_100Hz12Hz,
            BW_200Hz64Hz,
            BW_100Hz32Hz};  // 100 Hz ODR and 32 Hz bandwidth
        static const scaleOption defaultScale = GFS_125DPS;
        static const bandwidthOption defaultBandwidth = BW_200Hz23Hz;
        static const uint16_t defaultSamplingRate = 1000; // Hz
        /***** Constructors and destructors *****/
        IUBMX055Gyro(IUI2C *iuI2C, const char* name, Feature *tiltX=NULL,
                     Feature *tiltY=NULL, Feature *tiltZ=NULL);
        virtual ~IUBMX055Gyro() {}
        /***** Hardware & power management *****/
        virtual void setupHardware();
        void softReset();
        virtual void wakeUp();
        virtual void lowPower();
        virtual void suspend();
        /***** Configuration and calibration *****/
        virtual void configure(JsonVariant &config);
        void setScale(scaleOption scale);
        scaleOption getScale() { return m_scale; }
        void setBandwidth(bandwidthOption scale);
        /***** Data acquisition *****/
        virtual void readData() {}             // Not implemented
        /***** Debugging *****/
        virtual void exposeCalibration() {}    // Not implemented

    protected:
        /***** Core *****/
        IUI2C *m_iuI2C;
        /***** Configuration and calibration *****/
        scaleOption m_scale;
        bandwidthOption m_bandwidth;
        bool m_fastPowerUpEnabled;
        q15_t m_bias[3];  // Bias corrections
        /***** Data acquisition *****/
        int16_t m_rawData[3];  // 16-bit signed gyro sensor output
        q15_t m_data[3];  // Latest data values
};

/***** Instantiation *****/

extern IUBMX055Gyro iuGyroscope;

#endif // IUBMX055GYRO_H
