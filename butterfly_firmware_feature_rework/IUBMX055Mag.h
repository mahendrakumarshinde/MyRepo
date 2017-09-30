#ifndef IUBMX055MAG_H
#define IUBMX055MAG_H

#include <Arduino.h>
#include "Sensor.h"
#include "IUI2C.h"


namespace IUComponent
{
    /**
     * Magnetometer
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
     *      - magneticX: a Q15Feature with section size = 128
     *      - magneticY: a Q15Feature with section size = 128
     *      - magneticZ: a Q15Feature with section size = 128
     */
    class IUBMX055Mag : public Sensor
    {
        public:
            /***** Preset values and default settings *****/
            static const uint8_t ADDRESS   = 0x10;
            static const uint8_t WHO_AM_I  = 0x40;
            static const uint8_t I_AM      = 0x32;
            // Suspend or active / forced / sleep
            static const uint8_t PWR_CNTL1 = 0x4B;
            // Operation modes (active / forced / sleep)
            static const uint8_t PWR_CNTL2 = 0x4C;
            // Repetitions (oversampling) register
            static const uint8_t REP_XY    = 0x51;
            // Repetitions (oversampling) register
            static const uint8_t REP_Z     = 0x52;
            // ODR: Output Data Rate
            enum ODROption : uint8_t {ODR_10Hz,  // 000b (default)
                                      ODR_2Hz,   // 001b
                                      ODR_6Hz,   // 010b
                                      ODR_8Hz,   // 011b
                                      ODR_15Hz,  // 100b
                                      ODR_20Hz,  // 101b
                                      ODR_25Hz,  // 110b
                                      ODR_30Hz}; // 111b
            enum accuracyPreset : uint8_t {
                LOWPOWER,        // RMS noise ~1.0 microTesla, 0.17 mA power
                REGULAR,         // RMS noise ~0.6 microTesla, 0.5 mA power
                ENHANCEDREGULAR, // RMS noise ~0.5 microTesla, 0.8 mA power
                HIGHACCURACY};   // RMS noise ~0.3 microTesla, 4.9 mA power
            static const ODROption defaultODR = ODR_10Hz;
            static const accuracyPreset defaultAccuracy = \
                accuracyPreset::LOWPOWER;
            static const uint16_t defaultSamplingRate = 1000; // Hz
            /***** Constructors and destructors *****/
            IUBMX055Mag(IUI2C *iuI2C, uint8_t id=100,
                        FeatureBuffer *magneticX=NULL,
                        FeatureBuffer *magneticY=NULL,
                        FeatureBuffer *magneticZ=NULL);
            virtual ~IUBMX055Mag() {}
            /***** Hardware & power management *****/
            void softReset();
            virtual void wakeUp();
            virtual void sleep();
            virtual void suspend();
            /***** Configuration and calibration *****/
            void enterForcedMode();
            void exitForcedMode();
            void setODR(ODROption ODR);
            void setAccuracy(accuracyPreset accuracy);
            /***** Data acquisition *****/
            virtual void readData() {}             // Not implemented
            /***** Communication *****/
            virtual void dumpDataThroughI2C() {}   // Not implemented
            virtual void dumpDataForDebugging() {} // Not implemented
            /***** Debugging *****/
            virtual void exposeCalibration() {}    // Not implemented

        protected:
            /***** Core *****/
            IUI2C *m_iuI2C;
            /***** Configuration and calibration *****/
            bool m_forcedMode;
            ODROption m_odr;
            accuracyPreset m_accuracy;
            q15_t m_bias[3];  // Bias corrections
            void writeODRRegister();
            /***** Data acquisition *****/
            int16_t m_rawData[3];  // 13/15-bit signed magnetometer output
            q15_t m_data[3];       // Latest data values

    };
};

#endif // IUBMX055MAG_H
